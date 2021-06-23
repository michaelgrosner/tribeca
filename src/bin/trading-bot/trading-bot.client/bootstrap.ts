import 'zone.js';

import {bootstrapModule} from 'lib/shared';

import {ClientComponent} from './client';
import {OptionComponent} from './option';
import {SubmitComponent} from './submit';
import {OrdersComponent} from './orders';
import {WalletComponent} from './wallet';
import {TradesComponent} from './trades';
import {MarketComponent} from './market';
import {SafetyComponent} from './safety';
import {TakersComponent} from './takers';
import {StateComponent}  from './state';
import {StatsComponent}  from './stats';

bootstrapModule([
  ClientComponent,
  OptionComponent,
  SubmitComponent,
  OrdersComponent,
  TradesComponent,
  WalletComponent,
  MarketComponent,
  SafetyComponent,
  TakersComponent,
  StateComponent,
  StatsComponent
]);
