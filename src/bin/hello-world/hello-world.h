#ifndef K_HELLO_WORLD_H_
#define K_HELLO_WORLD_H_

class HelloWorld: public KryptoNinja {
  public:
    HelloWorld()
    {
      autobot   = true;
      arguments = { {
        {"subject", "NAME", "World", "say hello to NAME (default: 'World')"}
      }, [&](unordered_map<string, variant<string, int, double>> &args) {
        if (arg<string>("subject").empty())
          error("CF", "Invalid empty --subject value");
        else args["subject"] = Text::strU(arg<string>("subject")) + "!";
        log("CF", "arguments validated", "OK");
      } };
    };
  protected:
    void run() override {
      const string result = greeting();
      const string prefix = "Executed " + (
        arg<int>("debug")
          ? string(__PRETTY_FUNCTION__)
          : arg<string>("title")
      );
      exit(prefix + ' ' + result);
    };
  private:
    const string greeting() {
      cout << "Hello, " << arg<string>("subject") << endl;
      return "OK";
    };
} K;

#endif
