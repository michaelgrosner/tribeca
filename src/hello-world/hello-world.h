#ifndef K_HELLO_WORLD_H_
#define K_HELLO_WORLD_H_

namespace K {
  struct Options: public Arguments {
    const vector<Argument> custom_long_options() const {
      return {
        {"subject", "NAME", "World", "say hello to NAME (default: 'World')"}
      };
    };
    void tidy_values(
      unordered_map<string, string> &str,
      unordered_map<string, int>    &num,
      unordered_map<string, double> &dec
    ) {
      if (str["subject"].empty())
        error("CF", "Invalid empty --subject value");
      else str["subject"] += "!";
    };
  } options;

  class HelloWorld: public Klass {
    public:
      void run() {
        const string result = greeting();
        const string prefix = "Executed " + (
          options.num("debug")
            ? string(__PRETTY_FUNCTION__)
            : options.str("title")
        );
        exit(prefix + ' ' + result);
      };
    private:
      const string greeting() {
        cout << "Hello, " << options.str("subject") << endl;
        return "OK";
      };
  } hello_world;
}

#endif
