module Fix {
    var zeromq = require("zmq");

    export class FixGateway {
        sendEvent = (evt : string, obj : any) => {
            this._sock.send(JSON.stringify({evt: evt, obj: obj}));
        };

        _log : Logger = log("tribeca:gateway:FixBridge");
        _sock : any;
        constructor() {
            this._sock = new zeromq.socket("pair");
            this._sock.connect("tcp://localhost:5556");
            this._sock.on("message", rawMsg => {
                var msg = JSON.parse(rawMsg);

                if (this._handlers.hasOwnProperty(msg.evt)) {
                    this._handlers[msg.evt](new Timestamped(msg.obj, new Date(msg.ts)));
                }
                else {
                    this._log("no handler registered for inbound FIX message: %o", msg);
                }
            });
        }

        _handlers : { [channel : string] : (newMsg : Timestamped<any>) => void} = {};

        subscribe<T>(channel : string, handler: (newMsg : Timestamped<T>) => void) {
            this._handlers[channel] = handler;
        }
    }
}