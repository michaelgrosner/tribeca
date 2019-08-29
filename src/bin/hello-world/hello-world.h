class HelloWorld: public KryptoNinja {
  public:
    example::Engine engine;
  public:
    HelloWorld()
      : engine(*this)
    {
      autobot   = true;
      dustybot  = true;
      events    = {
        [&](const Levels &rawdata) { engine.read(rawdata); }
      };
      arguments = {
        {
          {"subject", "NAME", "World", "say hello to NAME (default: 'World')"}
        },
        [&](MutableUserArguments &args) {
          if (arg<string>("subject").empty())
            error("CF", "Invalid empty --subject value");
          else args["subject"] = Text::strU(arg<string>("subject")) + "!";
          log("CF", "arguments validated", "OK");
        }
      };
    };
} K;
