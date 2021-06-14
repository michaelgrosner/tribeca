class Portfolios: public KryptoNinja {
  private:
    analpaper::Engine engine;
  public:
    Portfolios()
      : engine(*this)
    {
      events = {
        [&](const Wallet &rawdata) { engine.read(rawdata); }
      };
    };
} K;
