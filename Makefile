V_UWS := 0.14.3
V_JSO := v2.1.1
V_QF  := v.1.14.4
G_ARG := -std=c++11 -DUSE_LIBUV -O3 -shared -fPIC -Ibuild/node-$(NODEv)/include/node          \
  -Ibuild/uWebSockets-$(V_UWS)/src              build/uWebSockets-$(V_UWS)/src/Extensions.cpp \
  build/uWebSockets-$(V_UWS)/src/Group.cpp      build/uWebSockets-$(V_UWS)/src/Networking.cpp \
  build/uWebSockets-$(V_UWS)/src/Hub.cpp        build/uWebSockets-$(V_UWS)/src/Node.cpp       \
  build/uWebSockets-$(V_UWS)/src/WebSocket.cpp  build/uWebSockets-$(V_UWS)/src/HTTPSocket.cpp \
  build/uWebSockets-$(V_UWS)/src/Socket.cpp     build/uWebSockets-$(V_UWS)/src/Epoll.cpp      \
  -Ibuild/json-$(V_JSO)                                                                       \
src/lib/K.cc -lsqlite3 -lz -lpng -lcurl

all: K

help:
	#                                                  #
	# Available commands inside K top level directory: #
	#   make help       - show this help               #
	#   make changelog  - show remote commits          #
	#                                                  #
	#   make            - compile K node module        #
	#   make K          - compile K node module        #
	#                                                  #
	#   make packages   - install dependencies         #
	#   make config     - copy distributed config file #
	#   PNG=% make png  - inject config file into PNG  #
	#   make stunnel    - run ssl tunnel daemon        #
	#   make gdax       - download gdax ssl cert       #
	#                                                  #
	#   make diff       - show commits and versions    #
	#   make changelog  - show commits                 #
	#   make latest     - show commits and reinstall   #
	#   make reinstall  - reinstall from remote git    #
	#                                                  #
	#   make server     - compile K server src         #
	#   make client     - compile K client src         #
	#   make pub        - compile K client src         #
	#   make bundle     - compile K client bundle      #
	#                                                  #
	#   make test       - run tests                    #
	#   make test-cov   - run tests and coverage       #
	#   make send-cov   - send coverage                #
	#                                                  #
	#   make node       - download node src files      #
	#   make json       - download json src files      #
	#   make uws        - download uws src files       #
	#   make quickfix   - download quickfix src files  #
	#   make clean      - remove external src files    #
	#                                                  #

K: src/lib/K.cc
	@g++ --version
	mkdir -p build app/server/lib
	$(MAKE) quickfix
	$(MAKE) json
	$(MAKE) uws
	NODEv=v7.1.0 ABIv=51 $(MAKE) node `(uname -s)`
	NODEv=v8.1.2 ABIv=57 $(MAKE) node `(uname -s)`
	for K in app/server/lib/K*node; do chmod +x $$K; done

node: build
ifndef NODEv
	@NODEv=v7.1.0 $(MAKE) $@
	@NODEv=v8.1.2 $(MAKE) $@
else
	test -d build/node-$(NODEv) || curl https://nodejs.org/dist/$(NODEv)/node-$(NODEv)-headers.tar.gz | tar xz -C build
endif

uws: build
	test -d build/uWebSockets-$(V_UWS) || curl -L https://github.com/uNetworking/uWebSockets/archive/v$(V_UWS).tar.gz | tar xz -C build

json: build
	test -f build/json-$(V_JSO)/json.h || (mkdir -p build/json-v2.1.1 && curl -L https://github.com/nlohmann/json/releases/download/$(V_JSO)/json.hpp -o build/json-$(V_JSO)/json.h)

quickfix: build
	(test -f /usr/local/lib/libquickfix.so || test -f /usr/local/lib/libquickfix.dylib) || ( \
	curl -L https://github.com/quickfix/quickfix/archive/$(V_QF).tar.gz | tar xz -C build  \
	&& cd build/quickfix-$(V_QF) && ./bootstrap && ./configure && make                     \
	&& sudo make install && sudo cp config.h /usr/local/include/quickfix/                  \
	&& (test -f /sbin/ldconfig && sudo ldconfig || :)                                      )

Linux: build app/server/lib
ifdef ABIv
	g++ -o app/server/lib/K.linux.$(ABIv).node -static-libstdc++ -static-libgcc -s $(G_ARG)
endif

Darwin: build app/server/lib
ifdef ABIv
	g++ -o app/server/lib/K.darwin.$(ABIv).node -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup  $(G_ARG)
endif

clean: build
	rm -rf build

clean-db: /data/db/K*
	rm -rf /data/db/K*.db

config: etc/K.json.dist
	test -f etc/K.json && echo etc/K.json already exists || cp etc/K.json.dist etc/K.json

packages:
	test -n "`command -v apt-get`" && sudo apt-get -y install g++ build-essential automake autoconf libtool libxml2 libxml2-dev zlib1g-dev libsqlite3-dev libcurl4-openssl-dev libssl-dev libpng-dev openssl stunnel python gzip imagemagick \
	|| (test -n "`command -v yum`" && sudo yum -y install gcc-c++ automake autoconf libtool libxml2 libxml2-devel zlib-devel sqlite-devel libcurl-devel openssl openssl-devel zlib-devel stunnel python gzip libpng-devel imagemagick) \
	|| (test -n "`command -v brew`" && (xcode-select --install || :) && (brew install automake autoconf libxml2 sqlite openssl zlib libuv libpng stunnel python curl gzip imagemagick || brew upgrade || :))
	sudo mkdir -p /data/db/
	sudo chown `id -u` /data/db
	$(MAKE)
	$(MAKE) gdax -s
	@$(MAKE) stunnel -s

reinstall: .git src
	rm -rf app
	git fetch
	git merge FETCH_HEAD
	npm install
	@$(MAKE) test -s
	./node_modules/.bin/forever restartall
	@$(MAKE) stunnel -s
	@echo && echo ..done! Please refresh the GUI if is currently opened in your browser.

stunnel: dist/K-stunnel.conf
	test -z "${SKIP_STUNNEL}`ps axu | grep stunnel | grep -v grep`" && stunnel dist/K-stunnel.conf &

gdax:
	openssl s_client -showcerts -connect fix.gdax.com:4198 < /dev/null | openssl x509 -outform PEM > fix.gdax.com.pem
	sudo rm -rf /usr/local/etc/stunnel
	sudo mkdir -p /usr/local/etc/stunnel/
	sudo mv fix.gdax.com.pem /usr/local/etc/stunnel/

server: node_modules/.bin/tsc src/server src/share app
	@echo -n Building server files..
	@./node_modules/.bin/tsc --alwaysStrict -t ES6 -m commonjs --outDir app src/server/*.ts src/server/*/*.ts src/share/*.ts
	@echo " done"

client: node_modules/.bin/tsc src/client src/share app
	@echo -n Building client dynamic files..
	@./node_modules/.bin/tsc --alwaysStrict --experimentalDecorators -t ES6 -m commonjs --outDir app/pub/js src/client/*.ts src/share/*.ts
	@echo " done"

pub: src/pub app/pub
	@echo -n Building client static files..
	@cp -R src/pub/* app/pub/
	@echo " done"

bundle: node_modules/.bin/browserify node_modules/.bin/uglifyjs app/pub/js/client/main.js
	@echo -n Building client bundle file..
	@./node_modules/.bin/browserify -t [ babelify --presets [ babili es2016 ] ] app/pub/js/client/main.js app/pub/js/lib/*.js | ./node_modules/.bin/uglifyjs | gzip > app/pub/js/client/bundle.min.js
	@echo " done"

diff: .git
	@_() { echo $$2 $$3 version: `git rev-parse $$1`; }; git remote update && _ @ Local running && _ @{u} Latest remote
	@$(MAKE) changelog -s

latest: .git diff
	@_() { git rev-parse $$1; }; test `_ @` != `_ @{u}` && $(MAKE) reinstall -s || :

changelog: .git
	@_() { echo `git rev-parse $$1`; }; echo && git --no-pager log --graph --oneline @..@{u} && test `_ @` != `_ @{u}` || echo No need to upgrade because both versions are equal.

test: node_modules/.bin/mocha
	./node_modules/.bin/mocha --timeout 42000 --compilers ts:ts-node/register test/*.ts

test-cov: node_modules/.bin/ts-node node_modules/istanbul/lib/cli.js node_modules/.bin/_mocha
	./node_modules/.bin/ts-node ./node_modules/istanbul/lib/cli.js cover --report lcovonly --dir test/coverage -e .ts ./node_modules/.bin/_mocha -- --timeout 42000 test/*.ts

send-cov: node_modules/.bin/codacy-coverage node_modules/.bin/istanbul-coveralls
	cd test
	cat coverage/lcov.info | ./node_modules/.bin/codacy-coverage
	./node_modules/.bin/istanbul-coveralls

png: etc/${PNG}.png etc/${PNG}.json
	convert etc/${PNG}.png -set "K.conf" "`cat etc/${PNG}.json`" K: etc/${PNG}.png 2>/dev/null || :
	@$(MAKE) png-check -s

png-check: etc/${PNG}.png
	@test -n "`identify -verbose etc/${PNG}.png | grep 'K\.conf'`" && echo Configuration injected into etc/${PNG}.png OK, feel free to remove etc/${PNG}.json anytime. || echo nope, injection failed.

enc: dist/img/K.png build/K.msg
	convert dist/img/K.png -set "K.msg" "[`cat build/K.msg | gpg -e -r 0xFA101D1FC3B39DE0 -a`" K: dist/img/K.png 2>/dev/null || :

dec: dist/img/K.png build
	identify -verbose dist/img/K.png | sed 's/.*\[//;s/^ .*//g;1d;s/Version:.*//' | sed -e :a -e '/./,$$!d;/^\n*$$/{$$d;N;ba' -e '}' | gpg -d > build/K.msg

md5: src build
	find src -type f -exec md5sum "{}" + > build/K.md5

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: K quickfix uws json node Linux Darwin clean clean-db stunnel gdax config packages reinstall server client pub bundle diff latest changelog test test-cov send-cov png png-check enc dec md5 asandwich
