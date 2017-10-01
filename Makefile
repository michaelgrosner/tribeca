K       ?= K.sh
KLIB     = 17c1ee4ae843d72fa94a29243a8b175be10cdda5
CROSS   ?= $(shell test -n "`command -v g++`" && g++ -dumpmachine || :)
KLOCAL   = build-$(CROSS)/local
CXX      = $(CROSS)-g++-6
CC       = $(CROSS)-gcc-6
KGIT     = 3.0
KHUB     = 6704776
V_ZLIB  := 1.2.11
V_SSL   := 1.1.0f
V_CURL  := 7.55.1
V_NCUR  := 6.0
V_JSON  := v2.1.1
V_UWS   := 0.14.4
V_SQL   := 3200100
V_QF    := v.1.14.4
KARGS    = -Wextra -std=c++11 -O3 -I$(KLOCAL)/include          \
  src/server/K.cc -pthread -rdynamic                           \
  -DK_STAMP='"$(shell date --rfc-3339=seconds | cut -f1 -d+)"' \
  -DK_BUILD='"$(CROSS)"'     $(KLOCAL)/include/uWS/*.cpp       \
  $(KLOCAL)/lib/K-$(CROSS).a $(KLOCAL)/lib/libquickfix.a       \
  $(KLOCAL)/lib/libsqlite3.a $(KLOCAL)/lib/libz.a              \
  $(KLOCAL)/lib/libcurl.a    $(KLOCAL)/lib/libssl.a            \
  $(KLOCAL)/lib/libcrypto.a  $(KLOCAL)/lib/libncurses.a -ldl

all: K

help:
	#                                                  #
	# Available commands inside K top level directory: #
	#  make help         - show this help              #
	#                                                  #
	#  make              - compile K sources           #
	#  make K            - compile K sources           #
	#  KALL=1 make K     - compile K sources           #
	#                                                  #
	#  make dist         - compile K dependencies      #
	#  KALL=1 make dist  - compile K dependencies      #
	#  make packages     - provide K dependencies      #
	#  make install      - install K application       #
	#  make docker       - install K application       #
	#  make reinstall    - upgrade K application       #
	#                                                  #
	#  make list         - show K instances            #
	#  make start        - start K instance            #
	#  make startall     - start K instances           #
	#  make stop         - stop K instance             #
	#  make stopall      - stop K instances            #
	#  make restart      - restart K instance          #
	#  make restartall   - restart K instances         #
	#                                                  #
	#  make diff         - show commits and versions   #
	#  make changelog    - show commits                #
	#  make latest       - show commits and reinstall  #
	#                                                  #
	#  make client       - compile K client src        #
	#  make www          - compile K client src        #
	#  make bundle       - compile K client bundle     #
	#                                                  #
	#  make test         - run tests                   #
	#  make test-cov     - run tests and coverage      #
	#  make send-cov     - send coverage               #
	#  make travis       - provide travis dev box      #
	#                                                  #
	#  make build        - download K src precompiled  #
	#  make zlib         - download zlib src files     #
	#  make curl         - download curl src files     #
	#  make ncurses      - download ncurses src files  #
	#  make sqlite       - download sqlite src files   #
	#  make openssl      - download openssl src files  #
	#  make json         - download json src files     #
	#  make uws          - download uws src files      #
	#  make quickfix     - download quickfix src files #
	#  make gdax         - download gdax ssl cert      #
	#  make clean        - remove external src files   #
	#  KALL=1 make clean - remove external src files   #
	#  make cleandb      - remove databases            #
	#                                                  #

K: src/server/K.cc
ifdef KALL
	unset KALL && CROSS=x86_64-linux-gnu $(MAKE) $@
	unset KALL && CROSS=arm-linux-gnueabihf $(MAKE) $@
	unset KALL && CROSS=aarch64-linux-gnu $(MAKE) $@
else
	@$(CXX) --version
	mkdir -p $(KLOCAL)/bin
	CROSS=$(CROSS) $(MAKE) $(shell uname -s)
	chmod +x $(KLOCAL)/bin/K-$(CROSS)
endif

dist:
ifdef KALL
	unset KALL && CROSS=x86_64-linux-gnu $(MAKE) $@
	unset KALL && CROSS=arm-linux-gnueabihf $(MAKE) $@
	unset KALL && CROSS=aarch64-linux-gnu $(MAKE) $@
else
	mkdir -p build-$(CROSS)
	CROSS=$(CROSS) $(MAKE) zlib openssl curl sqlite ncurses json uws quickfix
	test -f /sbin/ldconfig && sudo ldconfig || :
endif

Linux: build-$(CROSS)
	$(CXX) -o $(KLOCAL)/bin/K-$(CROSS) -static-libstdc++ -static-libgcc -g $(KARGS)

Darwin: build-$(CROSS)
	$(CXX) -o $(KLOCAL)/bin/K-$(CROSS) -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup $(KARGSG)

uws: build-$(CROSS)
	test -d build-$(CROSS)/uWebSockets-$(V_UWS)                                    \
	|| curl -L https://github.com/uNetworking/uWebSockets/archive/v$(V_UWS).tar.gz \
	| tar xz -C build-$(CROSS) && mkdir -p $(KLOCAL)/include/uWS                   \
	&& cp build-$(CROSS)/uWebSockets-$(V_UWS)/src/* $(KLOCAL)/include/uWS/

sqlite: build-$(CROSS)
	test -d build-$(CROSS)/sqlite-autoconf-$(V_SQL) || (                                            \
	curl -L https://sqlite.org/2017/sqlite-autoconf-$(V_SQL).tar.gz | tar xz -C build-$(CROSS)      \
	&& cd build-$(CROSS)/sqlite-autoconf-$(V_SQL) && CC=$(CC) ./configure --prefix=$(PWD)/$(KLOCAL) \
	--host=$(CROSS) --enable-static --disable-shared --enable-threadsafe && make && make install    )

zlib: build-$(CROSS)
	test -d build-$(CROSS)/zlib-$(V_ZLIB) || (                                 \
	curl -L https://zlib.net/zlib-$(V_ZLIB).tar.gz | tar xz -C build-$(CROSS)  \
	&& cd build-$(CROSS)/zlib-$(V_ZLIB) && CC=$(CC) CHOST=$(CROSS) ./configure \
	--prefix=$(PWD)/$(KLOCAL) && make && make install                          )

openssl: build-$(CROSS)
	test -d build-$(CROSS)/openssl-$(V_SSL) || (                                                            \
	curl -L https://www.openssl.org/source/openssl-$(V_SSL).tar.gz | tar xz -C build-$(CROSS)               \
	&& cd build-$(CROSS)/openssl-$(V_SSL) && CC=$(CC) ./Configure dist                                      \
	-fPIC --prefix=$(PWD)/$(KLOCAL) --openssldir=$(PWD)/$(KLOCAL) && make && make install_sw install_ssldirs)

curl: build-$(CROSS)
	test -d build-$(CROSS)/curl-$(V_CURL) || (                                                  \
	curl -L https://curl.haxx.se/download/curl-$(V_CURL).tar.gz | tar xz -C build-$(CROSS)      \
	&& cd build-$(CROSS)/curl-$(V_CURL) && CC=$(CC) ./configure                                 \
	--host=$(CROSS) --target=$(CROSS) --build=$(shell g++ -dumpmachine) --disable-manual        \
	--disable-shared --enable-static --prefix=$(PWD)/$(KLOCAL) --disable-ldap --without-libpsl  \
	--without-libssh2 --without-nghttp2 --disable-sspi --without-librtmp --disable-ftp          \
	--disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 \
	--disable-imap --disable-smtp --disable-gopher --disable-smb --without-libidn2              \
	--with-zlib=$(PWD)/$(KLOCAL) --with-ssl=$(PWD)/$(KLOCAL) && make && make install            )

ncurses:
	test -d build-$(CROSS)/ncurses-$(V_NCUR) || (                                                  \
	curl -L http://ftp.gnu.org/pub/gnu/ncurses/ncurses-$(V_NCUR).tar.gz | tar xz -C build-$(CROSS) \
	&& cd build-$(CROSS)/ncurses-$(V_NCUR) && CC=$(CC) CXX=$(CXX) CPPFLAGS=-P ./configure          \
	--host=$(CROSS) --prefix=$(PWD)/$(KLOCAL) && make && make install                              )

json: build-$(CROSS)
	test -f $(KLOCAL)/include/json.h || (mkdir -p $(KLOCAL)/include                  \
	&& curl -L https://github.com/nlohmann/json/releases/download/$(V_JSON)/json.hpp \
	-o $(KLOCAL)/include/json.h                                                      )

quickfix: build-$(CROSS)
	test -d build-$(CROSS)/quickfix-$(V_QF) || (                                                   \
	curl -L https://github.com/quickfix/quickfix/archive/$(V_QF).tar.gz | tar xz -C build-$(CROSS) \
	&& patch build-$(CROSS)/quickfix-$(V_QF)/m4/ax_lib_mysql.m4 < etc/without_mysql.m4.patch       \
	&& cd build-$(CROSS)/quickfix-$(V_QF) && ./bootstrap                                           \
	&& sed -i "s/bin spec test examples doc//" Makefile.am                                         \
	&& sed -i "s/CXX = g++/CXX \?= g++/" UnitTest++/Makefile                                       \
	&& CXX=$(CXX) ./configure --prefix=$(PWD)/$(KLOCAL) --enable-shared=no --enable-static=yes     \
	--host=$(CROSS) && cd UnitTest++ && CXX=$(CXX) make libUnitTest++.a                            \
	&& cd ../src && CXX=$(CXX) make && make install                                                )

build:
	mkdir -p $(KLOCAL)
	curl -L https://github.com/ctubio/Krypto-trading-bot/releases/download/$(KGIT)/$(KLIB)-$(CROSS).tar.gz \
	| tar xz -C $(KLOCAL) && chmod +x $(KLOCAL)/lib/K-$(CROSS).a $(KLOCAL)/bin/K-$(CROSS)

clean:
ifdef KALL
	unset KALL && CROSS=x86_64-linux-gnu $(MAKE) $@
	unset KALL && CROSS=arm-linux-gnueabihf $(MAKE) $@
	unset KALL && CROSS=aarch64-linux-gnu $(MAKE) $@
else
	rm -rf build-$(CROSS)
endif

cleandb: /data/db/K*
	rm -rf /data/db/K*.db

packages:
	test -n "`command -v apt-get`" && sudo apt-get -y install g++ build-essential automake autoconf libtool libxml2 libxml2-dev zlib1g-dev openssl stunnel python curl gzip imagemagick screen \
	|| (test -n "`command -v yum`" && sudo yum -y install gcc-c++ automake autoconf libtool libxml2 libxml2-devel openssl stunnel python curl gzip ImageMagick screen) \
	|| (test -n "`command -v brew`" && (xcode-select --install || :) && (brew install automake autoconf libxml2 sqlite openssl zlib libuv stunnel python curl gzip imagemagick || brew upgrade || :)) \
 	|| (test -n "`command -v pacman`" && sudo pacman --noconfirm -S --needed base-devel libxml2 zlib sqlite curl libcurl-compat openssl stunnel python gzip imagemagick screen)
	sudo mkdir -p /data/db/
	sudo chown $(shell id -u) /data/db
	$(MAKE) gdax -s
	test -n "`ls *.sh 2>/dev/null`" || (cp etc/K.sh.dist K.sh && chmod +x K.sh)

install:
	@$(MAKE) packages
	mkdir -p app/server
	@$(MAKE) build link

docker:
	@$(MAKE) packages
	mkdir -p app/server
	@$(MAKE) build link
	sed -i "/Usage/,+116d" K.sh

link:
	cd app && ln -f -s ../$(KLOCAL)/var/www client
	cd app/server && ln -f -s ../../$(KLOCAL)/bin/K-$(CROSS) K

reinstall: .git src
	rm -rf app
	git checkout .
	git fetch
	git merge FETCH_HEAD
	@rm -rf node_modules/hacktimer
	@$(MAKE) install
	#@$(MAKE) test -s
	@git checkout .
	@$(MAKE) restartall
	@echo && echo ..done! Please refresh the GUI if is currently opened in your browser.

list:
	@screen -list || :

restartall:
	@$(MAKE) stopall -s
	@sleep 3
	@$(MAKE) startall -s
	@$(MAKE) list -s

stopall:
	ls -1 *.sh | cut -d / -f2 | cut -d \* -f1 | grep -v ^_ | xargs -I % $(MAKE) K=% stop -s

startall:
	ls -1 *.sh | cut -d / -f2 | cut -d \* -f1 | grep -v ^_ | xargs -I % $(MAKE) K=% start -s
	@$(MAKE) list -s

restart:
	@$(MAKE) stop -s
	@sleep 3
	@$(MAKE) start -s
	@$(MAKE) list -s

stop:
	@screen -XS $(K) quit && echo STOP $(K) DONE || :

start:
	@test -d app || $(MAKE) install
	@test -n "`screen -list | grep "\.$(K)	("`"         \
	&& (echo $(K) is already running.. && screen -list)  \
	|| (screen -dmS $(K) ./$(K) && echo START $(K) DONE)

screen:
	@test -n "`screen -list | grep "\.$(K)	("`" && (    \
	echo Detach screen hotkey: holding CTRL hit A then D \
	&& sleep 2 && screen -r $(K)) || screen -list || :

gdax:
	openssl s_client -showcerts -connect fix.gdax.com:4198 < /dev/null \
	| openssl x509 -outform PEM > etc/sslcert/fix.gdax.com.pem

client: node_modules/.bin/tsc src/client
	rm -rf $(KLOCAL)/var
	mkdir -p $(KLOCAL)/var/www
	@echo Building client dynamic files..
	@npm install
	./node_modules/.bin/tsc --alwaysStrict --experimentalDecorators -t ES6 -m commonjs --outDir $(KLOCAL)/var/www/js src/client/*.ts
	@echo DONE

www: src/www $(KLOCAL)/var/www
	@echo Building client static files..
	cp -R src/www/* $(KLOCAL)/var/www/
	@echo DONE

bundle: client www node_modules/.bin/browserify node_modules/.bin/uglifyjs $(KLOCAL)/var/www/js/main.js
	@echo Building client bundle file..
	mkdir -p $(KLOCAL)/var/www/js/client
	./node_modules/.bin/browserify -t [ babelify --presets [ babili es2016 ] ] $(KLOCAL)/var/www/js/main.js $(KLOCAL)/var/www/js/lib/*.js | ./node_modules/.bin/uglifyjs | gzip > $(KLOCAL)/var/www/js/client/bundle.min.js
	rm $(KLOCAL)/var/www/js/*.js
	rm -rf build-arm-linux-gnueabihf/local/var
	cp -R $(KLOCAL)/var/* build-arm-linux-gnueabihf/local/var
	rm -rf build-aarch64-linux-gnu/local/var
	cp -R $(KLOCAL)/var/* build-aarch64-linux-gnu/local/var
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
	sudo apt-get install gcc-6
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-6 50
	sudo apt-get install g++-6
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 50
	npm install

png: etc/${PNG}.png etc/${PNG}.json
	convert etc/${PNG}.png -set "K.conf" "`cat etc/${PNG}.json`" K: etc/${PNG}.png 2>/dev/null || :
	@$(MAKE) png-check -s

png-check: etc/${PNG}.png
	@test -n "`identify -verbose etc/${PNG}.png | grep 'K\.conf'`" && echo Configuration injected into etc/${PNG}.png OK, feel free to remove etc/${PNG}.json anytime. || echo nope, injection failed.

check:
	@echo $(KLIB)
	@shasum $(KLOCAL)/bin/K-$(CROSS) | cut -d ' ' -f1

checkOK:
	@sed -i "s/^\(KLIB     = \).*$$/\1`shasum $(KLOCAL)/bin/K-$(CROSS) | cut -d ' ' -f1`/" Makefile
	@$(MAKE) check -s

release:
ifdef KALL
	unset KALL && CROSS=x86_64-linux-gnu $(MAKE) $@
	unset KALL && CROSS=arm-linux-gnueabihf $(MAKE) $@
	unset KALL && CROSS=aarch64-linux-gnu $(MAKE) $@
else
	@cd $(KLOCAL) && tar -cvzf $(KLIB)-$(CROSS).tar.gz bin/K-$(CROSS) var lib/K-$(CROSS).a                            \
	&& curl -s -n -H "Content-Type:application/octet-stream" -H "Authorization: token ${KRELEASE}"                    \
	--data-binary "@$(PWD)/$(KLOCAL)/$(KLIB)-$(CROSS).tar.gz"                                                         \
	"https://uploads.github.com/repos/ctubio/Krypto-trading-bot/releases/$(KHUB)/assets?name=$(KLIB)-$(CROSS).tar.gz" \
	&& rm $(KLIB)-$(CROSS).tar.gz && echo && echo DONE $(KLIB)-$(CROSS).tar.gz
endif

md5: src
	find src -type f -exec md5sum "{}" + > src.md5

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: K dist link Linux Darwin build zlib openssl curl ncurses quickfix uws json clean cleandb list screen start stop restart startall stopall restartall gdax packages install docker travis reinstall client www bundle diff latest changelog test test-cov send-cov png png-check md5 asandwich
