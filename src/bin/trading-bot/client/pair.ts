import {NgZone} from '@angular/core';

import * as Models from './models';
import * as Subscribe from './subscribe';
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

    if (_sub !== null) {
      _sub.registerSubscriber(this.update);
      this.connected = _sub.connected;
    }
    this.master = JSON.parse(JSON.stringify(defaultParameter));
    this.display = JSON.parse(JSON.stringify(defaultParameter));
  }

  public cleanDecimal = (p: T) => {
    return p;
  }

  public reset = () => {
    this.display = JSON.parse(JSON.stringify(this.master));
  };

  public update = (p: T) => {
    p = this.cleanDecimal(p);
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
    super({agree:0}, sub, fire, d => {return {agree:Math.abs(d.agree-1)};});
  }

  public getClass = () => {
    if (this.pending) return "btn btn-warning";
    if (this.display.agree) return "btn btn-success";
    return "btn btn-danger";
  }
}

class DisplayQuotingParameters extends FormViewModel<Models.QuotingParameters> {
  availableQuotingModes = [];
  availableOrderPctTotals = [];
  availableQuotingSafeties = [];
  availableFvModels = [];
  availableAutoPositionModes = [];
  availablePositionDivergenceModes = [];
  availableAggressivePositionRebalancings = [];
  availableSuperTrades = [];
  availablePingAt = [];
  availablePongAt = [];
  availableSTDEV = [];

  constructor(sub: Subscribe.ISubscribe<Models.QuotingParameters>,
    fire: Subscribe.IFire<Models.QuotingParameters>) {
    super(<Models.QuotingParameters>{}, sub, fire, (d: any) => {
      d.widthPing = parseFloat(d.widthPing);
      d.widthPong = parseFloat(d.widthPong);
      d.buySize = parseFloat(d.buySize);
      d.sellSize = parseFloat(d.sellSize);
      return d;
    });

    this.availableQuotingModes = DisplayQuotingParameters.getMapping(Models.QuotingMode);
    this.availableOrderPctTotals = DisplayQuotingParameters.getMapping(Models.OrderPctTotal);
    this.availableQuotingSafeties = DisplayQuotingParameters.getMapping(Models.QuotingSafety);
    this.availableFvModels = DisplayQuotingParameters.getMapping(Models.FairValueModel);
    this.availableAutoPositionModes = DisplayQuotingParameters.getMapping(Models.AutoPositionMode);
    this.availableAggressivePositionRebalancings = DisplayQuotingParameters.getMapping(Models.APR);
    this.availablePositionDivergenceModes = DisplayQuotingParameters.getMapping(Models.DynamicPDivMode);
    this.availableSuperTrades = DisplayQuotingParameters.getMapping(Models.SOP);
    this.availablePingAt = DisplayQuotingParameters.getMapping(Models.PingAt);
    this.availablePongAt = DisplayQuotingParameters.getMapping(Models.PongAt);
    this.availableSTDEV = DisplayQuotingParameters.getMapping(Models.STDEV);
  }

  public cleanDecimal = (d: any) => {
    d.widthPing = parseFloat(d.widthPing).toFixed(13).replace(/0+$/,'').replace(/\.$/,'');
    d.widthPong = parseFloat(d.widthPong).toFixed(13).replace(/0+$/,'').replace(/\.$/,'');
    d.buySize = parseFloat(d.buySize).toFixed(13).replace(/0+$/,'').replace(/\.$/,'');
    d.sellSize = parseFloat(d.sellSize).toFixed(13).replace(/0+$/,'').replace(/\.$/,'');
    return d;
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

  public backup = () => {
    try{
      this.display = JSON.parse(window.prompt('Backup quoting parameters\n\nTo export the current setup, copy all from the input below and paste it somewhere else.\n\nTo import a setup replacement, replace the quoting parameters below by some new, then click [Save] to apply.\n\nCurrent/New setup of quoting parameters:', JSON.stringify(this.display))) || this.display;
    }catch(e){}
  };
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
    this.active = new QuotingButtonViewModel(
      null,
      fireFactory
        .getFire(Models.Topics.Connectivity)
    );

    this.connectedToServer = subscriberFactory
      .getSubscriber(zone, Models.Topics.Connectivity)
      .registerSubscriber(this.setExchangeStatus)
      .registerConnectHandler(() => this.setServerStatus(true))
      .registerDisconnectedHandler(() => this.setServerStatus(false))
      .connected;

    this.setStatus();

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
      this.active.update(cs);
      this.connectedToExchange = cs.online == Models.Connectivity.Connected;
      this.setStatus();
  };

  private setServerStatus = (cs: boolean) => {
      this.connectedToServer = cs;
      this.setStatus();
  };
}
