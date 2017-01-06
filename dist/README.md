### tribeca-init.sh
To turn tribeca into a daemon, you may want to run the following command:
```
 $ cd /path/to/tribeca/dist
 $ sudo cp tribeca-init.sh /etc/init.d/tribeca
 $ sudo chmod +x /etc/init.d/tribeca
 $ sudo update-rc.d tribeca defaults
 $ sudo update-rc.d tribeca enable
```
Please make sure to correctly setup the value of `TRIBECA_SERVICE_PATH` variable hardcoded into the script.

### *-tribeca.json
Sample configuration files, must be located inside the compiled `service` folder, to initialize your configurations:
```
 $ cd /path/to/tribeca
 $ cp dist/prod-tribeca.json tribeca/service/tribeca.json
 $ vim tribeca/service/tribeca.json
```
Please note that inside the git clone folder `tribeca`, the compiler generates another subfolder also called `tribeca`, where the `service` folder is located.