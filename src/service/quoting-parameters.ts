/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
///<reference path="interfaces.ts"/>

import Models = require("../common/models");
import Interfaces = require("./interfaces");
import Messaging = require("../common/messaging");
import _ = require("lodash");

export class QuotingParametersRepository extends Interfaces.Repository<Models.QuotingParameters> {
    constructor(pub: Messaging.IPublish<Models.QuotingParameters>,
        rec: Messaging.IReceive<Models.QuotingParameters>,
        initParam: Models.QuotingParameters) {
        super("qpr",
            (p: Models.QuotingParameters) => p.size > 0 || p.width > 0,
            (a: Models.QuotingParameters, b: Models.QuotingParameters) => !_.isEqual(a, b),
            initParam, rec, pub);

    }
}