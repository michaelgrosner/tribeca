K       ?= K.sh
CHOST   ?= $(shell (test -d .git && test -n "`command -v g++`") && g++ -dumpmachine || ls . | grep build- | head -n1 | cut -d '/' -f1 | cut -d '-' -f2-)
CARCH    = x86_64-linux-gnu arm-linux-gnueabihf aarch64-linux-gnu x86_64-apple-darwin17
KLOCAL   = build-$(CHOST)/local
CXX      = $(CHOST)-g++
CC       = $(CHOST)-gcc
ERR      = *** K require g++ v6, but g++ v6 was not found at $(shell which $(CXX))
HINT     = consider to create a symlink at $(shell which $(CXX)) pointing to your g++-6 executable
KGIT     = 4.0
KHUB     = 8656597
V_ZLIB  := 1.2.11
V_SSL   := 1.1.0g
V_CURL  := 7.57.0
V_NCUR  := 6.0
V_JSON  := v3.0.0
V_UWS   := 0.14.4
V_SQL   := 3210000
V_QF    := v.1.14.4
V_UV    := 1.18.0
V_PVS   := 6.20.24121.1823
KZIP     = 9449f5d6da123e3f92d0ec21d845601098489d96
KARGS    = -Wextra -std=c++11 -O3 -I$(KLOCAL)/include      \
  src/server/K.cxx -pthread -rdynamic                      \
  -DK_STAMP='"$(shell date "+%Y-%m-%d %H:%M:%S")"'         \
  -DK_BUILD='"$(CHOST)"'     $(KLOCAL)/include/uWS/*.cpp   \
  $(KLOCAL)/lib/K-$(CHOST).a $(KLOCAL)/lib/libquickfix.a   \
  $(KLOCAL)/lib/libsqlite3.a $(KLOCAL)/lib/libz.a          \
  $(KLOCAL)/lib/libcurl.a    $(KLOCAL)/lib/libssl.a        \
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
	#  make pvs          - download pvs src files      #
	#  make zlib         - download zlib src files     #
	#  make curl         - download curl src files     #
	#  make ncurses      - download ncurses src files  #
	#  make sqlite       - download sqlite src files   #
	#  make openssl      - download openssl src files  #
	#  make json         - download json src files     #
	#  make uws          - download uws src files      #
	#  make quickfix     - download quickfix src files #
	#  make gdax         - download gdax ssl cert      #
	#  make cabundle     - download ssl CA certs       #
	#  make clean        - remove external src files   #
	#  KALL=1 make clean - remove external src files   #
	#  make cleandb      - remove databases            #
	#                                                  #

K: src/server/K.cxx
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	@$(if $(shell sh -c 'test "`g++ -dumpversion | cut -d . -f1`" != "6" || echo 1'),,$(warning $(ERR));$(error $(HINT)))
	@$(CXX) --version
	mkdir -p $(KLOCAL)/bin
	CHOST=$(CHOST) $(MAKE) $(shell test -n "`echo $(CHOST) | grep darwin`" && echo Darwin || uname -s)
	chmod +x $(KLOCAL)/bin/K-$(CHOST)
endif

dist:
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	@$(if $(shell sh -c 'test "`g++ -dumpversion | cut -d . -f1`" != "6" || echo 1'),,$(warning $(ERR));$(error $(HINT)))
	mkdir -p build-$(CHOST)
	CHOST=$(CHOST) $(MAKE) zlib openssl curl sqlite ncurses json uws quickfix libuv
	test -f /sbin/ldconfig && sudo ldconfig || :
endif

Linux: build-$(CHOST)
	$(CXX) -o $(KLOCAL)/bin/K-$(CHOST) -DUWS_THREADSAFE -static-libstdc++ -static-libgcc -g $(KARGS)

Darwin: build-$(CHOST)
	$(CXX) -o $(KLOCAL)/bin/K-$(CHOST) -DUSE_LIBUV $(KLOCAL)/lib/libuv.a -msse4.1 -maes -mpclmul -mmacosx-version-min=10.13 -nostartfiles $(KARGS)

zlib: build-$(CHOST)
	test -d build-$(CHOST)/zlib-$(V_ZLIB) || (                                 \
	curl -L https://zlib.net/zlib-$(V_ZLIB).tar.gz | tar xz -C build-$(CHOST)  \
	&& cd build-$(CHOST)/zlib-$(V_ZLIB) && CC=$(CC) ./configure --static       \
	--prefix=$(PWD)/$(KLOCAL) && make && make install                          )

openssl: build-$(CHOST)
	test -d build-$(CHOST)/openssl-$(V_SSL) || (                                                             \
	curl -L https://www.openssl.org/source/openssl-$(V_SSL).tar.gz | tar xz -C build-$(CHOST)                \
	&& cd build-$(CHOST)/openssl-$(V_SSL) && CC=$(CC) RANLIB=$(CHOST)-ranlib AR=$(CHOST)-ar ./Configure dist \
	-fPIC --prefix=$(PWD)/$(KLOCAL) --openssldir=$(PWD)/$(KLOCAL) && make && make install_sw install_ssldirs )

curl: build-$(CHOST)
	test -d build-$(CHOST)/curl-$(V_CURL) || (                                                  \
	curl -L https://curl.haxx.se/download/curl-$(V_CURL).tar.gz | tar xz -C build-$(CHOST)      \
	&& cd build-$(CHOST)/curl-$(V_CURL) && CC=$(CC) ./configure --with-ca-path=/etc/ssl/certs   \
	--host=$(CHOST) --target=$(CHOST) --build=$(shell g++ -dumpmachine) --disable-manual        \
	--disable-shared --enable-static --prefix=$(PWD)/$(KLOCAL) --disable-ldap --without-libpsl  \
	--without-libssh2 --without-nghttp2 --disable-sspi --without-librtmp --disable-ftp          \
	--disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 \
	--disable-imap --disable-smtp --disable-gopher --disable-smb --without-libidn2              \
	--with-zlib=$(PWD)/$(KLOCAL) --with-ssl=$(PWD)/$(KLOCAL) && make && make install            )

sqlite: build-$(CHOST)
	test -d build-$(CHOST)/sqlite-autoconf-$(V_SQL) || (                                            \
	curl -L https://sqlite.org/2017/sqlite-autoconf-$(V_SQL).tar.gz | tar xz -C build-$(CHOST)      \
	&& cd build-$(CHOST)/sqlite-autoconf-$(V_SQL) && CC=$(CC) ./configure --prefix=$(PWD)/$(KLOCAL) \
	--host=$(CHOST) --enable-static --disable-shared --enable-threadsafe && make && make install    )

uws: build-$(CHOST)
	test -d build-$(CHOST)/uWebSockets-$(V_UWS)                                    \
	|| curl -L https://github.com/uNetworking/uWebSockets/archive/v$(V_UWS).tar.gz \
	| tar xz -C build-$(CHOST) && mkdir -p $(KLOCAL)/include/uWS                   \
	&& cp build-$(CHOST)/uWebSockets-$(V_UWS)/src/* $(KLOCAL)/include/uWS/

ncurses: build-$(CHOST)
	test -d build-$(CHOST)/ncurses-$(V_NCUR) || (                                                         \
	curl -L http://ftp.gnu.org/pub/gnu/ncurses/ncurses-$(V_NCUR).tar.gz | tar xz -C build-$(CHOST)        \
	&& cd build-$(CHOST)/ncurses-$(V_NCUR) && CC=$(CC) AR=$(CHOST)-ar CXX=$(CXX) CPPFLAGS=-P ./configure  \
	--host=$(CHOST) --prefix=$(PWD)/$(KLOCAL)                                                             \
	--with-fallbacks=linux,screen,vt100,xterm,xterm-256color,putty-256color && make && make install       )

json: build-$(CHOST)
	test -f $(KLOCAL)/include/json.h || (mkdir -p $(KLOCAL)/include                  \
	&& curl -L https://github.com/nlohmann/json/releases/download/$(V_JSON)/json.hpp \
	-o $(KLOCAL)/include/json.h                                                      )

quickfix: build-$(CHOST)
	test -d build-$(CHOST)/quickfix-$(V_QF) || (                                                   \
	curl -L https://github.com/quickfix/quickfix/archive/$(V_QF).tar.gz | tar xz -C build-$(CHOST) \
	&& patch build-$(CHOST)/quickfix-$(V_QF)/m4/ax_lib_mysql.m4 < etc/without_mysql.m4.patch       \
	&& cd build-$(CHOST)/quickfix-$(V_QF) && ./bootstrap                                           \
	&& (test -n "`echo $(CHOST) | grep darwin`" &&                                                 \
	sed -i '' "s/bin spec test examples doc//" Makefile.am ||                                      \
	sed -i "s/bin spec test examples doc//" Makefile.am)                                           \
	&& (test -n "`echo $(CHOST) | grep darwin`" &&                                                 \
	sed -i '' "s/CXX = g++/CXX \?= g++/" UnitTest++/Makefile ||                                    \
	sed -i "s/CXX = g++/CXX \?= g++/" UnitTest++/Makefile)                                         \
	&& CXX=$(CXX) AR=$(CHOST)-ar ./configure --prefix=$(PWD)/$(KLOCAL) --enable-shared=no          \
	--enable-static=yes --host=$(CHOST) && cd UnitTest++ && CXX=$(CXX) make libUnitTest++.a        \
	&& cd ../src/C++ && CXX=$(CXX) make && make install                                            )

libuv: build-$(CHOST)
	test -z "`echo $(CHOST) | grep darwin`" || test -d build-$(CHOST)/libuv-$(V_UV) || (      \
	curl -L https://github.com/libuv/libuv/archive/v$(V_UV).tar.gz | tar xz -C build-$(CHOST) \
	&& cd build-$(CHOST)/libuv-$(V_UV) && sh autogen.sh && CC=$(CC) ./configure               \
	--host=$(CHOST) --prefix=$(PWD)/$(KLOCAL) --disable-shared && make && make install        )

pvs:
	test -d build-$(CHOST)/pvs-studio-$(V_PVS)-x86_64 || (                     \
	curl -L http://files.viva64.com/pvs-studio-$(V_PVS)-x86_64.tgz             \
	| tar xz -C build-$(CHOST) && cd build-$(CHOST)/pvs-studio-$(V_PVS)-x86_64 \
	&& chmod +x install.sh && sudo ./install.sh                                )

build:
	curl -L https://github.com/ctubio/Krypto-trading-bot/releases/download/$(KGIT)/$(KZIP)-$(CHOST).tar.gz \
	| tar xz && chmod +x build-*/local/lib/K-$(CHOST).a build-*/local/bin/K-$(CHOST)

clean:
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	rm -rf build-$(CHOST)
endif

cleandb: /data/db/K*
	rm -rf /data/db/K*.db

packages:
	test -n "`command -v apt-get`" && sudo apt-get -y install g++ build-essential automake autoconf libtool libxml2 libxml2-dev zlib1g-dev openssl stunnel python curl gzip screen \
	|| (test -n "`command -v yum`" && sudo yum -y install gcc-c++ automake autoconf libtool libxml2 libxml2-devel openssl stunnel python curl gzip screen) \
	|| (test -n "`command -v brew`" && (xcode-select --install || :) && (brew install automake autoconf libxml2 sqlite openssl zlib stunnel python curl gzip || brew upgrade || :)) \
 	|| (test -n "`command -v pacman`" && sudo pacman --noconfirm -S --needed base-devel libxml2 zlib sqlite curl libcurl-compat openssl stunnel python gzip screen)
	sudo mkdir -p /data/db/
	sudo chown $(shell id -u) /data/db

install:
	@$(MAKE) packages
	mkdir -p app/server
	@echo ================================================================================ && echo && echo "Select your architecture to download pre-compiled binaries:" && echo
	@echo -n $(CARCH) | tr ' ' "\n" | xargs -I % echo % | cat -n && echo && echo "(Hint! uname says \"`uname -s` `uname -m`\")" && echo
	@read -p "[1/2/3/4]: " chost; \
	CHOST=`echo $(CARCH) | cut -d ' ' -f$${chost}` $(MAKE) build link

docker:
	@$(MAKE) packages
	mkdir -p app/server
	@$(MAKE) build link
	sed -i "/Usage/,+113d" K.sh

link:
	cd app && ln -f -s \[\!`echo -n "0x46 0x52 0x45 0x45 0x44 0x4f 0x4d" | xxd -r`\ FOR\ `echo -n "0x43 0x41 0x54 0x41 0x4c 0x4f 0x4e 0x49 0x41" | xxd -r`\!\]\ btw\ K\ client\ is\ at\ ../$(KLOCAL)/var/www client
	cd app/server && ln -f -s ../../$(KLOCAL)/bin/K-$(CHOST) K
	test -n "`ls *.sh 2>/dev/null`" || (cp etc/K.sh.dist K.sh && chmod +x K.sh)
	$(MAKE) gdax -s

reinstall: src
	test -d .git && ((test -n "`git diff`" && (echo && echo !!Local changes will be lost!! press CTRL-C to abort. && echo && sleep 4) || :) \
	&& git fetch && git merge FETCH_HEAD || (git reset FETCH_HEAD && git checkout .)) || curl https://raw.githubusercontent.com/ctubio/Krypto-trading-bot/master/Makefile > Makefile
	rm -rf app
	@$(MAKE) install
	#@$(MAKE) test -s
	#@$(MAKE) restartall
	@echo && echo ..done! Please restart any running instance and also refresh the UI if is currently opened in your browser.

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
	ls -1 *.sh | cut -d / -f2 | cut -d \* -f1 | grep -v ^_ | xargs -I % sh -c 'sleep 2;$(MAKE) K=% start -s'
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

cabundle:
	curl --time-cond etc/K-cabundle.pem https://curl.haxx.se/ca/cacert.pem -o etc/K-cabundle.pem

gdax:
	openssl s_client -showcerts -connect fix.gdax.com:4198 -CApath /etc/ssl/certs < /dev/null \
	| openssl x509 -outform PEM > etc/sslcert/fix.gdax.com.pem

client: src/client
	rm -rf $(KLOCAL)/var
	mkdir -p $(KLOCAL)/var/www
	@echo Building client dynamic files..
	@npm install
	./node_modules/.bin/tsc --alwaysStrict --experimentalDecorators -t ES2017 -m commonjs --outDir $(KLOCAL)/var/www/js src/client/*.ts
	@echo DONE

www: src/www $(KLOCAL)/var/www
	@echo Building client static files..
	cp -R src/www/* $(KLOCAL)/var/www/
	@echo DONE

bundle: client www node_modules/.bin/browserify node_modules/.bin/uglifyjs $(KLOCAL)/var/www/js/main.js
	@echo Building client bundle file..
	mkdir -p $(KLOCAL)/var/www/js/client
	./node_modules/.bin/browserify -t [ babelify --presets [ babili env ] ] $(KLOCAL)/var/www/js/main.js $(KLOCAL)/var/www/js/lib/*.js | ./node_modules/.bin/uglifyjs | gzip > $(KLOCAL)/var/www/js/client/bundle.min.js
	rm $(KLOCAL)/var/www/js/*.js
	echo $(CARCH) | tr ' ' "\n" | xargs -I % echo % | grep -v $(CHOST) | xargs -I % sh -c 'if test -d build-%; then rm -rf build-%/local/var;mkdir -p build-%/local/var;cp -R $(KLOCAL)/var build-%/local; fi'
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
	$(MAKE) test-c

test-cov: node_modules/.bin/ts-node node_modules/istanbul/lib/cli.js node_modules/.bin/_mocha
	./node_modules/.bin/ts-node ./node_modules/istanbul/lib/cli.js cover --report lcovonly --dir test/coverage -e .ts ./node_modules/.bin/_mocha -- --timeout 42000 test/*.ts

test-c:
	@echo "// This is an independent project of an individual developer. Dear PVS-Studio, please check it.\n// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com\n\n" > src/server/K.test.cxx
	@cat src/server/K.cxx >> src/server/K.test.cxx
	@pvs-studio-analyzer analyze --exclude-path $(KLOCAL)/include --source-file src/server/K.test.cxx --cl-params -I$(KLOCAL)/include src/server/K.test.cxx && \
	plog-converter -a GA:1,2 -t tasklist -o report.tasks PVS-Studio.log
	@cat report.tasks
	@rm report.tasks PVS-Studio.log src/server/K.test.cxx

send-cov: node_modules/.bin/codacy-coverage node_modules/.bin/istanbul-coveralls
	cd test && cat coverage/lcov.info | ./node_modules/.bin/codacy-coverage && ./node_modules/.bin/istanbul-coveralls

travis-gcc:
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt-get install gcc-6
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-6 50
	sudo apt-get install g++-6
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 50
	$(MAKE) travis-dist pvs

travis-dist:
	mkdir -p $(KLOCAL)
	npm install

png: etc/${PNG}.png etc/${PNG}.json
	convert etc/${PNG}.png -set "K.conf" "`cat etc/${PNG}.json`" K: etc/${PNG}.png 2>/dev/null || :
	@$(MAKE) png-check -s

png-check: etc/${PNG}.png
	@test -n "`identify -verbose etc/${PNG}.png | grep 'K\.conf'`" && echo Configuration injected into etc/${PNG}.png OK, feel free to remove etc/${PNG}.json anytime. || echo nope, injection failed.

check:
	@echo $(KZIP)
	@shasum $(KLOCAL)/bin/K-$(CHOST) | cut -d ' ' -f1

checkOK:
	@sed -i "s/^\(KZIP     = \).*$$/\1`shasum $(KLOCAL)/bin/K-$(CHOST) | cut -d ' ' -f1`/" Makefile
	@$(MAKE) check -s

release:
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	@tar -cvzf $(KZIP)-$(CHOST).tar.gz                                                                                   \
	LICENSE COPYING THANKS README.md MANUAL.md src etc $(KLOCAL)/bin/K-$(CHOST) $(KLOCAL)/var $(KLOCAL)/lib/K-$(CHOST).a \
	Makefile && curl -s -n -H "Content-Type:application/octet-stream" -H "Authorization: token ${KRELEASE}"              \
	--data-binary "@$(PWD)/$(KZIP)-$(CHOST).tar.gz"                                                                      \
	"https://uploads.github.com/repos/ctubio/Krypto-trading-bot/releases/$(KHUB)/assets?name=$(KZIP)-$(CHOST).tar.gz"    \
	&& rm $(KZIP)-$(CHOST).tar.gz && echo && echo DONE $(KZIP)-$(CHOST).tar.gz
endif

md5: src
	find src -type f -exec md5sum "{}" + > src.md5

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: K dist link Linux Darwin build zlib openssl curl ncurses quickfix uws json pvs clean cleandb list screen start stop restart startall stopall restartall gdax packages install docker travis reinstall client www bundle diff latest changelog test test-cov send-cov png png-check release md5 asandwich
