#!/bin/bash
### BEGIN INIT INFO
# Provides: tribeca
# Required-Start:    $local_fs $syslog $remote_fs mongod
# Required-Stop:     $local_fs $syslog $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start npm forever tribeca daemon
### END INIT INFO

DAEMON_TOPLEVEL_PATH=/home/user/path/to/tribeca

cd $DAEMON_TOPLEVEL_PATH
case "$1" in
    start)
        npm start
        ;;
    stop)
        npm stop
        ;;
    reload|restart)
        npm restart
        ;;
    *)
        echo "Usage: /etc/init.d/tribeca {start|stop|restart|reload}"
        exit 1
        ;;
esac
exit 0
