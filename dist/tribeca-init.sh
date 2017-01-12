#!/bin/bash
### BEGIN INIT INFO
# Provides: tribeca
# Required-Start:    $local_fs $syslog $remote_fs mongod
# Required-Stop:     $local_fs $syslog $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start tribeca daemons
### END INIT INFO

DAEMON_TOPLEVEL_PATH=/home/user/path/to/tribeca

DAEMON=tribeca.js
cd $TRIBECA_SERVICE_PATH
case "$1" in
    start)
        forever start -a -l /dev/null $DAEMON
        ;;
    stop)
        forever stop -a -l /dev/null $DAEMON
        ;;
    stopall)
        forever stopall
        ;;
    restartall)
        forever restartall
        ;;
    reload|restart)
        forever restart -a -l /dev/null $DAEMON
        ;;
    list)
        forever list
        ;;
    *)
        echo "Usage: /etc/init.d/tribeca {start|stop|restart|reload|stopall|restartall|list}"
        exit 1
        ;;
esac
exit 0
