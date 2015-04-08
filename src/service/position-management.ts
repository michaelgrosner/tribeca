/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");
import util = require("util");
import _ = require("lodash");
import Persister = require("./persister");
import Agent = require("./arbagent");

class RegularFairValue {
    constructor(public time: Moment, public value: number) {}
}

export class RegularFairValuePersister {
    private _diffTime = moment.duration(10, 'minutes');

    constructor(private _persister: Persister.Persister<RegularFairValue>,
                private _fvAgent: Agent.FairValueEngine,
                initialValues: RegularFairValue[]) {
        if (!_.any(initialValues)) {
            this.startPersisting();
        }
        else {
            var timeout = _.last(initialValues).time.add(this._diffTime).diff(Utils.date());

            if (timeout > 0) {
                setTimeout(this.startPersisting, timeout)
            }
            else {
                this.startPersisting();
            }
        }
    }

    private persist = () => {
        var fv = this._fvAgent.latestFairValue;
        if (fv === null)
            return;
        this._persister.persist(new RegularFairValue(Utils.date(), fv.price));
    };

    private startPersisting = () => {
        this.persist();
        setInterval(this.persist, this._diffTime.asMilliseconds());
    };
}