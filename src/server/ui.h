#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  class UI: public Klass {
    private:
      int connections = 0;
      string B64auth = "",
             notes = "";
      map<uiTXT, string> queue;
      map<char, function<json()>*> hello;
      map<char, function<void(json)>*> kiss;
      double delayUI = 0;
      bool toggleSettings = true;
      mutex wsMutex;
    public:
      unsigned int orders60sec = 0;
    protected:
      void load() {
        if (((CF*)config)->argHeadless) return;
        if (((CF*)config)->argUser != "NULL" and ((CF*)config)->argPass != "NULL"
          and ((CF*)config)->argUser.length() > 0 and ((CF*)config)->argPass.length() > 0
        ) {
          B64::Encode(((CF*)config)->argUser + ':' + ((CF*)config)->argPass, &B64auth);
          B64auth = string("Basic ") + B64auth;
        }
        ((EV*)events)->uiGroup->onConnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          connections++;
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          FN::logUIsess(connections, addr);
        });
        ((EV*)events)->uiGroup->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          connections--;
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          FN::logUIsess(connections, addr);
        });
        ((EV*)events)->uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          string document;
          stringstream content;
          string auth = req.getHeader("authorization").toString();
          string addr = res->getHttpSocket()->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          lock_guard<mutex> lock(wsMutex);
          if (((CF*)config)->argWhitelist != "" and ((CF*)config)->argWhitelist.find(addr) == string::npos) {
            FN::log("UI", "dropping gzip bomb on", addr);
            content << ifstream("etc/K-bomb.gzip").rdbuf();
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            document += "Content-Encoding: gzip\r\nContent-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          } else if (B64auth != "" && auth == "") {
            FN::log("UI", "authorization attempt from", addr);
            document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (B64auth != "" && auth != B64auth) {
            FN::log("UI", "authorization failed from", addr);
            document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (req.getMethod() == uWS::HttpMethod::METHOD_GET) {
            string url = "";
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            string path = req.getUrl().toString();
            string::size_type n = 0;
            while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
            const string leaf = path.substr(path.find_last_of('.')+1);
            if (leaf == "/") {
              FN::log("UI", "authorization success from", addr);
              document += "Content-Type: text/html; charset=UTF-8\r\n";
              url = "/index.html";
            } else if (leaf == "js") {
              document += "Content-Type: application/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n";
              url = path;
            } else if (leaf == "css") {
              document += "Content-Type: text/css; charset=UTF-8\r\n";
              url = path;
            } else if (leaf == "png") {
              document += "Content-Type: image/png\r\n";
              url = path;
            } else if (leaf == "mp3") {
              document += "Content-Type: audio/mpeg\r\n";
              url = path;
            }
            if (url.length() > 0) content << ifstream(FN::readlink("app/client").substr(3) + url).rdbuf();
            else {
              struct timespec txxs;
              clock_gettime(CLOCK_MONOTONIC, &txxs);
              srand((time_t)txxs.tv_nsec);
              if (rand() % 21) {
                document = "HTTP/1.1 404 Not Found\r\n";
                content << "Today, is a beautiful day.";
              } else { // Humans! go to any random url to check your luck
                document = "HTTP/1.1 418 I'm a teapot\r\n";
                content << "Today, is your lucky day!";
              }
            }
            document += "Content-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          }
        });
        ((EV*)events)->uiGroup->onMessage([&](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          if ((((CF*)config)->argWhitelist != "" and ((CF*)config)->argWhitelist.find(addr) == string::npos) or length < 2)
            return;
          if (uiBIT::Hello == (uiBIT)message[0] and hello.find(message[1]) != hello.end()) {
            json reply = (*hello[message[1]])();
            if (!reply.is_null()) {
              lock_guard<mutex> lock(wsMutex);
              webSocket->send(string(message, 2).append(reply.dump()).data(), uWS::OpCode::TEXT);
            }
          } else if (uiBIT::Kiss == (uiBIT)message[0] and kiss.find(message[1]) != kiss.end())
            (*kiss[message[1]])(json::parse((length > 2 and (message[2] == '[' or message[2] == '{'))
              ? string(message, length).substr(2, length-2) : "{}"
            ));
        });
      };
      void waitTime() {
        if (((CF*)config)->argHeadless) return;
        ((EV*)events)->tDelay->data = (void*)this;
        uv_timer_start(((EV*)events)->tDelay, [](uv_timer_t *handle) {
          if (((CF*)((UI*)handle->data)->config)->argDebugEvents) FN::log("DEBUG", "EV GW tDelay timer");
          ((UI*)handle->data)->sendQueue();
        }, 0, 0);
      };
      void waitUser() {
        welcome(uiTXT::ApplicationState, &helloServer);
        welcome(uiTXT::Notepad, &helloNotes);
        clickme(uiTXT::Notepad, &kissNotes);
        welcome(uiTXT::ToggleSettings, &helloSettings);
        clickme(uiTXT::ToggleSettings, &kissSettings);
      };
      void run() {
        ((EV*)events)->listen(&wsMutex, ((CF*)config)->argHeadless, ((CF*)config)->argPort);
      };
    public:
      void welcome(uiTXT k, function<json()> *cb) {
        if (((CF*)config)->argHeadless) return;
        if (hello.find((char)k) == hello.end())
          hello[(char)k] = cb;
        else FN::logExit("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event", EXIT_SUCCESS);
      };
      void clickme(uiTXT k, function<void(json)> *cb) {
        if (((CF*)config)->argHeadless) return;
        if (kiss.find((char)k) == kiss.end())
          kiss[(char)k] = cb;
        else FN::logExit("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event", EXIT_SUCCESS);
      };
      void delayme(double next) {
        if (((CF*)config)->argHeadless) return;
        delayUI = next;
        uv_timer_set_repeat(((EV*)events)->tDelay, delayUI > 0 ? (int)(delayUI*1e+3) : 6e+4);
        uv_timer_again(((EV*)events)->tDelay);
      };
      void send(uiTXT k, json o, bool delayed = false) {
        if (((CF*)config)->argHeadless or connections == 0) return;
        if (delayUI > 0 and delayed) msg2Queue(k, o.dump());
        else msg2Client(k, o.dump());
      };
    private:
      function<json()> helloServer = [&]() {
        return (json){ serverState() };
      };
      function<json()> helloNotes = [&]() {
        return (json){ notes };
      };
      function<void(json)> kissNotes = [&](json k) {
        if (!k.is_null() and k.size())
          notes = k.at(0);
      };
      function<json()> helloSettings = [&]() {
        return (json){ toggleSettings };
      };
      function<void(json)> kissSettings = [&](json k) {
        if (!k.is_null() and k.size())
          toggleSettings = k.at(0);
      };
      void msg2Client(uiTXT k, string j) {
        string m(1, (char)uiBIT::Kiss);
        m += string(1, (char)k) + j;
        lock_guard<mutex> lock(wsMutex);
        ((EV*)events)->uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
      };
      void msg2Queue(uiTXT k, string j) {
        queue[k] = j;
      };
      void sendQueue(bool *sec60) {
        static unsigned long uiT_1m = 0;
        for (map<uiTXT, string>::iterator it = queue.begin(); it != queue.end(); ++it)
          msg2Client(it->first, it->second);
        queue.clear();
        if (uiT_1m+6e+4 > FN::T())
          *sec60 = false;
        else uiT_1m = FN::T();
      };
      void sendQueue() {
        bool sec60 = true;
        if (delayUI > 0) sendQueue(&sec60);
        if (!sec60) return;
        send(uiTXT::ApplicationState, serverState());
        orders60sec = 0;
      };
      json serverState() {
        time_t rawtime;
        time(&rawtime);
        return {
          {"memory", FN::memory()},
          {"hour", localtime(&rawtime)->tm_hour},
          {"freq", orders60sec},
          {"dbsize", ((DB*)memory)->size()},
          {"a", gw->A()}
        };
      };
  };
}

#endif
