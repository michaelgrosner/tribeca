import {Component, Input, OnInit} from '@angular/core';

import * as Models from '../../../www/ts/models';
import * as Socket from '../../../www/ts/socket';

@Component({
  selector: 'state-button',
  template: `<button
              style="font-size:16px;white-space:break-spaces;"
              class="col-md-12 col-xs-3"
              [ngClass]="getClass()"
              (click)="submitState()">{{ message ? message : product.exchange }}<br [hidden]="message" />{{ message ? "" : (product.margin == 2 ? product.symbol : product.base + '/' + product.quote) }}</button>`
})
export class StateComponent implements OnInit {

  public agree: number = 0;
  private pending: boolean = false;
  private connectedToExchange = false;
  private connectedToServer = false;
  private message : string = null;

  private fireCxl: Socket.IFire<any>;

  @Input() product: Models.ProductAdvertisement;

  @Input() set setExchangeStatus(cs: any) {
    this.agree = cs.agree;
    this.pending = false;
    this.connectedToExchange = cs.online == Models.Connectivity.Connected;
    this.setStatus();
  };

  @Input() set setServerStatus(cs: boolean) {
    this.connectedToServer = cs;
    this.setStatus();
  };

  ngOnInit() {
    this.fireCxl = new Socket.Fire(Models.Topics.Connectivity);

    this.setStatus();
  }

  public getClass = () => {
    if (this.pending) return "btn btn-warning";
    if (this.agree) return "btn btn-success";
    return "btn btn-danger";
  };

  private setStatus = () => {
    if (this.connectedToExchange && this.connectedToServer)
      this.message = null;
    if (!this.connectedToExchange)
      this.message = 'Connecting to exchange..';
    if (!this.connectedToServer)
      this.message = 'Disconnected from server';
  };

  private submitState = () => {
    this.pending = true;
    this.fireCxl.fire({
      agree: Math.abs(this.agree - 1)
    });
  };
}
