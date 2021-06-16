import {Component, Inject, Input, OnInit} from '@angular/core';

import * as Models from '../../../www/ts/models';
import * as Socket from '../../../www/ts/socket';

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
                    <td><select id="selectSide" class="form-control input-sm" [(ngModel)]="side">
                      <option *ngFor="let option of availableSides" [ngValue]="option">{{option}}</option>
                    </select>
                    </td>
                    <td><input id="orderPriceInput" class="form-control input-sm" type="number" step="{{ product.stepPrice }}" min="{{ product.stepPrice }}"[(ngModel)]="price" /></td>
                    <td><input id="orderSizeInput" class="form-control input-sm" type="number" step="{{ product.stepSize }}" min="{{ product.minSize}}" [(ngModel)]="quantity" /></td>
                    <td><select class="form-control input-sm" [(ngModel)]="timeInForce">
                      <option *ngFor="let option of availableTifs" [ngValue]="option">{{option}}</option>
                    </select></td>
                    <td><select class="form-control input-sm" [(ngModel)]="type">
                      <option *ngFor="let option of availableOrderTypes" [ngValue]="option">{{option}}</option>
                    </select></td>
                    <td style="width:70px;" rowspan="2" class="text-center"><button type="button" class="btn btn-default" (click)="submitManualOrder()">Submit</button></td>
                </tr>
              </tbody>
            </table>`
})
export class SubmitComponent implements OnInit {

  private side : string;
  private price : number;
  private quantity : number;
  private timeInForce : string;
  private type : string;

  private availableSides : string[];
  private availableTifs : string[];
  private availableOrderTypes : string[];

  private fireCxl: Socket.IFire<Models.OrderRequestFromUI>;

  @Input() product: Models.ProductAdvertisement;

  constructor(
    @Inject(Socket.FireFactory) private fireFactory: Socket.FireFactory
  ) {}

  ngOnInit() {
    this.fireCxl = this.fireFactory.getFire(Models.Topics.SubmitNewOrder);

    this.availableSides = this.getNames(Models.Side).slice(0, 2);
    this.availableTifs = this.getNames(Models.TimeInForce);
    this.availableOrderTypes = this.getNames(Models.OrderType);
    this.side = this.availableSides[0];
    this.timeInForce = this.availableTifs[0];
    this.type = this.availableOrderTypes[0];
  }

  private getNames(enumObject: any) {
    var names: string[] = [];
    for (var mem in enumObject) {
      if (!enumObject.hasOwnProperty(mem)) continue;
      if (parseInt(mem, 10) >= 0) {
        names.push(String(enumObject[mem]));
      }
    }
    return names;
  }

  private submitManualOrder = () => {
    if (!this.side || !this.price || !this.quantity || !this.timeInForce || !this.type) return;
    this.fireCxl.fire(new Models.OrderRequestFromUI(this.side, this.price, this.quantity, this.timeInForce, this.type));
  };
}
