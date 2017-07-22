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
src/lib/K.cc -lsqlite3 -lcurl

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
	#   make config     - initialize config file       #
	#   make cleandb    - remove K database files      #
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
		&& (test -f /sbin/ldconfig && sudo ldconfig || :)                                      \
	)

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

cleandb: /data/db/K*
	rm -rf /data/db/K*.db

config: etc/K.json.dist
	test -f etc/K.json && echo etc/K.json already exists || cp etc/K.json.dist etc/K.json

server: node_modules/.bin/tsc src/server src/share app
	./node_modules/.bin/tsc --alwaysStrict -t ES6 -m commonjs --outDir app src/server/*.ts src/server/*/*.ts src/share/*.ts

client: node_modules/.bin/tsc src/client src/share app
	./node_modules/.bin/tsc --alwaysStrict --experimentalDecorators -t ES6 -m commonjs --outDir app/pub/js src/client/*.ts src/share/*.ts

pub: src/pub app/pub
	cp -R src/pub/* app/pub/

bundle: node_modules/.bin/browserify node_modules/.bin/uglifyjs app/pub/js/client/main.js
	./node_modules/.bin/browserify -t [ babelify --presets [ babili es2016 ] ] app/pub/js/client/main.js app/pub/js/lib/*.js | ./node_modules/.bin/uglifyjs | gzip > app/pub/js/client/bundle.min.js

changelog: .git
	git --no-pager log --graph --oneline @..@{u}

test: node_modules/.bin/mocha
	./node_modules/.bin/mocha --timeout 42000 --compilers ts:ts-node/register test/*.ts

test-cov: node_modules/.bin/ts-node node_modules/istanbul/lib/cli.js node_modules/.bin/_mocha
	./node_modules/.bin/ts-node ./node_modules/istanbul/lib/cli.js cover --report lcovonly --dir test/coverage -e .ts ./node_modules/.bin/_mocha -- --timeout 42000 test/*.ts

send-cov: node_modules/.bin/codacy-coverage node_modules/.bin/istanbul-coveralls
	cd test && cat coverage/lcov.info | ./node_modules/.bin/codacy-coverage && ./node_modules/.bin/istanbul-coveralls

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: K quickfix uws json node Linux Darwin clean cleandb config server client pub bundle changelog test test-cov send-cov asandwich
