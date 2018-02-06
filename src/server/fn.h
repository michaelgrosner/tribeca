#ifndef K_FN_H_
#define K_FN_H_

#define _numsAz_ "0123456789"                 \
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                 "abcdefghijklmnopqrstuvwxyz"

#define _Tstamp_ chrono::duration_cast<chrono::milliseconds>(     \
                   chrono::system_clock::now().time_since_epoch() \
                 ).count()

namespace K {
  class FN {
    public:
      static string S2l(string k) { transform(k.begin(), k.end(), k.begin(), ::tolower); return k; };
      static string S2u(string k) { transform(k.begin(), k.end(), k.begin(), ::toupper); return k; };
      static unsigned long long int64() {
        static random_device rd;
        static mt19937_64 gen(rd());
        return uniform_int_distribution<unsigned long long>()(gen);
      };
      static string int45Id() {
        return to_string(int64()).substr(0,10);
      };
      static string int32Id() {
        return to_string(int64()).substr(0,8);
      };
      static string char16Id() {
        char s[16];
        for (unsigned int i = 0; i < 16; ++i) s[i] = _numsAz_[int64() % (sizeof(_numsAz_) - 1)];
        return string(s, 16);
      };
      static string uuid36Id() {
        string uuid = string(36,' ');
        unsigned long long rnd = int64();
        unsigned long long rnd_ = int64();
        uuid[8] = '-';
        uuid[13] = '-';
        uuid[18] = '-';
        uuid[23] = '-';
        uuid[14] = '4';
        for (unsigned int i=0;i<36;i++)
          if (i != 8 && i != 13 && i != 18 && i != 14 && i != 23) {
            if (rnd <= 0x02) rnd = 0x2000000 + (rnd_ * 0x1000000) | 0;
            rnd >>= 4;
            uuid[i] = _numsAz_[(i == 19) ? ((rnd & 0xf) & 0x3) | 0x8 : rnd & 0xf];
          }
        return S2l(uuid);
      };
      static string uuid32Id() {
        string uuid = uuid36Id();
        uuid.erase(remove(uuid.begin(), uuid.end(), '-'), uuid.end());
        return uuid;
      }
      static string oZip(string k) {
        z_stream zs;
        if (inflateInit2(&zs, -15) != Z_OK) return "";
        zs.next_in = (Bytef*)k.data();
        zs.avail_in = k.size();
        int ret;
        char outbuffer[32768];
        string k_;
        do {
          zs.avail_out = 32768;
          zs.next_out = (Bytef*)outbuffer;
          ret = inflate(&zs, Z_SYNC_FLUSH);
          if (k_.size() < zs.total_out)
            k_.append(outbuffer, zs.total_out - k_.size());
        } while (ret == Z_OK);
        inflateEnd(&zs);
        if (ret != Z_STREAM_END) return "";
        return k_;
      };
      static string oHex(string k) {
       unsigned int len = k.length();
        string k_;
        for (unsigned int i=0; i < len; i+=2) {
          string byte = k.substr(i,2);
          char chr = (char)(int)strtol(byte.data(), NULL, 16);
          k_.push_back(chr);
        }
        return k_;
      };
      static string oB64(string k) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, k.data(), k.length());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);
        BIO_set_close(bio, BIO_NOCLOSE);
        BIO_free_all(bio);
        return string(bufferPtr->data, bufferPtr->length);
      };
      static string oB64decode(string k) {
        BIO *bio, *b64;
        char buffer[k.length()];
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_mem_buf(k.data(), k.length());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_set_close(bio, BIO_NOCLOSE);
        int len = BIO_read(bio, buffer, k.length());
        BIO_free_all(bio);
        return string(buffer, len);
      };
      static string oMd5(string k) {
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
        char k_[16*2+1];
        for (unsigned int i = 0; i < 16; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return S2u(k_);
      };
      static string oSha256(string k) {
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
        char k_[SHA256_DIGEST_LENGTH*2+1];
        for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static string oSha512(string k) {
        unsigned char digest[SHA512_DIGEST_LENGTH];
        SHA512((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
        char k_[SHA512_DIGEST_LENGTH*2+1];
        for (unsigned int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static string oHmac256(string p, string s, bool hex = false) {
        unsigned char* digest;
        digest = HMAC(EVP_sha256(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
        char k_[SHA256_DIGEST_LENGTH*2+1];
        for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return hex ? oHex(k_) : k_;
      };
      static string oHmac512(string p, string s) {
        unsigned char* digest;
        digest = HMAC(EVP_sha512(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
        char k_[SHA512_DIGEST_LENGTH*2+1];
        for (unsigned int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static string oHmac384(string p, string s) {
        unsigned char* digest;
        digest = HMAC(EVP_sha384(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
        char k_[SHA384_DIGEST_LENGTH*2+1];
        for (unsigned int i = 0; i < SHA384_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
        return k_;
      };
      static void stunnel() {
        system("(pkill stunnel || :) && stunnel etc/K-stunnel.conf");
      };
      static int memory() {
        string ps = output(string("ps -p") + to_string(::getpid()) + " -orss | tail -n1");
        ps.erase(remove(ps.begin(), ps.end(), ' '), ps.end());
        if (ps.empty()) ps = "0";
        return stoi(ps) * 1e+3;
      };
      static string output(string cmd) {
        string data;
        FILE *stream;
        const int max_buffer = 256;
        char buffer[max_buffer];
        cmd.append(" 2>&1");
        stream = popen(cmd.data(), "r");
        if (stream) {
          while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
          pclose(stream);
        }
        return data;
      };
      static string readlink(const char* path) {
        string buffer(64, '\0');
        ssize_t len;
        while((len = ::readlink(path, &buffer[0], buffer.size())) == (ssize_t)buffer.size())
          buffer.resize(buffer.size() * 2);
        if (len == -1) ((SH*)screen)->logWar("FN", "readlink failed");
        buffer.resize(len);
        return buffer;
      };
      static json   wJet(string k, bool f = false) {
        return json::parse(wGet(k, f));
      };
      static string wGet(string k, bool f) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          if (f) curl_easy_setopt(curl, CURLOPT_TIMEOUT, 4L);
          else curl_easy_setopt(curl, CURLOPT_TIMEOUT, 13L);
          CURLcode r = curl_easy_perform(curl);
          if (!f and r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wGet failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string p) {
        return json::parse(wGet(k, p));
      };
      static string wGet(string k, string p) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string t, bool auth) {
        return json::parse(wGet(k, t, auth));
      };
      static string wGet(string k, string t, bool auth) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          if (!t.empty()) h_ = curl_slist_append(h_, string("Authorization: Bearer ").append(t).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, bool p, string a, string s, string n) {
        return json::parse(wGet(k, p, a, s, n));
      };
      static string wGet(string k, bool p, string a, string s, string n) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, n.data());
          h_ = curl_slist_append(h_, string("API-Key: ").append(a).data());
          h_ = curl_slist_append(h_, string("API-Sign: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, bool a, string p, string s) {
        return json::parse(wGet(k, a, p, s));
      };
      static string wGet(string k, bool a, string p, string s) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          if (a) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.data());
          }
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_USERPWD, p.data());
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wGet failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string p, string s, bool post) {
        return json::parse(wGet(k, p, s, post));
      };
      static string wGet(string k, string p, string s, bool post) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, string("X-Signature: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string p, string a, string s) {
        return json::parse(wGet(k, p, a, s));
      };
      static string wGet(string k, string p, string a, string s) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          h_ = curl_slist_append(h_, string("Key: ").append(a).data());
          h_ = curl_slist_append(h_, string("Sign: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string p, string a, string s, bool post) {
        return json::parse(wGet(k, p, a, s, post));
      };
      static string wGet(string k, string p, string a, string s, bool post) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, string("X-BFX-APIKEY: ").append(a).data());
          h_ = curl_slist_append(h_, string("X-BFX-PAYLOAD: ").append(p).data());
          h_ = curl_slist_append(h_, string("X-BFX-SIGNATURE: ").append(s).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string p, string a, string s, bool post, bool auth) {
        return json::parse(wGet(k, p, a, s, post, auth));
      };
      static string wGet(string k, string p, string a, string s, bool post, bool auth) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
          h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
          if (!a.empty()) h_ = curl_slist_append(h_, string("Authorization: Bearer ").append(a).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wPost failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string t, string a, string s, string p) {
        return json::parse(wGet(k, t, a, s, p));
      };
      static string wGet(string k, string t, string a, string s, string p) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          h_ = curl_slist_append(h_, string("CB-ACCESS-KEY: ").append(a).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-SIGN: ").append(s).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-TIMESTAMP: ").append(t).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-PASSPHRASE: ").append(p).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wGet failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static json   wJet(string k, string t, string a, string s, string p, bool d) {
        return json::parse(wGet(k, t, a, s, p, d));
      };
      static string wGet(string k, string t, string a, string s, string p, bool d) {
        string k_;
        CURL* curl;
        curl = curl_easy_init();
        if (curl) {
          struct curl_slist *h_ = NULL;
          curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/K-cabundle.pem");
          curl_easy_setopt(curl, CURLOPT_URL, k.data());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &wcb);
          h_ = curl_slist_append(h_, string("CB-ACCESS-KEY: ").append(a).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-SIGN: ").append(s).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-TIMESTAMP: ").append(t).data());
          h_ = curl_slist_append(h_, string("CB-ACCESS-PASSPHRASE: ").append(p).data());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, &k_);
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
          CURLcode r = curl_easy_perform(curl);
          if(r != CURLE_OK) ((SH*)screen)->logWar("CURL", string("wGet failed ") + curl_easy_strerror(r));
          curl_easy_cleanup(curl);
        }
        if (k_.empty() or (k_[0]!='{' and k_[0]!='[')) k_ = "{}";
        return k_;
      };
      static size_t wcb(void *buf, size_t size, size_t nmemb, void *up) {
        ((string*)up)->append((char*)buf, size * nmemb);
        return size * nmemb;
      };
  };
}

#endif
