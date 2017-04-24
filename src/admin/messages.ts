/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
///<reference path="shared_directives.ts"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

class MessageViewModel {
    text: string;
    time: moment.Moment;

    constructor(message: Models.Message) {
        this.time = (moment.isMoment(message.time) ? message.time : moment(message.time));
        this.text = message.text;
    }
}

interface MessageLoggerScope extends ng.IScope {
    messages: MessageViewModel[];
    messageOptions: Object;
}

var MessagesController = ($scope: MessageLoggerScope, $log: ng.ILogService, subscriberFactory: Shared.SubscriberFactory) => {
    $scope.messages = [];
    $scope.messageOptions = {
        data: 'messages',
        showGroupPanel: false,
        rowHeight: 20,
        headerRowHeight: 0,
        showHeader: false,
        groupsCollapsedByDefault: true,
        enableColumnResize: true,
        sortInfo: { fields: ['time'], directions: ['desc'] },
        columnDefs: [
            { width: 120, field: 'time', displayName: 't', cellFilter: 'momentFullDate' },
            { width: "*", field: 'text', displayName: 'text' }
        ]
    };

    var addNewMessage = (u: Models.Message) => {
        $scope.messages.push(new MessageViewModel(u));
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.Message)
        .registerSubscriber(addNewMessage, x => x.forEach(addNewMessage))
        .registerConnectHandler(() => $scope.messages.length = 0);

    $scope.$on('$destroy', () => {
        sub.disconnect();
        $log.info("destroy message grid");
    });

    $log.info("started message grid");
};

export var messagesDirective = "messagesDirective";

angular
    .module(messagesDirective, ['ui.bootstrap', 'ui.grid', Shared.sharedDirectives])
    .directive("messagesGrid", () => {
        var template = '<div><div style="height: 75px" class="table table-striped table-hover table-condensed" ui-grid="messageOptions"></div></div>';

        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            template: template,
            controller: MessagesController
        }
    });
