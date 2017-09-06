KCONFIG ?= K
V_CURL  := 7.55.1
V_SSL   := 1.1.0f
V_UWS   := 0.14.3
V_PNG   := 1.6.31
V_PSL   := 0.18.0
V_JSON  := v2.1.1
V_HTTP  := 1.25.0
V_IDN   := 2.0.4
V_ICU   := 59_1
V_SQL   := 3200100
V_SSH   := 1.8.0
V_QF    := v.1.14.4
G_ARG   := -Wextra -std=c++11 -O3               -Wl,-rpath,'$$ORIGIN'                             \
  -Ibuild/curl-$(V_CURL)/include/curl           -Lbuild/curl-$(V_CURL)/lib/.libs                  \
  -Ibuild/openssl-$(V_SSL)/include              -Lbuild/openssl-$(V_SSL)                          \
  -Ibuild/json-$(V_JSON)                        -Ibuild/sqlite-autoconf-$(V_SQL)                  \
src/server/K.cc -pthread -ldl -lssl -lcrypto -lz -lcurl                                           \
  -Ibuild/uWebSockets-$(V_UWS)/src              build/uWebSockets-$(V_UWS)/src/Extensions.cpp     \
  build/uWebSockets-$(V_UWS)/src/Group.cpp      build/uWebSockets-$(V_UWS)/src/Networking.cpp     \
  build/uWebSockets-$(V_UWS)/src/Hub.cpp        build/uWebSockets-$(V_UWS)/src/Node.cpp           \
  build/uWebSockets-$(V_UWS)/src/WebSocket.cpp  build/uWebSockets-$(V_UWS)/src/HTTPSocket.cpp     \
  build/uWebSockets-$(V_UWS)/src/Socket.cpp     build/uWebSockets-$(V_UWS)/src/Epoll.cpp          \
  build/libpng-$(V_PNG)/lib/libpng16.a          build/sqlite-autoconf-$(V_SQL)/.libs/libsqlite3.a \
  dist/lib/K-`uname -m`.a                       build/quickfix-$(V_QF)/lib/libquickfix.a

all: K

help:
	#                                                  #
	# Available commands inside K top level directory: #
	#   make help       - show this help               #
	#                                                  #
	#   make            - compile K node module        #
	#   make K          - compile K node module        #
	#                                                  #
	#   make packages   - provide K application        #
	#   make install    - install K application        #
	#   make docker     - install K application        #
	#   make reinstall  - upgrade K application        #
	#                                                  #
	#   make list       - show K instances             #
	#   make start      - start K instance             #
	#   make startall   - start K instances            #
	#   make stop       - stop K instance              #
	#   make stopall    - stop K instances             #
	#   make restart    - restart K instance           #
	#   make restartall - restart K instances          #
	#                                                  #
	#   make diff       - show commits and versions    #
	#   make changelog  - show commits                 #
	#   make latest     - show commits and reinstall   #
	#                                                  #
	#   make config     - copy distributed config file #
	#   PNG=% make png  - inject config file into PNG  #
	#   make stunnel    - run ssl tunnel daemon        #
	#   make gdax       - download gdax ssl cert       #
	#   make cleandb    - remove databases             #
	#                                                  #
	#   make client     - compile K client src         #
	#   make pub        - compile K client src         #
	#   make bundle     - compile K client bundle      #
	#                                                  #
	#   make test       - run tests                    #
	#   make test-cov   - run tests and coverage       #
	#   make send-cov   - send coverage                #
	#   make travis     - provide travis dev box       #
	#                                                  #
	#   make curl       - download curl src files      #
	#   make sqlite     - download sqlite src files    #
	#   make openssl    - download openssl src files   #
	#   make ssh2       - download ssh2 src files      #
	#   make idn2       - download idn2 src files      #
	#   make nghttp2    - download nghttp2 src files   #
	#   make psl        - download psl src files       #
	#   make json       - download json src files      #
	#   make png16      - download png16 src files     #
	#   make uws        - download uws src files       #
	#   make quickfix   - download quickfix src files  #
	#   make clean      - remove external src files    #
	#                                                  #

K: src/server/K.cc
	@g++ --version
	$(MAKE) `(uname -s)`
	chmod +x dist/lib/K-`uname -m`

uws: build
	test -d build/uWebSockets-$(V_UWS) || curl -L https://github.com/uNetworking/uWebSockets/archive/v$(V_UWS).tar.gz | tar xz -C build

curl: build
	test -d build/curl-$(V_CURL) || (curl -L https://curl.haxx.se/download/curl-$(V_CURL).tar.gz | tar xz -C build && cd build/curl-$(V_CURL) && ./configure --enable-shared --disable-static --prefix=/tmp/curl --disable-ldap --disable-sspi --without-librtmp --disable-ftp --disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-smb --without-libidn2 --with-ssl=$(PWD)/build/openssl-$(V_SSL) && make)

sqlite: build
	test -d build/sqlite-$(V_SQL) || (curl -L https://sqlite.org/2017/sqlite-autoconf-$(V_SQL).tar.gz | tar xz -C build && cd build/sqlite-$(V_SQL) && ./configure --enable-static --disable-shared && make)

psl: build
	test -d build/libpsl-$(V_PSL) || (curl -L https://github.com/rockdaboot/libpsl/releases/download/libpsl-0.18.0/libpsl-$(V_PSL).tar.gz | tar xz -C build && cd build/libpsl-$(V_PSL) && ./configure --enable-runtime=libicu --enable-builtin=libicu && make)

idn: build
	test -d build/libidn2-$(V_IDN) || (curl -L https://gitlab.com/libidn/libidn2/uploads/15020b7769a7c69cebd38654ddf0648a/libidn2-$(V_IDN).tar.gz | tar xz -C build && cd build/libidn2-$(V_IDN) && ./configure && make)

ssh2: build
	test -d build/libssh2-$(V_SSH) || (curl -L https://www.libssh2.org/download/libssh2-$(V_SSH).tar.gz | tar xz -C build && cd build/libssh2-$(V_SSH) && ./configure --disable-shared --with-libssl-prefix=$(PWD)/build/openssl-$(V_SSL) && make)

icu: build
	test -d build/icu || (curl -L http://download.icu-project.org/files/icu4c/59.1/icu4c-$(V_ICU)-src.tgz | tar xz -C build && cd build/icu/source && ./configure --disable-shared --enable-static && make)

nghttp2: build
	test -d build/nghttp2-$(V_HTTP) || (curl -L https://github.com/nghttp2/nghttp2/releases/download/v1.25.0/nghttp2-$(V_HTTP).tar.gz | tar xz -C build && cd build/nghttp2-$(V_HTTP) && ./configure && make)

openssl: build
	test -d build/openssl-$(V_SSL) || (curl -L https://www.openssl.org/source/openssl-$(V_SSL).tar.gz | tar xz -C build && cd build/openssl-$(V_SSL) && ./config -fPIC --prefix=/usr/local --openssldir=/usr/local/ssl && make && sudo make install)

json: build
	test -f build/json-$(V_JSON)/json.h || (mkdir -p build/json-v2.1.1 && curl -L https://github.com/nlohmann/json/releases/download/$(V_JSON)/json.hpp -o build/json-$(V_JSON)/json.h)

png16:
	test -d build/libpng-$(V_PNG) || (curl -L https://github.com/glennrp/libpng/archive/v$(V_PNG).tar.gz | tar xz -C build && cd build/libpng-$(V_PNG) && ./autogen.sh && ./configure && make && sudo make install)

quickfix: build
	test -d build/quickfix-$(V_QF) || ( \
	curl -L https://github.com/quickfix/quickfix/archive/$(V_QF).tar.gz | tar xz -C build    \
	&& patch build/quickfix-$(V_QF)/m4/ax_lib_mysql.m4 < dist/lib/without_mysql.m4.patch     \
	&& cd build/quickfix-$(V_QF) && ./bootstrap                                              \
	&& ./configure --enable-shared=no --enable-static=yes && make                            \
	&& sudo make install && sudo cp config.h /usr/local/include/quickfix/                    )

Linux: build
	g++-6 -o dist/lib/K-`uname -m` -static-libstdc++ -static-libgcc -s $(G_ARG)

Darwin: build
	g++-6 -o dist/lib/K-`uname -m` -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup $(G_ARG)

dist:
	mkdir -p build app/server
	$(MAKE) openssl sqlite curl uws quickfix json png16
	test -f /sbin/ldconfig && sudo ldconfig || :
	cd app/server && ln -f -s ../../dist/lib/K-`(uname -m)` K
	# for K in dist/lib/*; do chmod +x $$K && cp $$K app/server; done

clean: build
	rm -rf build

cleandb: /data/db/K*
	rm -rf /data/db/K*.db

config: etc/K.json.dist
	@test -f etc/K.json && echo etc/K.json already exists || cp etc/K.json.dist etc/K.json && echo DONE

packages:
	test -n "`command -v apt-get`" && sudo apt-get -y install g++ build-essential automake autoconf libtool libxml2 libxml2-dev zlib1g-dev libsqlite3-dev libcurl4-openssl-dev openssl stunnel python curl gzip imagemagick\
	|| (test -n "`command -v yum`" && sudo yum -y install gcc-c++ automake autoconf libtool libxml2 libxml2-devel zlib-devel sqlite-devel libcurl-devel openssl zlib-devel stunnel python curl gzip ImageMagick) \
	|| (test -n "`command -v brew`" && (xcode-select --install || :) && (brew install automake autoconf libxml2 sqlite openssl zlib libuv stunnel python curl gzip imagemagick || brew upgrade || :)) \
 	|| (test -n "`command -v pacman`" && sudo pacman --noconfirm -S --needed base-devel libxml2 zlib sqlite curl libcurl-compat openssl stunnel python gzip imagemagick)
	sudo mkdir -p /data/db/
	sudo chown `id -u` /data/db
	$(MAKE) gdax -s

install:
	@$(MAKE) packages
	@npm install
	@$(MAKE) dist -s
	@$(MAKE) client pub bundle

docker:
	@$(MAKE) packages
	@npm install --unsafe-perm
	@$(MAKE) client pub bundle

reinstall: .git src
	rm -rf app
	git checkout .
	git fetch
	git merge FETCH_HEAD
	@rm -rf node_modules/hacktimer
	@$(MAKE) install
	@$(MAKE) test -s
	@git checkout .
	./node_modules/.bin/forever restartall
	@echo && echo ..done! Please refresh the GUI if is currently opened in your browser.

list:
	./node_modules/.bin/forever list

restartall:
	./node_modules/.bin/forever restartall

stopall:
	./node_modules/.bin/forever stopall

startall:
	ls -1 etc/*.json etc/*.png | cut -d / -f2 | cut -d . -f1 | grep -v ^_ | xargs -I % $(MAKE) KCONFIG=% start -s
	$(MAKE) list -s

restart:
	$(MAKE) stop -s
	$(MAKE) start -s
	$(MAKE) list -s

stop:
	./node_modules/.bin/forever stop -a -l /dev/null "$(KCONFIG)" || :

start:
	@test -d app || $(MAKE) install
	./node_modules/.bin/forever start --minUptime 1 --spinSleepTime 21000 --uid "$(KCONFIG)" -a -l /dev/null -c /bin/sh K.sh

stunnel: dist/K-stunnel.conf
	test -z "`ps axu | grep stunnel | grep -v grep`" && stunnel dist/K-stunnel.conf &

gdax:
	openssl s_client -showcerts -connect fix.gdax.com:4198 < /dev/null | openssl x509 -outform PEM > fix.gdax.com.pem
	sudo rm -rf /usr/local/etc/stunnel
	sudo mkdir -p /usr/local/etc/stunnel/
	sudo mv fix.gdax.com.pem /usr/local/etc/stunnel/

client: node_modules/.bin/tsc src/client
	mkdir -p app
	@echo Building client dynamic files..
	./node_modules/.bin/tsc --alwaysStrict --experimentalDecorators -t ES6 -m commonjs --outDir app/pub/js src/client/*.ts
	@echo DONE

pub: src/pub app/pub
	@echo Building client static files..
	cp -R src/pub/* app/pub/
	mkdir -p app/pub/js/client
	@echo DONE

bundle: node_modules/.bin/browserify node_modules/.bin/uglifyjs app/pub/js/main.js
	@echo Building client bundle file..
	./node_modules/.bin/browserify -t [ babelify --presets [ babili es2016 ] ] app/pub/js/main.js app/pub/js/lib/*.js | ./node_modules/.bin/uglifyjs | gzip > app/pub/js/client/bundle.min.js
	rm app/pub/js/*.js
	@echo DONE

diff: .git
	@_() { echo $$2 $$3 version: `git rev-parse $$1`; }; git remote update && _ @ Local running && _ @{u} Latest remote
	@$(MAKE) changelog -s

latest: .git diff
	@_() { git rev-parse $$1; }; test `_ @` != `_ @{u}` && $(MAKE) reinstall || :

changelog: .git
	@_() { echo `git rev-parse $$1`; }; echo && git --no-pager log --graph --oneline @..@{u} && test `_ @` != `_ @{u}` || echo No need to upgrade, both versions are equal.

test: node_modules/.bin/mocha
	./node_modules/.bin/mocha --timeout 42000 --compilers ts:ts-node/register test/*.ts

test-cov: node_modules/.bin/ts-node node_modules/istanbul/lib/cli.js node_modules/.bin/_mocha
	./node_modules/.bin/ts-node ./node_modules/istanbul/lib/cli.js cover --report lcovonly --dir test/coverage -e .ts ./node_modules/.bin/_mocha -- --timeout 42000 test/*.ts

send-cov: node_modules/.bin/codacy-coverage node_modules/.bin/istanbul-coveralls
	cd test && cat coverage/lcov.info | ./node_modules/.bin/codacy-coverage && ./node_modules/.bin/istanbul-coveralls

travis:
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt-get install gcc-4.9
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 50
	sudo apt-get install g++-4.9
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 50

png: etc/${PNG}.png etc/${PNG}.json
	convert etc/${PNG}.png -set "K.conf" "`cat etc/${PNG}.json`" K: etc/${PNG}.png 2>/dev/null || :
	@$(MAKE) png-check -s

png-check: etc/${PNG}.png
	@test -n "`identify -verbose etc/${PNG}.png | grep 'K\.conf'`" && echo Configuration injected into etc/${PNG}.png OK, feel free to remove etc/${PNG}.json anytime. || echo nope, injection failed.

md5: src build
	find src -type f -exec md5sum "{}" + > build/K.md5

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: K quickfix uws json curl openssl Linux Darwin dist clean cleandb list start stop restart startall stopall restartall stunnel gdax config packages install docker travis reinstall client pub bundle diff latest changelog test test-cov send-cov png png-check enc dec md5 asandwich
