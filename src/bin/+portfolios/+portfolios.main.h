class Portfolios: public KryptoNinja {
  private:
    example::Engine engine;
  public:
    Portfolios()
      : engine(*this)
    {
      events    = {
        [&](const Wallets &rawdata) { engine.read(rawdata); }
      };
      // arguments = {
        // {
          // {"subject", "NAME", "World", "say hello to NAME (default: 'World')"}
        // },
        // [&](MutableUserArguments &args) {
          // if (arg<string>("subject").empty())
            // error("CF", "Invalid empty --subject value");
          // else args["subject"] = Text::strU(arg<string>("subject")) + "!";
          // log("CF", "arguments validated", "OK");
        // }
      // };
    };
} K;
