import {Component, Input} from '@angular/core';

@Component({
  selector: 'assets',
  template: `<div *ngFor="let asset of assets; let i = index">
              <pre>{{ asset }}</pre>
            </div>`
})
export class AssetsComponent {

  private assets : string[];

  @Input() set setAsset(o: any[]) {
    if (o != null) for (let x in o) { this.assets.push(x + ": " + JSON.stringify(o[x])); }
    else this.assets = [];
    console.log(o);
  }
}
