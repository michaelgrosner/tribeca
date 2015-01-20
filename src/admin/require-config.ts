/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="./client.ts" />

require.config({
    paths: {
        'jquery': '//ajax.googleapis.com/ajax/libs/jquery/1.11.0/jquery.min',
        'angular': '//ajax.googleapis.com/ajax/libs/angularjs/1.3.0/angular',
        'socket.io-client': '//cdn.socket.io/socket.io-1.1.0',
        'bootstrap': '/js/bootstrap.min',
        'moment': '//cdnjs.cloudflare.com/ajax/libs/moment.js/2.8.4/moment.min',
        'ui.bootstrap': '//cdnjs.cloudflare.com/ajax/libs/angular-ui-bootstrap/0.12.0/ui-bootstrap-tpls.min',
        'ngGrid': 'https://cdnjs.cloudflare.com/ajax/libs/ng-grid/2.0.11/ng-grid.min'
    },

    shim: {
        'angular': {
            exports: 'angular',
            deps: ['jquery']
        },
        'socket.io-client': ['jquery'],
        'bootstrap': ['jquery'],
        'ui.bootstrap': {
            deps: ['angular', 'bootstrap']
        },
        'ngGrid': {
            deps: ['angular', 'jquery']
        }
    }
});

require(["angular", 'ngGrid', "./client", 'ui.bootstrap', './orderlist', './exchange', './shared_directives', './pair'], () => {
    angular.bootstrap(document, ["projectApp"]);
});