import {NgZone} from '@angular/core';

import _ = require('lodash');

import Models = require('../share/models');
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
    this.master = _.cloneDeep(defaultParameter);
    this.display = _.cloneDeep(defaultParameter);
  }

  public reset = () => {
    this.display = _.cloneDeep(this.master);
  };

  public update = (p: T) => {
    // console.log("updating parameters", p);
    this.master = _.cloneDeep(p);
    this.display = _.cloneDeep(p);
    this.pending = false;
  };

  public submit = () => {
    this.pending = true;
    this._fire.fire(this._submitConverter(this.display));
  };
}

class QuotingButtonViewModel extends FormViewModel<boolean> {
  constructor(sub: Subscribe.ISubscribe<boolean>,
    fire: Subscribe.IFire<boolean>) {
    super(false, sub, fire, d => !d);
  }

  public getClass = () => {
    if (this.pending) return "btn btn-warning";
    if (this.display) return "btn btn-success";
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

  constructor(sub: Subscribe.ISubscribe<Models.QuotingParameters>,
    fire: Subscribe.IFire<Models.QuotingParameters>) {
    super(new Models.QuotingParameters(null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null), sub, fire);

    this.availableQuotingModes = DisplayQuotingParameters.getMapping(Models.QuotingMode);
    this.availableFvModels = DisplayQuotingParameters.getMapping(Models.FairValueModel);
    this.availableAutoPositionModes = DisplayQuotingParameters.getMapping(Models.AutoPositionMode);
    this.availableAggressivePositionRebalancings = DisplayQuotingParameters.getMapping(Models.APR);
    this.availableSuperTrades = DisplayQuotingParameters.getMapping(Models.SOP);
    this.availablePingAt = DisplayQuotingParameters.getMapping(Models.PingAt);
    this.availablePongAt = DisplayQuotingParameters.getMapping(Models.PongAt);
  }

  private static getMapping<T>(enumObject: T) {
    var names = [];
    for (var mem in enumObject) {
      if (!enumObject.hasOwnProperty(mem)) continue;
      var val = parseInt(mem, 10);
      if (val >= 0) {
        var str = String(enumObject[mem]);
        if (str == 'AK47') str = 'AK-47';
        names.push({ 'str': str, 'val': val });
      }
    }
    return names;
  }
}

export class DisplayPair {
  name: string;
  connected: boolean;

  active: QuotingButtonViewModel;
  quotingParameters: DisplayQuotingParameters;

  constructor(
    public zone: NgZone,
    subscriberFactory: SubscriberFactory,
    fireFactory: FireFactory
  ) {
    subscriberFactory
      .getSubscriber(zone, Models.Topics.ExchangeConnectivity)
      .registerSubscriber(this.setConnectStatus);

    this.active = new QuotingButtonViewModel(
      subscriberFactory
        .getSubscriber(zone, Models.Topics.ActiveChange),
      fireFactory
        .getFire(Models.Topics.ActiveChange)
    );

    this.quotingParameters = new DisplayQuotingParameters(
      subscriberFactory
        .getSubscriber(zone, Models.Topics.QuotingParametersChange),
      fireFactory
        .getFire(Models.Topics.QuotingParametersChange)
    );
  }

  public setConnectStatus = (cs: Models.ConnectivityStatus) => {
      this.connected = cs == Models.ConnectivityStatus.Connected;
  };

  public updateParameters = (p: Models.QuotingParameters) => {
    this.quotingParameters.update(p);
  };
}
