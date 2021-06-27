import {Component, OnInit, Input} from '@angular/core';

import {Socket, Models} from 'lib/K';

@Component({
  selector: 'client',
  template: `<div class="row">
      <div class="col-md-12 col-xs-12">
          <div class="row">
            <wallet
              [asset]="asset"></wallet>
          </div>
      </div>
  </div>`
})
export class ClientComponent implements OnInit {

  private asset: any = null;

  @Input() addr: string;

  @Input() tradeFreq: number;

  @Input() state: Models.ExchangeState;

  @Input() product: Models.ProductAdvertisement;

  ngOnInit() {
    new Socket.Subscriber(Models.Topics.Position)
      .registerSubscriber((o: any[]) => { this.asset = o; })
      .registerDisconnectedHandler(() => { this.asset = null; });
  };
};
