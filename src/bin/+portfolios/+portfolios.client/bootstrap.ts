import 'zone.js';

import {bootstrapModule} from 'lib/shared';

import {ClientComponent} from './client';
import {WalletComponent} from './wallet';

bootstrapModule([
  ClientComponent,
  WalletComponent
]);
