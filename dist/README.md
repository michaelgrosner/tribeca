### tribeca-init.sh
To turn tribeca into a daemon, you may want to run the following command:
```
 $ cd /path/to/tribeca/dist
 $ sudo cp tribeca-init.sh /etc/init.d/tribeca
 $ sudo chmod +x /etc/init.d/tribeca
 $ sudo update-rc.d tribeca defaults
 $ sudo update-rc.d tribeca enable
```
Please make sure to correctly setup the value of `DAEMON_USER` and `DAEMON_TOPLEVEL_PATH` variables hardcoded into the script.

Log messages will be saved in `log/tribeca.log` (log files in `~/.forever/` will be instead piped to `/dev/null`).

### Dockerfile
To run tribeca under winy or mac, make use of the [Dockerfile](https://raw.githubusercontent.com/ctubio/tribeca/master/dist/Dockerfile):

1. Please install [docker](https://www.docker.com/) for your system before preceeding. Requires at least Docker 1.7.1. Mac/Windows only: Ensure boot2docker or docker-machine is set up, depending on Docker version. See [the docs](https://docs.docker.com/installation/mac/) for more help.

2. Set up mongodb. If you do not have a mongodb instance already running: `docker run -p 27017:27017 --name tribeca-mongo -d mongo`.

3. Copy the repository [Dockerfile](https://raw.githubusercontent.com/ctubio/tribeca/master/dist/Dockerfile) into a text editor. Change the environment variables to match your desired [configuration](https://github.com/ctubio/tribeca/tree/master/etc#configuration-options). Input your exchange connectivity information, account information, and mongoDB credentials.

4. Save your new Dockerfile, preferably in a secure location and in an empty directory. Build the image from the Dockerfile `docker build -t tribeca .`

5. Run the container `docker run -p 3000:3000 --link tribeca-mongo:mongo --name tribeca -d tribeca`. If you run `docker ps`, you should see tribeca and mongo containers running.
