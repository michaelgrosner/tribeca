import {Component, Input} from '@angular/core';

import {Socket, Models} from 'lib/K';

@Component({
  selector: 'state-button',
  template: `<button
    style="font-size:16px;white-space:break-spaces;"
    class="col-md-12 col-xs-3"
    [ngClass]="getClass()"
    (click)="toggle()">{{
      state.online ? product.exchange : 'Connecting to exchange..'
    }}<span [hidden]="!state.online"><br />{{
      product.margin == 2 ? product.symbol : product.base + '/' + product.quote
    }}</span>
  </button>`
})
export class StateComponent {

  private fireCxl: Socket.IFire<Models.AgreeRequestFromUI> = new Socket.Fire(Models.Topics.Connectivity);

  @Input() product: Models.ProductAdvertisement;

  @Input() state: Models.ExchangeState;

  private getClass = () => {
    if (this.state.agree === null) return "btn btn-warning";
    else if (this.state.agree)     return "btn btn-success";
    else                           return "btn btn-danger";
  };

  private toggle = () => {
    this.fireCxl.fire(new Models.AgreeRequestFromUI(Math.abs(this.state.agree - 1)));
    this.state.agree = null;
  };
};
