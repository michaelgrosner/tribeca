import {NgZone} from '@angular/core';

import Models = require('./models');
import Subscribe = require('./subscribe');
import {FireFactory, SubscriberFactory} from './shared_directives';

class FormViewModel<T> {
  master: T;
  display: T;
  pending: boolean = false;
  connected: boolean = false;

  constructor(
    defaultParameter: T,
    private _sub: Subscribe.ISubscribe<T>,
    private _fire: Subscribe.IFire<T>,
    private _submitConverter: (disp: T) => T = null
  ) {
    if (this._submitConverter === null)
      this._submitConverter = d => d;

    _sub.registerSubscriber(this.update);
    this.connected = _sub.connected;
    this.master = JSON.parse(JSON.stringify(defaultParameter));
    this.display = JSON.parse(JSON.stringify(defaultParameter));
  }

  public reset = () => {
    this.display = JSON.parse(JSON.stringify(this.master));
  };

  public update = (p: T) => {
    this.master = JSON.parse(JSON.stringify(p));
    this.display = JSON.parse(JSON.stringify(p));
    this.pending = false;
  };

  public submit = () => {
    this.pending = true;
    this._fire.fire(this._submitConverter(this.display));
  };
}

class QuotingButtonViewModel extends FormViewModel<any> {
  constructor(
    sub: Subscribe.ISubscribe<any>,
    fire: Subscribe.IFire<any>
  ) {
    super({state:0}, sub, fire, d => {return {state:Math.abs(d.state-1)};});
  }

  public getClass = () => {
    if (this.pending) return "btn btn-warning";
    if (this.display.state) return "btn btn-success";
    return "btn btn-danger";
  }
}

class DisplayQuotingParameters extends FormViewModel<Models.QuotingParameters> {
  availableQuotingModes = [];
  availableFvModels = [];
  availableAutoPositionModes = [];
  availableAggressivePositionRebalancings = [];
  availableSuperTrades = [];
  availablePingAt = [];
  availablePongAt = [];
  availableSTDEV = [];

  constructor(sub: Subscribe.ISubscribe<Models.QuotingParameters>,
    fire: Subscribe.IFire<Models.QuotingParameters>) {
    super(<Models.QuotingParameters>{}, sub, fire);

    this.availableQuotingModes = DisplayQuotingParameters.getMapping(Models.QuotingMode);
    this.availableFvModels = DisplayQuotingParameters.getMapping(Models.FairValueModel);
    this.availableAutoPositionModes = DisplayQuotingParameters.getMapping(Models.AutoPositionMode);
    this.availableAggressivePositionRebalancings = DisplayQuotingParameters.getMapping(Models.APR);
    this.availableSuperTrades = DisplayQuotingParameters.getMapping(Models.SOP);
    this.availablePingAt = DisplayQuotingParameters.getMapping(Models.PingAt);
    this.availablePongAt = DisplayQuotingParameters.getMapping(Models.PongAt);
    this.availableSTDEV = DisplayQuotingParameters.getMapping(Models.STDEV);
  }

  private static getMapping<T>(enumObject: T) {
    let names = [];
    for (let mem in enumObject) {
      if (!enumObject.hasOwnProperty(mem)) continue;
      let val = parseInt(mem, 10);
      if (val >= 0) {
        let str = String(enumObject[mem]);
        if (str == 'AK47') str = 'AK-47';
        names.push({ 'str': str, 'val': val });
      }
    }
    return names;
  }
}

export class DisplayPair {
  name: string;
  connected: boolean = false;
  connectedToExchange = false;
  connectedToServer = false;
  connectionMessage : string = null;

  active: QuotingButtonViewModel;
  quotingParameters: DisplayQuotingParameters;

  constructor(
    public zone: NgZone,
    subscriberFactory: SubscriberFactory,
    fireFactory: FireFactory
  ) {
    this.connectedToServer = subscriberFactory
      .getSubscriber(zone, Models.Topics.ExchangeConnectivity)
      .registerSubscriber(this.setExchangeStatus)
      .registerConnectHandler(() => this.setServerStatus(true))
      .registerDisconnectedHandler(() => this.setServerStatus(false))
      .connected;

    this.setStatus();

    this.active = new QuotingButtonViewModel(
      subscriberFactory
        .getSubscriber(zone, Models.Topics.ActiveState),
      fireFactory
        .getFire(Models.Topics.ActiveState)
    );

    this.quotingParameters = new DisplayQuotingParameters(
      subscriberFactory
        .getSubscriber(zone, Models.Topics.QuotingParametersChange),
      fireFactory
        .getFire(Models.Topics.QuotingParametersChange)
    );
  }

  private setStatus = () => {
      this.connected = (this.connectedToExchange && this.connectedToServer);
      // console.log("connection status changed: ", this.connected, "connectedToExchange", this.connectedToExchange, "connectedToServer", this.connectedToServer);
      if (this.connected) {
          this.connectionMessage = null;
          return;
      }
      this.connectionMessage = '';
      if (!this.connectedToExchange)
          this.connectionMessage = 'Connecting to exchange..';
      if (!this.connectedToServer)
          this.connectionMessage = 'Disconnected from server';
  }

  private setExchangeStatus = (cs) => {
      this.connectedToExchange = cs.status == Models.Connectivity.Connected;
      this.setStatus();
  };

  private setServerStatus = (cs: boolean) => {
      this.connectedToServer = cs;
      this.setStatus();
  };
}
