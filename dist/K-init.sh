#!/bin/bash
### BEGIN INIT INFO
# Provides: K.js
# Required-Start:    $local_fs $syslog $remote_fs mongod
# Required-Stop:     $local_fs $syslog $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start npm forever K.js daemon
### END INIT INFO

DAEMON_USER=user
DAEMON_TOPLEVEL_PATH=/home/user/path/to/K

cd $DAEMON_TOPLEVEL_PATH
case "$1" in
    start)
        su $DAEMON_USER -c "./node_modules/.bin/forever start -a -l /dev/null K.js"
        ;;
    stop)
        su $DAEMON_USER -c "./node_modules/.bin/forever stop -a -l /dev/null K.js"
        ;;
    list)
        su $DAEMON_USER -c "./node_modules/.bin/forever list"
        ;;
    reload|restart)
        su $DAEMON_USER -c "./node_modules/.bin/forever restart -a -l /dev/null K.js"
        ;;
    *)
        echo "Usage: /etc/init.d/K {start|stop|list|restart|reload}"
        exit 1
        ;;
esac
exit 0
