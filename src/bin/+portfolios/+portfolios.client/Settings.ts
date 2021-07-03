import {Component, Input} from '@angular/core';

import {Socket, Models} from 'lib/K';

@Component({
  selector: 'settings',
  template: `<select class="form-control input-lg" id="base_select"
    style="width: 121px;margin-top: -1px;height: 26px;font-size: 19px;"
    title="base currency"
    [(ngModel)]="params.currency"
    (ngModelChange)="submitSettings()">
    <option [ngValue]="product.base">{{ product.base }}</option>
    <option [ngValue]="product.quote">{{ product.quote }}</option>
  </select>
  <input type="checkbox" id="zeroed_checkbox"
    title="show zeroed"
    style="zoom: 200%;margin-top:0px;"
    [(ngModel)]="params.zeroed"
    (ngModelChange)="submitSettings()">`
})
export class SettingsComponent {

  private params: Models.PortfolioParameters = JSON.parse(JSON.stringify(new Models.PortfolioParameters()));
  private pending: boolean = false;

  private fireCxl: Socket.IFire<Models.PortfolioParameters> = new Socket.Fire(Models.Topics.QuotingParametersChange);

  @Input() product: Models.ProductAdvertisement;

  @Input() set settings(o: Models.PortfolioParameters) {
    this.params = JSON.parse(JSON.stringify(o));
    this.pending = false;
    setTimeout(() => {window.dispatchEvent(new Event('resize'))}, 0);
  };

  private submitSettings = () => {
    this.pending = true;
    this.fireCxl.fire(this.params);
  };
};
