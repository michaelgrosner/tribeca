/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="./models.ts" />
/// <reference path="./client.ts" />
require.config({
    paths: {
        'jquery': '//ajax.googleapis.com/ajax/libs/jquery/1.11.0/jquery.min',
        'angular': '//ajax.googleapis.com/ajax/libs/angularjs/1.3.0/angular',
        'socket.io-client': '//cdn.socket.io/socket.io-1.1.0',
        'bootstrap': '//netdna.bootstrapcdn.com/bootstrap/3.1.1/js/bootstrap.min',
        'moment': '//cdnjs.cloudflare.com/ajax/libs/moment.js/2.8.4/moment.min',
        'ui.bootstrap': '//cdnjs.cloudflare.com/ajax/libs/angular-ui-bootstrap/0.10.0/ui-bootstrap-tpls.min'
    },
    shim: {
        'angular': {
            exports: 'angular',
            deps: ['jquery']
        },
        'ui.bootstrap': ['angular'],
        'socket.io-client': ['jquery'],
        'bootstrap': ['jquery']
    }
});
require(["angular", "./client", 'ui.bootstrap'], function () {
    console.log("bootstrapping projectApp");
    angular.bootstrap(document, ["projectApp"]);
});
