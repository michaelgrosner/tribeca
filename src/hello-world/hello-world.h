#ifndef K_HELLO_WORLD_H_
#define K_HELLO_WORLD_H_

namespace K {
  struct Options: public Arguments {
    const vector<Argument> custom_long_options() {
      return {
        {"subject", "NAME", "World", "say hello to NAME (default: 'World')"}
      };
    };
    void tidy_values() {
      if (optstr["subject"].empty())
        error("CF", "Invalid empty --subject value");
      else optstr["subject"] += "!";
    };
  } options;

  const string greeting() {
    cout << "Hello, " << options.optstr["subject"] << endl;
    return "OK";
  };

  void hello_world(const string &file) {
    exit(file + " executed " + greeting() + '.');
  };
}

#endif
