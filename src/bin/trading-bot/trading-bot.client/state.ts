import {Component, Input, OnInit} from '@angular/core';

import * as Models from 'lib/models';
import * as Socket from 'lib/socket';

@Component({
  selector: 'state-button',
  template: `<button
              style="font-size:16px;white-space:break-spaces;"
              class="col-md-12 col-xs-3"
              [ngClass]="getClass()"
              (click)="submitState()">{{ message ? message : product.exchange }}<br [hidden]="message" />{{ message ? "" : (product.margin == 2 ? product.symbol : product.base + '/' + product.quote) }}</button>`
})
export class StateComponent implements OnInit {

  private connected = false;
  private agree: number = 0;
  private pending: boolean = false;
  private message : string = null;

  private fireCxl: Socket.IFire<Models.AgreeRequestFromUI>;

  @Input() product: Models.ProductAdvertisement;

  @Input() set setExchangeStatus(cs: Models.ExchangeStatus) {
    this.pending   = false;
    this.agree     = cs.agree;
    this.connected = cs.online == Models.Connectivity.Connected;
    this.setMessage();
  };

  ngOnInit() {
    this.fireCxl = new Socket.Fire(Models.Topics.Connectivity);

    this.setMessage();
  }

  private getClass = () => {
    if      (this.pending) return "btn btn-warning";
    else if (this.agree)   return "btn btn-success";
    else                   return "btn btn-danger";
  };

  private setMessage = () => {
    this.message = this.connected ? '' : 'Connecting to exchange..';
  };

  private submitState = () => {
    this.pending = true;
    this.fireCxl.fire(new Models.AgreeRequestFromUI(Math.abs(this.agree - 1)));
  };
}
