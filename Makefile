K       ?= K.sh
MAJOR    = 0
MINOR    = 4
PATCH    = 9
BUILD    = 4
CHOST   ?= $(shell $(MAKE) CHOST= chost -s)
CARCH    = x86_64-linux-gnu arm-linux-gnueabihf aarch64-linux-gnu x86_64-apple-darwin17 x86_64-w64-mingw32
KLOCAL  := build-$(CHOST)/local
CXX     := $(CHOST)-g++
CC      := $(CHOST)-gcc
V_CXX    = 6
ERR     := *** K require g++ v$(V_CXX), but g++ v$(V_CXX) was not found at $(shell which "$(CXX)" 2> /dev/null)
HINT    := consider to create a symlink at $(shell which "$(CXX)" 2> /dev/null) pointing to your g++-$(V_CXX) executable
V_ZLIB   = 1.2.11
V_SSL    = 1.1.0h
V_CURL   = 7.61.0
V_NCUR   = 6.1
V_JSON   = 3.1.2
V_UWS    = 0.14.8
V_SQL    = 3230100
V_QF     = 1.15.1
V_UV     = 1.20.3
V_CATCH  = 2.2.3
KARGS   := -I$(KLOCAL)/include -pthread -std=c++11 -O3   \
  $(KLOCAL)/lib/K-$(CHOST)-docroot.o src/server/K.cxx    \
  -DK_0_DAY='"v$(MAJOR).$(MINOR).$(PATCH)+$(BUILD)"'     \
  -DK_STAMP='"$(shell date "+%Y-%m-%d %H:%M:%S")"'       \
  -DK_BUILD='"$(CHOST)"'     $(KLOCAL)/include/uWS/*.cpp \
  $(KLOCAL)/lib/K-$(CHOST).a $(KLOCAL)/lib/libquickfix.a \
  $(KLOCAL)/lib/libsqlite3.a $(KLOCAL)/lib/libz.a        \
  $(KLOCAL)/lib/libcurl.a    $(KLOCAL)/lib/libssl.a      \
  $(KLOCAL)/lib/libcrypto.a  $(KLOCAL)/lib/libncurses.a

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
	#  make clients      - compile K clients src       #
	#  make www          - compile K clients src       #
	#  make css          - compile K clients css       #
	#  make bundle       - compile K clients bundle    #
	#  make docroot      - compile K clients lib       #
	#                                                  #
	#  make test         - run tests                   #
	#  make test-c       - run static tests            #
	#                                                  #
	#  make build        - download K src precompiled  #
	#  make pvs          - download pvs src files      #
	#  make zlib         - download zlib src files     #
	#  make curl         - download curl src files     #
	#  make ncurses      - download ncurses src files  #
	#  make sqlite       - download sqlite src files   #
	#  make openssl      - download openssl src files  #
	#  make json         - download json src files     #
	#  make catch        - download catch src files    #
	#  make uws          - download uws src files      #
	#  make quickfix     - download quickfix src files #
	#  make coinbase     - download coinbase ssl cert  #
	#  make cabundle     - download ssl CA certs       #
	#  make clean        - remove external src files   #
	#  KALL=1 make clean - remove external src files   #
	#  make cleandb      - remove databases            #
	#                                                  #

chost:
	@echo -n $(shell (test -d .git && test -n "`command -v g++`") && \
	g++ -dumpmachine || ls -1 . | grep build- | head -n1 | cut -d '/' -f1 | cut -d '-' -f2-)

K: src/server/K.cxx
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	@$(if $(shell sh -c 'test "`g++ -dumpversion | cut -d . -f1`" != $(V_CXX) || echo 1'),,$(warning $(ERR));$(error $(HINT)))
	@$(CXX) --version
	mkdir -p $(KLOCAL)/bin
	CHOST=$(CHOST) $(MAKE) $(shell test -n "`echo $(CHOST) | grep darwin`" && echo Darwin || (test -n "`echo $(CHOST) | grep mingw32`" && echo Win32 || uname -s))
	chmod +x $(KLOCAL)/bin/K-$(CHOST)$(shell test -n "`echo $(CHOST) | grep mingw32`" && echo .exe || :)
endif

dist:
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	@$(if $(shell sh -c 'test "`g++ -dumpversion | cut -d . -f1`" != $(V_CXX) || echo 1'),,$(warning $(ERR));$(error $(HINT)))
	mkdir -p build-$(CHOST)
	CHOST=$(CHOST) $(MAKE) zlib openssl curl sqlite ncurses json catch uws quickfix libuv
	test -f /sbin/ldconfig && sudo ldconfig || :
endif

docroot:
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	cd $(KLOCAL) && $(CXX) -c ../../src/build/document_root.S -o lib/K-$(CHOST)-docroot.o
endif

Linux:
ifdef KUNITS
	@unset KUNITS && $(MAKE) KTEST="--coverage -I. test/unit_testing_framework.cxx" $@
else ifndef KTEST
	@$(MAKE) KTEST="-DNDEBUG" $@
else
	$(CXX) $(KTEST) -o $(KLOCAL)/bin/K-$(CHOST) -DHAVE_STD_UNIQUE_PTR -DUWS_THREADSAFE -static-libstdc++ -static-libgcc -rdynamic $(KARGS) -ldl
endif

Darwin:
	$(CXX) -DNDEBUG -o $(KLOCAL)/bin/K-$(CHOST) -DUSE_LIBUV $(KLOCAL)/lib/libuv.a -msse4.1 -maes -mpclmul -mmacosx-version-min=10.13 -nostartfiles -rdynamic $(KARGS) -ldl

Win32:
	$(CXX)-posix -DNDEBUG -o $(KLOCAL)/bin/K-$(CHOST).exe -DUSE_LIBUV $(KARGS) $(KLOCAL)/lib/libuv.dll.a $(KLOCAL)/lib/libssl.dll.a $(KLOCAL)/lib/libcrypto.dll.a -DCURL_STATICLIB -static -lstdc++ -lgcc -lwldap32 -lws2_32

zlib:
	test -d build-$(CHOST)/zlib-$(V_ZLIB) || (                                                  \
	curl -L https://zlib.net/zlib-$(V_ZLIB).tar.gz | tar xz -C build-$(CHOST)                   \
	&& cd build-$(CHOST)/zlib-$(V_ZLIB) && (test -n "`echo $(CHOST) | grep mingw32`" &&         \
	(sed -i "s/^\(PREFIX =\).*$$/\1$(CHOST)-/" win32/Makefile.gcc && make -fwin32/Makefile.gcc  \
	&& BINARY_PATH=$(PWD)/$(KLOCAL)/bin INCLUDE_PATH=$(PWD)/$(KLOCAL)/include                   \
	LIBRARY_PATH=$(PWD)/$(KLOCAL)/lib make install -fwin32/Makefile.gcc)                        \
	|| (CC=$(CC) ./configure --static --prefix=$(PWD)/$(KLOCAL) && make && make install))       )

openssl:
	test -d build-$(CHOST)/openssl-$(V_SSL) || (                                               \
	curl -L https://www.openssl.org/source/openssl-$(V_SSL).tar.gz | tar xz -C build-$(CHOST)  \
	&& cd build-$(CHOST)/openssl-$(V_SSL) && CC=gcc                                            \
	./Configure $(shell test -n "`echo $(CHOST) | grep mingw32`" && echo mingw64 || echo dist) \
	--cross-compile-prefix=$(CHOST)- --prefix=$(PWD)/$(KLOCAL)                                 \
	--openssldir=$(PWD)/$(KLOCAL) && make && make install_sw install_ssldirs                   )

curl:
	test -d build-$(CHOST)/curl-$(V_CURL) || (                                                  \
	curl -L https://curl.haxx.se/download/curl-$(V_CURL).tar.gz | tar xz -C build-$(CHOST)      \
	&& cd build-$(CHOST)/curl-$(V_CURL) && CC=$(CC) ./configure --with-ca-path=/etc/ssl/certs   \
	--host=$(CHOST) --target=$(CHOST) --build=$(shell g++ -dumpmachine) --disable-manual        \
	--disable-shared --enable-static --prefix=$(PWD)/$(KLOCAL) --disable-ldap --without-libpsl  \
	--without-libssh2 --without-nghttp2 --disable-sspi --without-librtmp --disable-ftp          \
	--disable-file --disable-dict --disable-telnet --disable-tftp --disable-rtsp --disable-pop3 \
	--disable-imap --disable-smtp --disable-gopher --disable-smb --without-libidn2              \
	--with-zlib=$(PWD)/$(KLOCAL) --with-ssl=$(PWD)/$(KLOCAL) && make && make install            )

sqlite:
	test -d build-$(CHOST)/sqlite-autoconf-$(V_SQL) || (                                            \
	curl -L https://sqlite.org/2018/sqlite-autoconf-$(V_SQL).tar.gz | tar xz -C build-$(CHOST)      \
	&& cd build-$(CHOST)/sqlite-autoconf-$(V_SQL) && CC=$(CC) ./configure --prefix=$(PWD)/$(KLOCAL) \
	--host=$(CHOST) --enable-static --disable-shared --enable-threadsafe && make && make install    )

ncurses:
	test -d build-$(CHOST)/ncurses-$(V_NCUR) || (                                                        \
	curl -L http://ftp.gnu.org/pub/gnu/ncurses/ncurses-$(V_NCUR).tar.gz | tar xz -C build-$(CHOST)       \
	&& cd build-$(CHOST)/ncurses-$(V_NCUR) && CC=$(CC) AR=$(CHOST)-ar CXX=$(CXX) CPPFLAGS=-P ./configure \
	--host=$(CHOST) --prefix=$(PWD)/$(KLOCAL) $(shell test -n "`echo $(CHOST) | grep mingw32`" && echo   \
	--without-cxx-binding --without-ada --enable-reentrant --with-normal                                 \
	--disable-home-terminfo --enable-sp-funcs --enable-term-driver --enable-interop || :)                \
	--disable-lib-suffixes --without-debug --without-progs --without-tests                               \
	--with-fallbacks=linux,screen,vt100,xterm,xterm-256color,putty-256color && make && make install      )

json:
	test -f $(KLOCAL)/include/json.h || (mkdir -p $(KLOCAL)/include                   \
	&& curl -L https://github.com/nlohmann/json/releases/download/v$(V_JSON)/json.hpp \
	-o $(KLOCAL)/include/json.h                                                       )

catch:
	test -f $(KLOCAL)/include/catch.h || (mkdir -p $(KLOCAL)/include                      \
	&& curl -L https://github.com/catchorg/Catch2/releases/download/v$(V_CATCH)/catch.hpp \
	-o $(KLOCAL)/include/catch.h                                                          )

uws:
	test -d build-$(CHOST)/uWebSockets-$(V_UWS)                                    \
	|| curl -L https://github.com/uNetworking/uWebSockets/archive/v$(V_UWS).tar.gz \
	| tar xz -C build-$(CHOST) && mkdir -p $(KLOCAL)/include/uWS                   \
	&& cp build-$(CHOST)/uWebSockets-$(V_UWS)/src/* $(KLOCAL)/include/uWS/         \
	&& (test -n "`echo $(CHOST) | grep mingw32`" &&                                \
	(sed -i "s/W\(s2tcpip\)/w\1/" $(KLOCAL)/include/uWS/Networking.h &&            \
	sed -i "s/WinSock2/winsock2/" $(KLOCAL)/include/uWS/Networking.h) ||          :)

quickfix:
	test -d build-$(CHOST)/quickfix-$(V_QF) || (                                                    \
	curl -L https://github.com/quickfix/quickfix/archive/v$(V_QF).tar.gz | tar xz -C build-$(CHOST) \
	&& cd build-$(CHOST)/quickfix-$(V_QF) && ./bootstrap                                            \
	&& (test -n "`echo $(CHOST) | grep darwin`" &&                                                  \
	sed -i '' "s/bin spec test examples doc//" Makefile.am ||                                       \
	sed -i "s/bin spec test examples doc//" Makefile.am)                                            \
	&& (test -n "`echo $(CHOST) | grep darwin`" &&                                                  \
	sed -i '' "s/SUBDIRS = test//" src/C++/Makefile.am ||                                           \
	sed -i "s/SUBDIRS = test//" src/C++/Makefile.am) && (test -n "`echo $(CHOST) | grep mingw32`"   \
	&& patch -p2 < ../../src/build/with_win32.src.patch || :)                                       \
	&& CXX=$(CXX) AR=$(CHOST)-ar ./configure --prefix=$(PWD)/$(KLOCAL) --enable-shared=no           \
	--enable-static=yes --host=$(CHOST) && cd src/C++ && CXX=$(CXX) make && make install            )

libuv:
	test -z "`echo $(CHOST) | grep darwin;echo $(CHOST) | grep mingw32`" || test -d build-$(CHOST)/libuv-$(V_UV) || ( \
	curl -L https://github.com/libuv/libuv/archive/v$(V_UV).tar.gz | tar xz -C build-$(CHOST)                         \
	&& cd build-$(CHOST)/libuv-$(V_UV) && sh autogen.sh && CC=$(CHOST)-clang ./configure --host=$(CHOST)              \
	--prefix=$(PWD)/$(KLOCAL) && make && make install                                                                 )

pvs:
ifndef V_PVS
	$(MAKE) V_PVS=$(shell curl -s https://www.viva64.com/en/pvs-studio-download-linux/ | grep x86_64.tgz | sed 's/.*href=\"\(.*\)\" .*/\1/' | cut -d '-' -f3) $@
else
	test -d build-$(CHOST)/pvs-studio-$(V_PVS)-x86_64 || (                     \
	curl -L http://files.viva64.com/pvs-studio-$(V_PVS)-x86_64.tgz             \
	| tar xz -C build-$(CHOST) && cd build-$(CHOST)/pvs-studio-$(V_PVS)-x86_64 \
	&& chmod +x install.sh && sudo ./install.sh                                )
endif

build:
	curl -L https://github.com/ctubio/Krypto-trading-bot/releases/download/$(MAJOR).$(MINOR).x/v$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(CHOST).tar.gz \
	| tar xz && chmod +x build-*/local/bin/K-$(CHOST)

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
	|| (test -n "`command -v brew`" && (xcode-select --install || :) && (brew install automake autoconf libxml2 sqlite openssl zlib stunnel python curl gzip proctools || brew upgrade || :)) \
	|| (test -n "`command -v pacman`" && sudo pacman --noconfirm -S --needed base-devel libxml2 zlib sqlite curl libcurl-compat openssl stunnel python gzip screen)
	sudo mkdir -p /data/db/
	sudo chown $(shell id -u) /data/db

install:
	@$(MAKE) packages
	mkdir -p app/server
	@yes = | head -n`expr $(shell tput cols) / 2` | xargs echo && echo " _  __\n| |/ /\n| ' /   Select your (beloved) architecture\n| . \\   to download pre-compiled binaries:\n|_|\\_\\ \n"
	@echo $(CARCH) | tr ' ' "\n" | cat -n && echo "\n(Hint! uname says \"`uname -sm`\", and win32 auto-install does not work yet)\n"
	@read -p "[1/2/3/4/5]: " chost; \
	CHOST=`echo $(CARCH) | cut -d ' ' -f$${chost}` $(MAKE) build link

docker:
	@$(MAKE) packages
	mkdir -p app/server
	@$(MAKE) build link
	sed -i "/Usage/,+115d" K.sh

link:
	cd app/server && ln -f -s ../../$(KLOCAL)/bin/K-$(CHOST) K
	test -n "`ls *.sh 2>/dev/null`" || (cp etc/K.sh.dist K.sh && chmod +x K.sh)
	test -f etc/sslcert/server.crt || cp etc/sslcert/unsecure-sample.server.crt etc/sslcert/server.crt
	test -f etc/sslcert/server.key || cp etc/sslcert/unsecure-sample.server.key etc/sslcert/server.key
	@$(MAKE) coinbase -s

reinstall: src
	test -d .git && ((test -n "`git diff`" && (echo && echo !!Local changes will be lost!! press CTRL-C to abort. && echo && sleep 4) || :) \
	&& git fetch && git merge FETCH_HEAD || (git reset FETCH_HEAD && git checkout .)) || curl https://raw.githubusercontent.com/ctubio/Krypto-trading-bot/master/Makefile > Makefile
	rm -rf app
	@$(MAKE) install
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
	curl --time-cond etc/cabundle.pem https://curl.haxx.se/ca/cacert.pem -o etc/cabundle.pem

coinbase:
	@openssl s_client -showcerts -connect fix.gdax.com:4198 -CApath /etc/ssl/certs < /dev/null 2> /dev/null \
	| openssl x509 -outform PEM > etc/sslcert/fix.gdax.com.pem

clients: src/client-2d
	rm -rf $(KLOCAL)/var
	mkdir -p $(KLOCAL)/var/www
	@echo Building clients dynamic files..
	@npm install
	./node_modules/.bin/tsc --alwaysStrict --experimentalDecorators -t ES2017 -m commonjs --outDir $(KLOCAL)/var/www/js src/client-2d/*.ts
	@echo DONE

www: src/www
	@echo Building clients static files..
	cp -R src/www/* $(KLOCAL)/var/www/
	@echo DONE

css: src/www/sass
	@echo Building clients CSS files..
	rm -rf $(KLOCAL)/var/www/css
	mkdir -p $(KLOCAL)/var/www/css
	ls -1 src/www/sass/*.scss | sed -r 's/(.*)(\.scss)$$/\1\2 \1\.min.css/' | xargs -I % sh -c './node_modules/.bin/sass %;' \
	&& rm src/www/sass/*.min.css.map && mv src/www/sass/*.min.css $(KLOCAL)/var/www/css/ && cat ./node_modules/ag-grid/dist/styles/ag-grid.css >> $(KLOCAL)/var/www/css/bootstrap.min.css
	@echo DONE

bundle: clients www css node_modules/.bin/browserify node_modules/.bin/uglifyjs
	@echo Building clients bundle zip and docroot lib..
	mkdir -p $(KLOCAL)/var/www/js/client
	./node_modules/.bin/browserify -t [ babelify --presets [ babili env ] ] $(KLOCAL)/var/www/js/main.js $(KLOCAL)/var/www/js/lib/*.js | ./node_modules/.bin/uglifyjs | gzip > $(KLOCAL)/var/www/js/client/bundle.min.js
	rm -rf $(KLOCAL)/var/www/js/lib $(KLOCAL)/var/www/js/*.js $(KLOCAL)/var/www/sass
	echo $(CARCH) | tr ' ' "\n" | xargs -I % echo % | grep -v $(CHOST) | xargs -I % sh -c 'if test -d build-%; then rm -rf build-%/local/var;mkdir -p build-%/local/var;cp -R $(KLOCAL)/var build-%/local; fi'
	echo $(CARCH) | tr ' ' "\n" | xargs -I % echo % | xargs -I % sh -c 'if test -d build-%; then CHOST=% make docroot; fi'
	@echo DONE

diff: .git
	@_() { echo $$2 $$3 version: `git rev-parse $$1`; }; git remote update && _ @ Local running && _ @{u} Latest remote
	@$(MAKE) changelog -s

latest: .git diff
	@_() { git rev-parse $$1; }; test `_ @` != `_ @{u}` && $(MAKE) reinstall || :

changelog: .git
	@_() { echo `git rev-parse $$1`; }; echo && git --no-pager log --graph --oneline @..@{u} && test `_ @` != `_ @{u}` || echo No need to upgrade, both versions are equal.

test:
	./K.sh --version
ifndef TRAVIS_DEPLOY
	@rm -f *.gcda *.gcno
endif

test-c:
	@pvs-studio-analyzer analyze --exclude-path $(KLOCAL)/include --source-file test/static_code_analysis.cxx --cl-params -I. -I$(KLOCAL)/include test/static_code_analysis.cxx && \
	(plog-converter -a GA:1,2 -t tasklist -o report.tasks PVS-Studio.log && cat report.tasks && rm report.tasks) || :
	@rm -f PVS-Studio.log

png: etc/${PNG}.png etc/${PNG}.json
	convert etc/${PNG}.png -set "K.conf" "`cat etc/${PNG}.json`" K: etc/${PNG}.png 2>/dev/null || :
	@$(MAKE) png-check -s

png-check: etc/${PNG}.png
	@test -n "`identify -verbose etc/${PNG}.png | grep 'K\.conf'`" && echo Configuration injected into etc/${PNG}.png OK, feel free to remove etc/${PNG}.json anytime. || echo nope, injection failed.

checkOK:
	read -p "KMOD: " KMOD;date=`date`;git diff;git status && read ctrl_c    \
	&& KALL=1 $(MAKE) K release && git add . && git commit -S -m "$${KMOD}" \
	&& git push && echo $${date} && date

MAJOR:
	@sed -i "s/^\(MAJOR    =\).*$$/\1 $(shell expr $(MAJOR) + 1)/" Makefile
	@sed -i "s/^\(MINOR    =\).*$$/\1 0/" Makefile
	@sed -i "s/^\(PATCH    =\).*$$/\1 0/" Makefile
	@sed -i "s/^\(BUILD    =\).*$$/\1 0/" Makefile
	$(MAKE) checkOK

MINOR:
	@sed -i "s/^\(MINOR    =\).*$$/\1 $(shell expr $(MINOR) + 1)/" Makefile
	@sed -i "s/^\(PATCH    =\).*$$/\1 0/" Makefile
	@sed -i "s/^\(BUILD    =\).*$$/\1 0/" Makefile
	$(MAKE) checkOK

PATCH:
	@sed -i "s/^\(PATCH    =\).*$$/\1 $(shell expr $(PATCH) + 1)/" Makefile
	@sed -i "s/^\(BUILD    =\).*$$/\1 0/" Makefile
	$(MAKE) checkOK

BUILD:
	@sed -i "s/^\(BUILD    =\).*$$/\1 $(shell expr $(BUILD) + 1)/" Makefile
	$(MAKE) checkOK

release:
ifdef KALL
	unset KALL && echo -n $(CARCH) | tr ' ' "\n" | xargs -I % $(MAKE) CHOST=% $@
else
	@tar -cvzf v$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(CHOST).tar.gz $(KLOCAL)/bin/K-$(CHOST)* $(KLOCAL)/lib/K-$(CHOST)*                   \
	$(shell test -n "`echo $(CHOST) | grep mingw32`" && echo $(KLOCAL)/bin/*dll || :)                                                     \
	LICENSE COPYING THANKS README.md MANUAL.md src etc test Makefile WHITE_*                                                                   \
	&& curl -s -n -H "Content-Type:application/octet-stream" -H "Authorization: token ${KRELEASE}"                                        \
	--data-binary "@$(PWD)/v$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(CHOST).tar.gz"                                                          \
	"https://uploads.github.com/repos/ctubio/Krypto-trading-bot/releases/$(shell curl -s                                                  \
	https://api.github.com/repos/ctubio/Krypto-trading-bot/releases/latest | grep id | head -n1 | cut -d ' ' -f4 | cut -d ',' -f1         \
	)/assets?name=v$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(CHOST).tar.gz"                                                                   \
	&& rm v$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(CHOST).tar.gz && echo && echo DONE v$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(CHOST).tar.gz
endif

md5: src
	find src -type f -exec md5sum "{}" + > src.md5

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: K chost dist link Linux Darwin Win32 build zlib openssl curl ncurses quickfix uws json catch pvs clean cleandb list screen start stop restart startall stopall restartall coinbase packages install docker reinstall clients www bundle diff latest changelog test test-c png png-check release md5 asandwich
