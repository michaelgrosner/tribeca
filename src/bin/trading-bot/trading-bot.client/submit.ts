import {Component, Input} from '@angular/core';

import {Socket, Models} from 'lib/K';

@Component({
  selector: 'submit-order',
  template: `<table class="table table-responsive">
    <thead>
      <tr>
        <th style="width:100px;">Side:</th>
        <th style="width:100px;">Price:</th>
        <th style="width:100px;">Size:</th>
        <th style="width:100px;">TIF:</th>
        <th style="width:100px;">Type:</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>
          <select id="selectSide" class="form-control input-sm" [(ngModel)]="side">
            <option
              *ngFor="let option of availableSides"
              [ngValue]="option.val"
            >{{ option.str }}</option>
          </select>
        </td>
        <td>
          <input
            id="orderPriceInput"
            class="form-control input-sm"
            type="number"
            step="{{ product.stepPrice }}"
            min="{{ product.stepPrice }}"
            [(ngModel)]="price" />
        </td>
        <td>
          <input
            id="orderSizeInput"
            class="form-control input-sm"
            type="number"
            step="{{ product.stepSize }}"
            min="{{ product.minSize}}"
            [(ngModel)]="quantity" />
        </td>
        <td>
          <select class="form-control input-sm" [(ngModel)]="timeInForce">
            <option
              *ngFor="let option of availableTifs"
              [ngValue]="option.val"
            >{{ option.str }}</option>
          </select>
        </td>
        <td>
          <select class="form-control input-sm" [(ngModel)]="type">
            <option
              *ngFor="let option of availableOrderTypes"
              [ngValue]="option.val"
            >{{ option.str }}</option>
          </select>
        </td>
        <td style="width:70px;" rowspan="2" class="text-center">
          <button
            type="button"
            class="btn btn-default"
            (click)="submitManualOrder()"
          >Submit</button>
        </td>
      </tr>
    </tbody>
  </table>`
})
export class SubmitComponent {

  private side: Models.Side = Models.Side.Bid;
  private price: number;
  private quantity: number;
  private timeInForce: Models.TimeInForce = Models.TimeInForce.GTC;
  private type: Models.OrderType = Models.OrderType.Limit;

  private availableSides:      Models.Map[] = Models.getMap(Models.Side);
  private availableTifs:       Models.Map[] = Models.getMap(Models.TimeInForce);
  private availableOrderTypes: Models.Map[] = Models.getMap(Models.OrderType);

  private fireCxl: Socket.IFire<Models.OrderRequestFromUI> = new Socket.Fire(Models.Topics.SubmitNewOrder);

  @Input() product: Models.ProductAdvertisement;

  private submitManualOrder = () => {
    if (this.price && this.quantity)
      this.fireCxl.fire(new Models.OrderRequestFromUI(
        this.side, this.price, this.quantity, this.timeInForce, this.type
      ));
  };
};
