ERR      = *** Unexpected MAKELEVEL = 0.
HINT     = This Makefile can't be used directly, consider cd ../.. before try again
$(if $(subst 0,,${MAKELEVEL}),,$(warning $(ERR));$(error $(HINT)))

V_ZLIB  = 1.2.11
V_SSL   = 1.1.1i
V_CURL  = 7.74.0
V_SQL   = 3340000
V_NCUR  = 6.1
V_JSON  = 3.9.1
V_CATCH = 2.11.1
V_UV    = 1.40.0

KBUILD := $(abspath ../../build-$(KHOST))

all dist:
	@mkdir -vp $(KBUILD)
	$(MAKE) zlib openssl curl sqlite ncurses json catch uv
ifdef TRAVIS_OS_NAME
	$(MAKE) lcov pvs
endif

clean:
	@rm -vrf $(KBUILD)

check:
	# zlib    | $(V_ZLIB)  | $(shell curl -s https://www.zlib.net/ChangeLog.txt | grep "Changes in " | head -n1 | cut -d' ' -f3)
	# openssl | $(V_SSL)  | $(shell curl -s https://www.openssl.org/news/cl$(shell curl -s https://www.openssl.org/source/ | grep "The latest stable version is " | cut -d' ' -f12  | tr -d '.').txt | grep "Changes between " | head -n1 | cut -d' ' -f4)
	# curl    | $(V_CURL)  | $(shell curl -s https://curl.se/changes.html | grep "Fixed in " | head -n1 | cut -d' ' -f4)
	# sqlite  | $(V_SQL) | $(shell curl -s https://www.sqlite.org/download.html | grep "sqlite-amalgamation-" | head -n1 | cut -d'-' -f3 | cut -d'.' -f1)
	# ncurses | $(V_NCUR)     | $(shell curl -s https://www.gnu.org/software/ncurses/ | grep "Announcing " | head -n1 | cut -d' ' -f5 | cut -d'<' -f1)
	# json    | $(V_JSON)   | $(shell curl -s https://api.github.com/repos/nlohmann/json/tags   | grep "name" | head -n1 | cut -d'v' -f2 | cut -d'"' -f1)
	# catch   | $(V_CATCH)  | $(shell curl -s https://api.github.com/repos/catchorg/Catch2/tags | grep "name" | head -n1 | cut -d'v' -f2 | cut -d'"' -f1)
	# uv      | $(V_UV)  | $(shell curl -s https://api.github.com/repos/libuv/libuv/tags        | grep "name" | head -n1 | cut -d'v' -f2 | cut -d'"' -f1)

zlib:
	test -d $(KBUILD)/zlib-$(V_ZLIB) || (                                             \
	curl -L https://zlib.net/zlib-$(V_ZLIB).tar.gz | tar xz -C $(KBUILD)              \
	&& cd $(KBUILD)/zlib-$(V_ZLIB) && (test -n "`echo $(CHOST) | grep mingw32`"       \
	&& DESTDIR=$(KBUILD)/local/ BINARY_PATH=bin INCLUDE_PATH=include LIBRARY_PATH=lib \
	make all install -fwin32/Makefile.gcc PREFIX=$(CHOST)-                            \
	|| (./configure --static --prefix=$(KBUILD)/local && make all install))           )

openssl:
	test -d $(KBUILD)/openssl-$(V_SSL) || (                                                   \
	curl -L https://www.openssl.org/source/openssl-$(V_SSL).tar.gz | tar xz -C $(KBUILD)      \
	&& cd $(KBUILD)/openssl-$(V_SSL) && CC=gcc ./Configure no-asm                             \
	$(shell test -n "`echo $(CHOST) | grep mingw32`" && echo mingw64 || echo gcc)             \
	--cross-compile-prefix=$(CHOST)- --prefix=$(KBUILD)/local --openssldir=$(KBUILD)/local    \
	&& make all install_sw install_ssldirs                                                    )

curl:
	test -d $(KBUILD)/curl-$(V_CURL) || (                                                   \
	curl -L https://curl.haxx.se/download/curl-$(V_CURL).tar.gz | tar xz -C $(KBUILD)       \
	&& cd $(KBUILD)/curl-$(V_CURL)                                                          \
	&& PKG_CONFIG="pkg-config --static" PKG_CONFIG_PATH=$(KBUILD)/local/lib/pkgconfig       \
	./configure --host=$(CHOST) --prefix=$(KBUILD)/local                                    \
	--with-ca-bundle=/etc/ssl/certs/ca-certificates.crt                                     \
	--with-zlib=$(KBUILD)/local --with-ssl=$(KBUILD)/local --disable-ares                   \
	--disable-manual --disable-imap --disable-file --disable-tftp --disable-smtp            \
	--disable-pop3 --disable-smb --disable-ftp --disable-sspi --disable-ldap --disable-dict \
	--without-libpsl --without-nghttp2 --without-libssh2 --disable-telnet --disable-rtsp    \
	--disable-gopher --without-libidn2 --without-librtmp --disable-shared --enable-static   \
	&& sed -i "/HAVE_GETEUID/d" lib/curl_config.h && make all install                       )

sqlite:
	test -d $(KBUILD)/sqlite-autoconf-$(V_SQL) || (                                       \
	curl -L https://sqlite.org/2020/sqlite-autoconf-$(V_SQL).tar.gz | tar xz -C $(KBUILD) \
	&& cd $(KBUILD)/sqlite-autoconf-$(V_SQL) && ./configure --prefix=$(KBUILD)/local      \
	--host=$(CHOST) --enable-static --disable-dynamic-extensions --disable-shared         \
	--enable-threadsafe && make all install                                               )

ncurses:
	test -d $(KBUILD)/ncurses-$(V_NCUR) || (                                                          \
	curl -L http://ftp.gnu.org/pub/gnu/ncurses/ncurses-$(V_NCUR).tar.gz | tar xz -C $(KBUILD)         \
	&& cd $(KBUILD)/ncurses-$(V_NCUR) && CPPFLAGS=-P ./configure                                      \
	--host=$(CHOST) --prefix=$(KBUILD)/local $(shell test -n "`echo $(CHOST) | grep mingw32`" && echo \
	--without-cxx-binding --without-ada --enable-reentrant --with-normal                              \
	--disable-home-terminfo --enable-sp-funcs --enable-term-driver --enable-interop || :)             \
	--disable-lib-suffixes --without-debug --without-progs --without-tests                            \
	--with-fallbacks=linux,screen,vt100,xterm,xterm-256color,putty-256color && make all install       \
	&& (test -n "`echo $(CHOST) | grep mingw32`"                                                      \
	&& sed -i '1 i\#undef KEY_EVENT' $(KBUILD)/local/include/ncurses/curses.h || :)                   )

json:
	test -f $(KBUILD)/local/include/json.h || (mkdir -p $(KBUILD)/local/include       \
	&& curl -L https://github.com/nlohmann/json/releases/download/v$(V_JSON)/json.hpp \
	-o $(KBUILD)/local/include/json.h &&                                              \
	sed -i '$$iusing namespace nlohmann;' $(KBUILD)/local/include/json.h &&           \
	(test -n "`echo $(CHOST) | grep mingw32`" &&                                      \
	sed -i "s/\(#include <cstddef>\)/\/\/\1/" $(KBUILD)/local/include/json.h ||     :))

catch:
	test -f $(KBUILD)/local/include/catch.h || (mkdir -p $(KBUILD)/local/include          \
	&& curl -L https://github.com/catchorg/Catch2/releases/download/v$(V_CATCH)/catch.hpp \
	-o $(KBUILD)/local/include/catch.h                                                    )

uv:
	test -z "`echo $(CHOST) | grep darwin;echo $(CHOST) | grep mingw32`" || test -d $(KBUILD)/libuv-$(V_UV) || ( \
	curl -L https://github.com/libuv/libuv/archive/v$(V_UV).tar.gz | tar xz -C $(KBUILD)                         \
	&& cd $(KBUILD)/libuv-$(V_UV) && sh autogen.sh && CC=$(CHOST)-clang ./configure --host=$(CHOST)              \
	--prefix=$(KBUILD)/local && make all install                                                                 )

pvs:
ifndef V_PVS
	$(MAKE) $@ V_PVS=$(shell curl -s https://www.viva64.com/en/pvs-studio-download/ | grep x86_64.tgz | sed 's/.*href=\"\(.*\)\" .*/\1/' | cut -d '-' -f3)
else
	test -d $(KBUILD)/pvs-studio-$(V_PVS)-x86_64 || (                \
	curl -L http://files.viva64.com/pvs-studio-$(V_PVS)-x86_64.tgz   \
	| tar xz -C $(KBUILD) && cd $(KBUILD)/pvs-studio-$(V_PVS)-x86_64 \
	&& chmod +x install.sh && sudo ./install.sh                      )
endif

lcov:
ifndef V_LCOV
	$(MAKE) $@ V_LCOV=$(shell curl -s https://api.github.com/repos/linux-test-project/lcov/releases/latest | grep name | grep -v 'tag\|lcov' | cut -d '"' -f4)
else
	test -d $(KBUILD)/lcov-$(V_LCOV) || (                                                                 \
	curl -L https://github.com/linux-test-project/lcov/releases/download/v$(V_LCOV)/lcov-$(V_LCOV).tar.gz \
	| tar xz -C $(KBUILD) && cd $(KBUILD)/lcov-$(V_LCOV) && sudo make install                             )
endif

.PHONY: all dist clean check zlib openssl curl sqlite ncurses json catch uv lcov pvs
