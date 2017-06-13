### K-init.sh
To turn K.js into a daemon, you may want to run the following command:
```
 $ cd /path/to/K/dist
 $ sudo cp K-init.sh /etc/init.d/K
 $ sudo chmod +x /etc/init.d/K
 $ sudo update-rc.d K defaults
 $ sudo update-rc.d K enable
```
Please make sure to correctly setup the value of `DAEMON_USER` and `DAEMON_TOPLEVEL_PATH` variables hardcoded into the script.

### Dockerfile
To run K.js under winy (or if you love Docker), make use of the [Dockerfile](https://raw.githubusercontent.com/ctubio/Krypto-trading-bot/master/dist/Dockerfile):

1. Please install [docker](https://www.docker.com/) for your system before proceeding. Requires at least Docker 1.7.1. Mac/Windows only: Ensure boot2docker or docker-machine is set up, depending on Docker version. See [the docs](https://docs.docker.com/installation/mac/) for more help.

2. Set up mongodb. If you do not have a mongodb instance already running: `docker run -p 27017:27017 --name K-mongo -d mongo`.

3. Copy the repository [Dockerfile](https://raw.githubusercontent.com/ctubio/Krypto-trading-bot/master/dist/Dockerfile) into a text editor. Change the environment variables to match your desired [configuration](https://github.com/ctubio/Krypto-trading-bot/tree/master/etc#configuration-options). Input your exchange connectivity information, account information, and mongoDB credentials.

4. Save your new Dockerfile, preferably in a secure location and in an empty directory. Build the image from the Dockerfile `docker build -t K .`

5. Run the container `docker run -p 3000:3000 --link K-mongo:mongo --name K -d K`. If you run `docker ps`, you should see K and mongo containers running.

### fix.gdax.com.conf
To run GDAX FIX API encrypted under SSL, this configuration file will be used to launch [stunnel](https://www.stunnel.org/index.html); no need to edit.