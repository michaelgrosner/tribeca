import Models = require("../share/models");
import moment = require('moment');

function roundUp(x: number, minTick: number) {
    return Math.ceil(x/minTick)*minTick;
}

export function roundDown(x: number, minTick: number) {
    return Math.floor(x/minTick)*minTick;
}

export function roundSide(x: number, minTick: number, side: Models.Side) {
    switch (side) {
        case Models.Side.Bid: return roundDown(x, minTick);
        case Models.Side.Ask: return roundUp(x, minTick);
        default: return roundNearest(x, minTick);
    }
}

export function roundNearest(x: number, minTick: number) {
    const up = roundUp(x, minTick);
    const down = roundDown(x, minTick);
    return (Math.abs(x - down) > Math.abs(up - x)) ? up : down;
}
/*
namespace K {
  void Round::Init(Local<Object> target) {
    Nan::Export(target, "roundUp", Round::RoundUp);
    Nan::Export(target, "roundDown", Round::RoundDown);
    Nan::Export(target, "roundNearest", Round::RoundNearest);
    Nan::Export(target, "roundSide", Round::RoundSide);
  }

  NAN_METHOD(Round::RoundUp) {
    double value = info[0]->NumberValue();
    double minTick = info[1]->NumberValue();

    v8::Local<v8::Number> num = Nan::New(ceil(value / minTick) * minTick);

    info.GetReturnValue().Set(num);
  }

  NAN_METHOD(Round::RoundDown) {
    double value = info[0]->NumberValue();
    double minTick = info[1]->NumberValue();

    v8::Local<v8::Number> num = Nan::New(floor(value / minTick) * minTick);

    info.GetReturnValue().Set(num);
  }

  NAN_METHOD(Round::RoundNearest) {
    double value = info[0]->NumberValue();
    double minTick = info[1]->NumberValue();

    v8::Local<v8::Number> num = Nan::New(round(value / minTick) * minTick);

    info.GetReturnValue().Set(num);
  }

  NAN_METHOD(Round::RoundSide) {
    int side = info[2]->NumberValue();
    switch (side) {
      case 0: Round::RoundDown(info); break;
      case 1: Round::RoundUp(info); break;
      default: Round::RoundNearest(info); break;
    }
  }
}
*/