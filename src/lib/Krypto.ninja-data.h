#ifndef K_DATA_H_
#define K_DATA_H_
//! \file
//! \brief Data transfer/transform helpers.

namespace ₿ {
  class Curl {
    public:
      static function<void(CURL*)> global_setopt;
      static void cleanup(CURL *&curl, curl_socket_t &sockfd) {
        if (curl) curl_easy_cleanup(curl);
        curl   = nullptr;
        sockfd = 0;
      };
      static const CURLcode connect(CURL *&curl, curl_socket_t &sockfd, string &buffer, const string &wss) {
        CURLcode rc = CURLE_URL_MALFORMAT;
        CURLU *url = curl_url();
        char *host,
             *port,
             *path;
        if (  !curl_url_set(url, CURLUPART_URL, ("http" + wss.substr(2)).data(), 0)
          and !curl_url_get(url, CURLUPART_HOST, &host, 0)
          and !curl_url_get(url, CURLUPART_PORT, &port, CURLU_DEFAULT_PORT)
          and !curl_url_get(url, CURLUPART_PATH, &path, 0)
          and CURLE_OK == (rc = init(curl, sockfd))
        ) {
          global_setopt(curl);
          curl_easy_setopt(curl, CURLOPT_URL, ("http" + wss.substr(2)).data());
          curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
          if ( CURLE_OK != (rc = curl_easy_perform(curl))
            or CURLE_OK != (rc = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd))
            or CURLE_OK != (rc = send(curl, sockfd, "GET " + string(path) + " HTTP/1.1"
                                                    "\r\n" "Host: " + string(host) + ":" + string(port) +
                                                    "\r\n" "Upgrade: websocket"
                                                    "\r\n" "Connection: Upgrade"
                                                    "\r\n" "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw=="
                                                    "\r\n" "Sec-WebSocket-Version: 13"
                                                    "\r\n"
                                                    "\r\n"))
            or CURLE_OK != (rc = recv(curl, sockfd, buffer, 5))
            or string::npos == buffer.find("HTTP/1.1 101 Switching Protocols")
            or string::npos == buffer.find("HSmrc0sMlYUkAGmm5OPpG2HaGWk=")
          ) {
            if (rc == CURLE_OK)
              rc = CURLE_WEIRD_SERVER_REPLY;
            cleanup(curl, sockfd);
          } else buffer = buffer.substr(buffer.rfind("\r\n\r\n") + 4);
        }
        curl_url_cleanup(url);
        return rc;
      };
      static const CURLcode emit(CURL *&curl, curl_socket_t &sockfd, const string &data, const int &opcode) {
        CURLcode rc;
        if (CURLE_OK != (rc = send(curl, sockfd, frame(data, opcode))))
          cleanup(curl, sockfd);
        return rc;
      };
      static const CURLcode receive(CURL *&curl, curl_socket_t &sockfd, string &buffer) {
        CURLcode rc;
        if (CURLE_OPERATION_TIMEDOUT == (rc = recv(curl, sockfd, buffer, 0)))
          rc = CURLE_OK;
        if (rc != CURLE_OK)
          cleanup(curl, sockfd);
        return rc;
      };
      static const json xfer(const string &url, const long &timeout = 13) {
        return request(url, [&](CURL *curl) {
          curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        });
      };
      static const json xfer(const string &url, const string &post) {
        return request(url, [&](CURL *curl) {
          struct curl_slist *h_ = nullptr;
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
        });
      };
      static const json request(const string &url, const function<void(CURL*)> custom_setopt) {
        static mutex waiting_reply;
        lock_guard<mutex> lock(waiting_reply);
        string reply;
        CURLcode rc = CURLE_FAILED_INIT;
        CURL *curl = curl_easy_init();
        if (curl) {
          custom_setopt(curl);
          global_setopt(curl);
          curl_easy_setopt(curl, CURLOPT_URL, url.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reply);
          rc = curl_easy_perform(curl);
          curl_easy_cleanup(curl);
        }
        return rc == CURLE_OK
          ? (json::accept(reply)
              ? json::parse(reply)
              : json::object()
            )
          : (json){ {"error", string("CURL Request Error: ") + curl_easy_strerror(rc)} };
      };
      static void unframe(CURL *&curl, curl_socket_t &sockfd, string &data, string &msg) {
        const size_t max = data.length();
        if (max < 2) return;
        unsigned int pos = 2,
                     len = 0;
        if      (data[1] <= 0x7D) len =    data[1];
        else if (data[1] == 0x7E) len = (((data[2] & 0xFF) <<  8)
                                      |   (data[3] & 0xFF)       ), pos += 2;
        else if (data[1] == 0x7F) len = (((data[6] & 0xFF) << 24)
                                      |  ((data[7] & 0xFF) << 16)
                                      |  ((data[8] & 0xFF) <<  8)
                                      |   (data[9] & 0xFF)       ), pos += 8;
        const unsigned int key = (data[1] >> 7) & 0x01 ? 4 : 0;
        pos += key;
        if (max < pos + len) return;
        if (key)
          for (int i = 0; i < len; i++)
            data.at(pos + i) ^= data.at(pos - key + (i % key));
        const unsigned char opcode = data[0] & 0x0F;
        if (opcode == 0x2 or opcode == 0xA or opcode == 0x8
          or ((opcode == 0x0 or opcode == 0x1) and !((data[0] >> 7) & 0x01))
        ) {
          if (opcode == 0x8)
            cleanup(curl, sockfd);
        } else if (opcode == 0x9)
          emit(curl, sockfd, data.substr(pos, len), 0xA);
        else msg = data.substr(pos, len);
        data = data.substr(pos + len);
      };
    private:
      static size_t write(void *buf, size_t size, size_t nmemb, void *reply) {
        ((string*)reply)->append((char*)buf, size *= nmemb);
        return size;
      };
      static const string frame(string data, const int &opcode) {
        const int key = rand();
        unsigned int pos = 0,
                     len = data.length();
                                data.insert(pos++  , 1,  opcode     | 0x80 );
        if      (len <= 0x7D)   data.insert(pos++  , 1,  len        | 0x80 );
        else if (len <= 0xFFFF) data.insert(pos    , 1, (char)(0x7E | 0x80))
                                    .insert(pos + 1, 1, (len >>  8) & 0xFF )
                                    .insert(pos + 2, 1,  len        & 0xFF ), pos += 3;
        else                    data.insert(pos    , 1, (char)(0x7F | 0x80))
                                    .insert(pos + 1, 4,                  0 )
                                    .insert(pos + 5, 1, (len >> 24) & 0xFF )
                                    .insert(pos + 6, 1, (len >> 16) & 0xFF )
                                    .insert(pos + 7, 1, (len >>  8) & 0xFF )
                                    .insert(pos + 8, 1,  len        & 0xFF ), pos += 9;
                                data.insert(pos    , 1, (key >> 24) & 0xFF )
                                    .insert(pos + 1, 1, (key >> 16) & 0xFF )
                                    .insert(pos + 2, 1, (key >>  8) & 0xFF )
                                    .insert(pos + 3, 1,  key        & 0xFF ); pos += 4;
        for (int i = 0; i < len; i++)
          data.at(pos + i) ^= data.at(pos - 4 + (i % 4));
        return data;
      };
      static const CURLcode init(CURL *&curl, curl_socket_t &sockfd) {
        if (!curl) curl = curl_easy_init();
        else curl_easy_reset(curl);
        sockfd = 0;
        return curl
          ? CURLE_OK
          : CURLE_FAILED_INIT;
      };
      static const CURLcode send(CURL *curl, const curl_socket_t &sockfd, const string &data) {
        CURLcode rc;
        size_t len  = data.length(),
               sent = 0;
        do {
          do {
            size_t n = 0;
            rc = curl_easy_send(curl, data.substr(sent).data(), len - sent, &n);
            sent += n;
            if (rc == CURLE_AGAIN
              and !wait(sockfd, false, 5)
            ) return CURLE_OPERATION_TIMEDOUT;
          } while (rc == CURLE_AGAIN);
          if (rc != CURLE_OK) break;
        } while (sent < len);
        return rc;
      };
      static const CURLcode recv(CURL *curl, curl_socket_t &sockfd, string &buffer, const int &timeout) {
        CURLcode rc;
        for(;;) {
          char data[524288];
          size_t n;
          do {
            n = 0;
            rc = curl_easy_recv(curl, data, sizeof(data), &n);
            buffer.append(data, n);
            if (rc == CURLE_AGAIN
              and !wait(sockfd, true, timeout)
            ) return CURLE_OPERATION_TIMEDOUT;
          } while (rc == CURLE_AGAIN);
          if ((timeout and buffer.find("\r\n\r\n") != string::npos)
            or rc != CURLE_OK
            or n == 0
          ) break;
        }
        return rc;
      };
      static const int wait(const curl_socket_t &sockfd, const bool &io, const int &timeout) {
        struct timeval tv;
        tv.tv_sec  = timeout;
        tv.tv_usec = 10000;
        fd_set infd,
               outfd;
        FD_ZERO(&infd);
        FD_ZERO(&outfd);
        FD_SET(sockfd, io ? &infd : &outfd);
        return select(sockfd + 1, &infd, &outfd, nullptr, &tv);
      };
  };

  class Text {
    public:
      static string strL(string input) {
        transform(input.begin(), input.end(), input.begin(), ::tolower);
        return input;
      };
      static string strU(string input) {
        transform(input.begin(), input.end(), input.begin(), ::toupper);
        return input;
      };
      static string B64(const string &input) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        BIO_set_close(bio, BIO_CLOSE);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, input.data(), input.length());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);
        const string output(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);
        return output;
      };
      static string B64_decode(const string &input) {
        BIO *bio, *b64;
        char output[input.length()];
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_mem_buf(input.data(), input.length());
        bio = BIO_push(b64, bio);
        BIO_set_close(bio, BIO_CLOSE);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        int len = BIO_read(bio, output, input.length());
        BIO_free_all(bio);
        return string(output, len);
      };
      static string SHA256(const string &input, const bool &hex = false) {
        return SHA(input, hex, ::SHA256, SHA256_DIGEST_LENGTH);
      };
      static string SHA512(const string &input) {
        return SHA(input, false, ::SHA512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC1(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha1, SHA_DIGEST_LENGTH);
      };
      static string HMAC256(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha256, SHA256_DIGEST_LENGTH);
      };
      static string HMAC512(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC384(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha384, SHA384_DIGEST_LENGTH);
      };
    private:
      static string SHA(
        const string  &input,
        const bool    &hex,
        unsigned char *(*md)(const unsigned char*, size_t, unsigned char*),
        const int     &digest_len
      ) {
        unsigned char digest[digest_len];
        md((unsigned char*)input.data(), input.length(), (unsigned char*)&digest);
        char output[digest_len * 2 + 1];
        for (unsigned int i = 0; i < digest_len; i++)
          sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
        return hex ? HEX(output) : output;
      };
      static string HMAC(
        const string &key,
        const string &input,
        const bool   &hex,
        const EVP_MD *(evp_md)(),
        const int    &digest_len
      ) {
        unsigned char* digest;
        digest = ::HMAC(
          evp_md(),
          input.data(), input.length(),
          (unsigned char*)key.data(), key.length(),
          nullptr, nullptr
        );
        char output[digest_len * 2 + 1];
        for (unsigned int i = 0; i < digest_len; i++)
          sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
        return hex ? HEX(output) : output;
      };
      static string HEX(const string &input) {
        const unsigned int len = input.length();
        string output;
        for (unsigned int i = 0; i < len; i += 2)
          output.push_back(
            (char)(int)strtol(input.substr(i, 2).data(), nullptr, 16)
          );
        return output;
      };
  };

  class Random {
    public:
      static const unsigned long long int64() {
        static random_device rd;
        static mt19937_64 gen(rd());
        return uniform_int_distribution<unsigned long long>()(gen);
      };
      static const RandId int45Id() {
        return to_string(int64()).substr(0, 10);
      };
      static const RandId int32Id() {
        return to_string(int64()).substr(0,  8);
      };
      static const RandId char16Id() {
        string id = string(16, ' ');
        for (auto &it : id) {
         const int offset = int64() % (26 + 26 + 10);
         if (offset < 26)           it = 'a' + offset;
         else if (offset < 26 + 26) it = 'A' + offset - 26;
         else                       it = '0' + offset - 26 - 26;
        }
        return id;
      };
      static const RandId uuid36Id() {
        string uuid = string(36, ' ');
        uuid[8]  =
        uuid[13] =
        uuid[18] =
        uuid[23] = '-';
        uuid[14] = '4';
        unsigned long long rnd = int64();
        for (auto &it : uuid)
          if (it == ' ') {
            if (rnd <= 0x02) rnd = 0x2000000 + (int64() * 0x1000000) | 0;
            rnd >>= 4;
            const int offset = (uuid[17] != ' ' and uuid[19] == ' ')
              ? ((rnd & 0xf) & 0x3) | 0x8
              : rnd & 0xf;
            if (offset < 10) it = '0' + offset;
            else             it = 'a' + offset - 10;
          }
        return uuid;
      };
      static const RandId uuid32Id() {
        RandId uuid = uuid36Id();
        uuid.erase(remove(uuid.begin(), uuid.end(), '-'), uuid.end());
        return uuid;
      }
  };
}

#endif
