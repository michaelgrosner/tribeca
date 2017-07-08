G_SHARED_ARGS := -lsqlite3 -DUSE_LIBUV -std=c++11 -O3 -shared -fPIC \
   -Ibuild/node-$(NODEv)/include/node \
   -Ibuild/uWebSockets-$(UWSv)/src              build/uWebSockets-$(UWSv)/src/Extensions.cpp \
   build/uWebSockets-$(UWSv)/src/Group.cpp      build/uWebSockets-$(UWSv)/src/Networking.cpp \
   build/uWebSockets-$(UWSv)/src/Hub.cpp        build/uWebSockets-$(UWSv)/src/Node.cpp       \
   build/uWebSockets-$(UWSv)/src/WebSocket.cpp  build/uWebSockets-$(UWSv)/src/HTTPSocket.cpp \
   build/uWebSockets-$(UWSv)/src/Socket.cpp     build/uWebSockets-$(UWSv)/src/Epoll.cpp      \
src/lib/K.cc

all: K

help:
	#
	# Available commands inside K directory:
	#   make help                      - show this help
	#
	#   make                           - compile K node module
	#   make K                         - compile K node module
	#   make node                      - download node src files
	#   make uws                       - download uws src files
	#   make clean                     - remove external src files

K:
	mkdir -p build app/server/lib
	NODEv=v7.1.0 ABIv=51 UWSv=0.14.3 $(MAKE) node uws `(uname -s)`
#	NODEv=v8.1.2 ABIv=57 UWSv=0.14.3 $(MAKE) node uws `(uname -s)`
	for K in app/server/lib/K*node; do chmod +x $$K; done
	# SUCCESS

node:
ifndef NODEv
	@NODEv=v7.1.0 $(MAKE) $@
	@NODEv=v8.1.2 $(MAKE) $@
else
	test -d build/node-$(NODEv) || curl https://nodejs.org/dist/$(NODEv)/node-$(NODEv)-headers.tar.gz | tar xz -C build
endif

uws:
ifndef UWSv
	@UWSv=0.14.3 $(MAKE) $@
else
	test -d build/uWebSockets-$(UWSv) || curl -L https://github.com/uNetworking/uWebSockets/archive/v$(UWSv).tar.gz | tar xz -C build
endif

Linux:
	g++ $(G_SHARED_ARGS) -static-libstdc++ -static-libgcc -s -o app/server/lib/K.linux.$(ABIv).node

Darwin:
	g++ $(G_SHARED_ARGS) -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup -o app/server/lib/K.darwin.$(ABIv).node

clean:
	rm -rf build

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: build K uws node Linux Darwin clean asandwich