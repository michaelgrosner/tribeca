### Dockerfile
To run K.sh under winy (or if you love Docker), make use of the [Dockerfile](https://raw.githubusercontent.com/ctubio/Krypto-trading-bot/master/etc/Dockerfile):

1. Please install [docker](https://www.docker.com/) for your system before proceeding. Requires at least Docker 1.7.1. Mac/Windows only: Ensure boot2docker or docker-machine is set up, depending on Docker version. See [the docs](https://docs.docker.com/installation/mac/) for more help.

2. Copy the file [Dockerfile](https://raw.githubusercontent.com/ctubio/Krypto-trading-bot/master/etc/Dockerfile) into a text editor. Change the environment variables to match your desired [configuration](https://github.com/ctubio/Krypto-trading-bot/tree/master/etc#configuration-options). Input your exchange connectivity information and account information.

3. Save your new Dockerfile, preferably in a secure location and in an empty directory. Then build the images and run the containers:
```
 $ cd path/to/Dockerfile
 $ docker build --no-cache -t ksh .
 $ docker run -p 3000:3000 --name Ksh -d ksh
```
If you want to ensure that your data is persisted, mount a local folder into the container's `/data` folder:
```
$ docker run -p 3000:3000 -v /path/to/data:/data --name Ksh -d ksh
```

If you run `docker ps`, you should see K container running.

### K.sh.dist
Used on install to initialize `./K.sh` file, feel free to add your own hardcoded arguments to your own `./K.sh` file after install.

### K-stunnel.conf
To run GDAX FIX API encrypted under SSL, this configuration file will be used to launch [stunnel](https://www.stunnel.org/index.html); no need to edit.

### without_mysql.m4.patch
Used against libquickfix sources by `make dist`; no need to edit.
