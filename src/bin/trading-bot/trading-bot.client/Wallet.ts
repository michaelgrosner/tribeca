import {Component, Input} from '@angular/core';

import {Models} from 'lib/K';

@Component({
  selector: 'wallet',
  template: `<div class="positions">
    <h4 class="col-md-12 col-xs-2">
      <small><i class="beacon-sym-_default-s beacon-sym-{{ product.base.toLowerCase() }}-s" ></i> {{ product.base }}:
        <br/>
        <span
          title="{{ product.base }} Available"
          class="text-danger"
        >{{ position.base.amount.toFixed(8) }}</span>
        <br/>
        <span
          title="{{ product.base }} Held"
          [ngClass]="position.base.held ? 'sell' : 'text-muted'"
        >{{ position.base.held.toFixed(8) }}</span>
        <hr style="margin:0 30px;color:#fff;opacity:.7">
        <span
          class="text-muted"
          title="{{ product.base}} Total"
        >{{ (position.base.amount + position.base.held).toFixed(8)}}</span>
      </small>
    </h4>
    <div *ngIf="!product.margin">
      <h4 class="col-md-12 col-xs-2">
        <small><i class="beacon-sym-_default-s beacon-sym-{{ product.quote.toLowerCase() }}-s" ></i> {{ product.quote }}:
          <br/>
          <span
            title="{{ product.quote }} Available"
            class="text-danger"
          >{{ position.quote.amount.toFixed(product.tickPrice) }}</span>
          <br/>
          <span
            title="{{ product.quote }} Held"
            [ngClass]="position.quote.held ? 'buy' : 'text-muted'"
          >{{ position.quote.held.toFixed(product.tickPrice) }}</span>
          <hr style="margin:0 30px;color:#fff;opacity:.7">
          <span
            class="text-muted"
            title="{{ product.quote }} Total"
          >{{ (position.quote.amount + position.quote.held).toFixed(product.tickPrice) }}</span>
        </small>
      </h4>
    </div>
    <h4 class="col-md-12 col-xs-2" style="margin-bottom: 0px!important;">
      <small>Value:</small>
      <br/>
      <b title="{{ product.base }} Total">{{ position.base.value.toFixed(8) }}<i
        class="beacon-sym-{{ product.base.toLowerCase() }}-s"></i></b>
      <br/>
      <b title="{{ product.quote }} Total">{{ position.quote.value.toFixed(product.tickPrice) }}<i
        class="beacon-sym-{{ product.quote.toLowerCase() }}-s"></i></b>
    </h4>
    <h4 class="col-md-12 col-xs-2" style="margin-top: 0px!important;">
      <small style="font-size:69%">
        <span
          title="{{ product.base }} profit %"
          class="{{ position.base.profit>0 ? \'text-danger\' : \'text-muted\' }}"
        >{{ position.base.profit>=0?'+':'' }}{{ position.base.profit.toFixed(2) }}%</span>
        <span *ngIf="!product.margin">, </span>
        <span *ngIf="!product.margin"
          title="{{ product.quote }} profit %"
          class="{{ position.quote.profit>0 ? \'text-danger\' : \'text-muted\' }}"
        >{{ position.quote.profit>=0?'+':'' }}{{ position.quote.profit.toFixed(2) }}%</span>
      </small>
    </h4>
  </div>`
})
export class WalletComponent {

  @Input() product: Models.ProductAdvertisement;

  @Input() position: Models.PositionReport;
};
