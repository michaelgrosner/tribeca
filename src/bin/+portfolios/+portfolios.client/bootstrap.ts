import 'zone.js';

import {Shared} from 'lib/K';

import {ClientComponent} from './client';
import {WalletComponent} from './wallet';

Shared.bootstrapModule([
  ClientComponent,
  WalletComponent
]);
