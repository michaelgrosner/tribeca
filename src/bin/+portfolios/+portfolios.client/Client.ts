import {Component, OnInit, Input} from '@angular/core';

import {Socket, Models} from 'lib/K';

@Component({
  selector: 'client',
  template: `<div class="row">
      <div class="col-md-12 col-xs-12">
          <div class="row">
            <h4>SUM: {{ balance }}<i class="beacon-sym-{{ settings.currency.toLowerCase() }}-s"></i></h4>
            <settings
              [product]="product"
              [settings]="settings"></settings>
          </div>
          <div class="row">
            <wallet
              [asset]="asset"
              [settings]="settings"
              (onBalance)="onBalance($event)"></wallet>
          </div>
      </div>
  </div>`
})
export class ClientComponent implements OnInit {

  private asset: any = null;
  private balance: string = "0";

  private settings: Models.PortfolioParameters = new Models.PortfolioParameters();

  @Input() addr: string;

  @Input() tradeFreq: number;

  @Input() state: Models.ExchangeState;

  @Input() product: Models.ProductAdvertisement;

  ngOnInit() {
    new Socket.Subscriber(Models.Topics.QuotingParametersChange)
      .registerSubscriber((o: Models.PortfolioParameters) => { this.settings = o; });

    new Socket.Subscriber(Models.Topics.Position)
      .registerSubscriber((o: any[]) => { this.asset = o; })
      .registerDisconnectedHandler(() => { this.asset = null; });
  };

  private onBalance(o: number) {
    this.balance = o.toFixed(8);
  };
};
