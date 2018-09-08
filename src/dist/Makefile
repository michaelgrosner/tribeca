ERR     := *** Unexpected MAKELEVEL = 0.
HINT    := This Makefile can't be used directly, consider cd ../.. before try again
$(if $(shell test ${MAKELEVEL} -eq 0 || echo 1),,$(warning $(ERR));$(error $(HINT)))

V_ZLIB   = 1.2.11
V_SSL    = 1.1.0h
V_CURL   = 7.61.0
V_SQL    = 3230100
V_NCUR   = 6.1
V_JSON   = 3.1.2
V_CATCH  = 2.2.3
V_UWS    = 0.14.8
V_QF     = 1.15.1
V_UV     = 1.20.3

KBUILD  := $(abspath ../../build-$(CHOST))

all dist:
	mkdir -p $(KBUILD)
	$(MAKE) zlib openssl curl sqlite ncurses json catch uws quickfix libuv

clean:
	rm -rf $(KBUILD)

zlib:
	test -d $(KBUILD)/zlib-$(V_ZLIB) || (                                                    \
	curl -L https://zlib.net/zlib-$(V_ZLIB).tar.gz | tar xz -C $(KBUILD)                     \
	&& cd $(KBUILD)/zlib-$(V_ZLIB) && (test -n "`echo $(CHOST) | grep mingw32`" &&           \
	(sed -i "s/^\(PREFIX =\).*$$/\1$(CHOST)-/" win32/Makefile.gcc && make win32/Makefile.gcc \
	&& BINARY_PATH=$(KBUILD)/local/bin INCLUDE_PATH=$(KBUILD)/local/include                  \
	LIBRARY_PATH=$(KBUILD)/local/lib make install -fwin32/Makefile.gcc)                      \
	|| (./configure --static --prefix=$(KBUILD)/local && make && make install))              )

openssl:
	test -d $(KBUILD)/openssl-$(V_SSL) || (                                                    \
	curl -L https://www.openssl.org/source/openssl-$(V_SSL).tar.gz | tar xz -C $(KBUILD)       \
	&& cd $(KBUILD)/openssl-$(V_SSL) && CC=gcc                                                 \
	./Configure $(shell test -n "`echo $(CHOST) | grep mingw32`" && echo mingw64 || echo dist) \
	--cross-compile-prefix=$(CHOST)- --prefix=$(KBUILD)/local                                  \
	--openssldir=$(KBUILD)/local && make && make install_sw install_ssldirs                    )

curl:
	curl --time-cond $(KBUILD)/local/certs/cabundle.pem https://curl.haxx.se/ca/cacert.pem \
	  -o $(KBUILD)/local/certs/cabundle.pem
	test -d $(KBUILD)/curl-$(V_CURL) || (                                                   \
	curl -L https://curl.haxx.se/download/curl-$(V_CURL).tar.gz | tar xz -C $(KBUILD)       \
	&& cd $(KBUILD)/curl-$(V_CURL) && ./configure --host=$(CHOST) --disable-manual          \
	--with-ca-bundle=$(KBUILD)/local/certs/cabundle.pem                                     \
	--prefix=$(KBUILD)/local --disable-imap --disable-file --disable-tftp --disable-smtp    \
	--disable-pop3 --disable-smb --disable-ftp --disable-sspi --disable-ldap --disable-dict \
	--without-libpsl --without-nghttp2 --without-libssh2 --disable-telnet --disable-rtsp    \
	--disable-gopher --without-libidn2 --without-librtmp --disable-shared --enable-static   \
	--with-zlib=$(KBUILD)/local --with-ssl=$(KBUILD)/local && make && make install          )

sqlite:
	test -d $(KBUILD)/sqlite-autoconf-$(V_SQL) || (                                              \
	curl -L https://sqlite.org/2018/sqlite-autoconf-$(V_SQL).tar.gz | tar xz -C $(KBUILD)        \
	&& cd $(KBUILD)/sqlite-autoconf-$(V_SQL) && ./configure --prefix=$(KBUILD)/local             \
	--host=$(CHOST) --enable-static --disable-shared --enable-threadsafe && make && make install )

ncurses:
	test -d $(KBUILD)/ncurses-$(V_NCUR) || (                                                          \
	curl -L http://ftp.gnu.org/pub/gnu/ncurses/ncurses-$(V_NCUR).tar.gz | tar xz -C $(KBUILD)         \
	&& cd $(KBUILD)/ncurses-$(V_NCUR) && CPPFLAGS=-P ./configure                                      \
	--host=$(CHOST) --prefix=$(KBUILD)/local $(shell test -n "`echo $(CHOST) | grep mingw32`" && echo \
	--without-cxx-binding --without-ada --enable-reentrant --with-normal                              \
	--disable-home-terminfo --enable-sp-funcs --enable-term-driver --enable-interop || :)             \
	--disable-lib-suffixes --without-debug --without-progs --without-tests                            \
	--with-fallbacks=linux,screen,vt100,xterm,xterm-256color,putty-256color && make && make install   )

json:
	test -f $(KBUILD)/local/include/json.h || (mkdir -p $(KBUILD)/local/include       \
	&& curl -L https://github.com/nlohmann/json/releases/download/v$(V_JSON)/json.hpp \
	-o $(KBUILD)/local/include/json.h                                                 )

catch:
	test -f $(KBUILD)/local/include/catch.h || (mkdir -p $(KBUILD)/local/include          \
	&& curl -L https://github.com/catchorg/Catch2/releases/download/v$(V_CATCH)/catch.hpp \
	-o $(KBUILD)/local/include/catch.h                                                    )

uws:
	test -d $(KBUILD)/uWebSockets-$(V_UWS)                                             \
	|| curl -L https://github.com/uNetworking/uWebSockets/archive/v$(V_UWS).tar.gz     \
	| tar xz -C $(KBUILD) && rm -rf $(KBUILD)/local/include/uWS                        \
	&& mkdir -p $(KBUILD)/local/include/uWS && cp $(KBUILD)/uWebSockets-$(V_UWS)/src/* \
	$(KBUILD)/local/include/uWS/ && (test -n "`echo $(CHOST) | grep mingw32`"          \
	&& (sed -i "s/W\(s2tcpip\)/w\1/" $(KBUILD)/local/include/uWS/Networking.h          \
	&& sed -i "s/WinSock2/winsock2/" $(KBUILD)/local/include/uWS/Networking.h          \
	) ||    :)

quickfix:
	test -d $(KBUILD)/quickfix-$(V_QF) || (                                                       \
	curl -L https://github.com/quickfix/quickfix/archive/v$(V_QF).tar.gz | tar xz -C $(KBUILD)    \
	&& cd $(KBUILD)/quickfix-$(V_QF) && ./bootstrap                                               \
	&& (test -n "`echo $(CHOST) | grep darwin`" &&                                                \
	sed -i '' "s/bin spec test examples doc//" Makefile.am ||                                     \
	sed -i "s/bin spec test examples doc//" Makefile.am)                                          \
	&& (test -n "`echo $(CHOST) | grep darwin`" &&                                                \
	sed -i '' "s/SUBDIRS = test//" src/C++/Makefile.am ||                                         \
	sed -i "s/SUBDIRS = test//" src/C++/Makefile.am) && (test -n "`echo $(CHOST) | grep mingw32`" \
	&& patch -p0 < $(realpath with_win32.src.patch) || :)                                         \
	&& ./configure --prefix=$(KBUILD)/local --enable-shared=no                                    \
	--enable-static=yes --host=$(CHOST) && cd src/C++ && make && make install                     )

libuv:
	test -z "`echo $(CHOST) | grep darwin;echo $(CHOST) | grep mingw32`" || test -d $(KBUILD)/libuv-$(V_UV) || ( \
	curl -L https://github.com/libuv/libuv/archive/v$(V_UV).tar.gz | tar xz -C $(KBUILD)                         \
	&& cd $(KBUILD)/libuv-$(V_UV) && sh autogen.sh && CC=$(CHOST)-clang ./configure --host=$(CHOST)              \
	--prefix=$(KBUILD)/local && make && make install                                                             )

.PHONY: all dist clean zlib openssl curl sqlite ncurses json catch uws quickfix libuv