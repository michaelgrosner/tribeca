import Models = require("../share/models");

export class Publisher {
  public publish = (topic: string, msg: any, monitor?: boolean) => {
    if (monitor) this.delay(topic, msg);
    else this._socket.up(topic, msg);
  };

  constructor(
    private _dbSize,
    private _socket,
    private _evOn,
    private _delayUI,
    public registerSnapshot,
    public registerReceiver
  ) {
    this.setTick();
    this._evOn('QuotingParameters', (qp) => {
      this._delayUI = qp.delayUI;
      this.setTick();
    });

    this.registerSnapshot(Models.Topics.ApplicationState, () => [this._app_state]);

    this.registerSnapshot(Models.Topics.Notepad, () => [this._notepad]);

    this.registerSnapshot(Models.Topics.ToggleConfigs, () => [this._toggleConfigs]);

    this.registerReceiver(Models.Topics.Notepad, (notepad: string) => {
      this._notepad = notepad;
    });

    this.registerReceiver(Models.Topics.ToggleConfigs, (toggleConfigs: boolean) => {
      this._toggleConfigs = toggleConfigs;
    });
  }

  private _delayed: any[] = [];
  private _app_state: Models.ApplicationState;
  private _notepad: string;
  private _toggleConfigs: boolean = true;
  private _newOrderMinute: number = 0;
  private _tick: number = 0;
  private _interval = null;

  private onTick = () => {
    this._tick = 0;
    this._app_state = new Models.ApplicationState(
      process.memoryUsage().rss,
      (new Date()).getHours(),
      this._newOrderMinute / 2,
      this._dbSize()
    );
    this._newOrderMinute = 0;
    this.publish(Models.Topics.ApplicationState, this._app_state);
  };

  private onDelay = () => {
    this._tick += this._delayUI;
    if (this._tick>=6e1) this.onTick();
    let orders: any[] = this._delayed.filter(x => x[0]===Models.Topics.OrderStatusReports);
    this._delayed = this._delayed.filter(x => x[0]!==Models.Topics.OrderStatusReports);
    if (orders.length) this._delayed.push([Models.Topics.OrderStatusReports, orders.map(x => x[1])]);
    this._delayed.forEach(x => this._socket.up(x[0], x[1]));
    this._delayed = orders.filter(x => x[1].orderStatus===Models.OrderStatus.Working);
  };

  private delay = (topic: string, msg: any) => {
    const isOSR: boolean = topic === Models.Topics.OrderStatusReports;
    if (isOSR && msg.orderStatus === Models.OrderStatus.New) return ++this._newOrderMinute;
    if (!this._delayUI) return this._socket.up(topic, isOSR?[msg]:msg);
    this._delayed = this._delayed.filter(x => x[0] !== topic || (isOSR?x[1].orderId !== msg.orderId:false));
    this._delayed.push([topic, msg]);
  };

  private setTick = () => {
    if (this._interval) clearInterval(this._interval);
    if (this._delayUI<1) this._delayUI = 0;
    this._delayed = [];
    this._interval = setInterval(
      this._delayUI ? this.onDelay : this.onTick,
      (this._delayUI || 6e1) * 1e3
    );
    this.onTick();
  };
}
