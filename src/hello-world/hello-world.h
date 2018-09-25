#ifndef K_HELLO_WORLD_H_
#define K_HELLO_WORLD_H_

namespace K {
  struct Options: public Arguments {
    void default_notempty_values() {
      optstr["subject"] = "World";
    };
    void tidy_values() {
      if (optstr["subject"].empty())
        error("CF", "Invalid empty --subject value.");
      else optstr["subject"] += "!";
    };
    const vector<option> custom_long_options() {
      return {
        {"subject", required_argument, 0, 9999}
      };
    };
    const string custom_help(const function<string()> &stamp) {
      return stamp() + "    --subject=NAME        - say hello to NAME (default: 'World')." + '\n';
    };
  } options;

  const string greeting() {
    cout << "Hello, " << args->optstr["subject"] << endl;
    return "OK";
  };

  void hello_world(const string &file) {
    exit(file + " executed " + greeting() + '.');
  };
}

#endif
