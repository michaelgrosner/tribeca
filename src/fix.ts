module Fix {
    var zeromq = require("zmq");

    export class FixGateway {
        ConnectChanged = new Evt<ConnectivityStatus>();

        sendEvent = (evt : string, obj : any) => {
            this._sock.send(JSON.stringify({evt: evt, obj: obj}));
        };

        _lastHeartbeatTime : Moment = null;
        _handlers : { [channel : string] : (newMsg : Timestamped<any>) => void} = {};
        _log : Logger = log("tribeca:gateway:FixBridge");
        _sock : any;
        constructor() {
            this._sock = new zeromq.socket("pair");
            this._sock.connect("tcp://localhost:5556");
            this.subscribe("ConnectionStatus", this.onConnectionStatus);
            this._sock.on("message", rawMsg => {
                var msg = JSON.parse(rawMsg);

                if (this._handlers.hasOwnProperty(msg.evt)) {
                    this._handlers[msg.evt](new Timestamped(msg.obj, new Date(msg.ts)));
                }
                else {
                    this._log("no handler registered for inbound FIX message: %o", msg);
                }
            });

            setInterval(this.checkMissedHeartbeats, 100);
        }

        private checkMissedHeartbeats = () => {
            if (this._lastHeartbeatTime == null) {
                this.ConnectChanged.trigger(ConnectivityStatus.Disconnected);
                return;
            }

            if (date().diff(this._lastHeartbeatTime) > 10000) {
                this.ConnectChanged.trigger(ConnectivityStatus.Disconnected);
            }
        };

        private onConnectionStatus = (tsMsg : Timestamped<string>) => {
            if (tsMsg.data == "Logon") {
                this._lastHeartbeatTime = date();
                this.ConnectChanged.trigger(ConnectivityStatus.Connected);
            }
            else if (tsMsg.data == "Logout") {
                this.ConnectChanged.trigger(ConnectivityStatus.Disconnected);
            }
            else {
                throw new Error(util.format("unknown connection status raised by FIX socket : %o", tsMsg));
            }
        };

        subscribe<T>(channel : string, handler: (newMsg : Timestamped<T>) => void) {
            this._handlers[channel] = handler;
        }
    }
}