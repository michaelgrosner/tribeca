V_UWS := 0.14.3
V_QF  := v.1.14.4
G_SHARED_ARGS := -lsqlite3 -DUSE_LIBUV -O3 -shared -fPIC -Ibuild/node-$(NODEv)/include/node    \
   -Ibuild/uWebSockets-$(V_UWS)/src              build/uWebSockets-$(V_UWS)/src/Extensions.cpp \
   build/uWebSockets-$(V_UWS)/src/Group.cpp      build/uWebSockets-$(V_UWS)/src/Networking.cpp \
   build/uWebSockets-$(V_UWS)/src/Hub.cpp        build/uWebSockets-$(V_UWS)/src/Node.cpp       \
   build/uWebSockets-$(V_UWS)/src/WebSocket.cpp  build/uWebSockets-$(V_UWS)/src/HTTPSocket.cpp \
   build/uWebSockets-$(V_UWS)/src/Socket.cpp     build/uWebSockets-$(V_UWS)/src/Epoll.cpp      \
-std=c++11 src/lib/K.cc

all: K

help:
	#
	# Available commands inside K top level directory:
	#   make help                      - show this help
	#
	#   make                           - compile K node module
	#   make K                         - compile K node module
	#   make node                      - download node src files
	#   make uws                       - download uws src files
	#   make quickfix                  - download quickfix src files
	#   make clean                     - remove external src files

K:
	mkdir -p build app/server/lib
	$(MAKE) uws
	$(MAKE) quickfix
	NODEv=v7.1.0 ABIv=51 $(MAKE) node `(uname -s)`
	NODEv=v8.1.2 ABIv=57 $(MAKE) node `(uname -s)`
	for K in app/server/lib/K*node; do chmod +x $$K; done

node:
ifndef NODEv
	@NODEv=v7.1.0 $(MAKE) $@
	@NODEv=v8.1.2 $(MAKE) $@
else
	test -d build/node-$(NODEv) || curl https://nodejs.org/dist/$(NODEv)/node-$(NODEv)-headers.tar.gz | tar xz -C build
endif

uws:
	test -d build/uWebSockets-$(V_UWS) || curl -L https://github.com/uNetworking/uWebSockets/archive/v$(V_UWS).tar.gz | tar xz -C build

quickfix:
	(test -f /usr/local/lib/libquickfix.so || test -f /usr/local/lib/libquickfix.dylib) || ( \
		curl -L https://github.com/quickfix/quickfix/archive/$(V_QF).tar.gz | tar xz -C build  \
		&& cd build/quickfix-$(V_QF) && ./bootstrap && ./configure && make                     \
		&& sudo make install && sudo cp config.h /usr/local/include/quickfix/                  \
		&& (test -f /sbin/ldconfig && sudo ldconfig || :)                                      \
	)

Linux:
	g++ $(G_SHARED_ARGS) -static-libstdc++ -static-libgcc -s -o app/server/lib/K.linux.$(ABIv).node

Darwin:
	g++ $(G_SHARED_ARGS) -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup -o app/server/lib/K.darwin.$(ABIv).node

clean:
	rm -rf build

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: K uws node Linux Darwin clean asandwich