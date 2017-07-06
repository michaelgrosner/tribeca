G_SHARED_ARGS := -lsqlite3 -DUSE_LIBUV -std=c++11 -O3 -shared -fPIC \
    -I /usr/include/uWS             /usr/include/uWS/Extensions.cpp \
    /usr/include/uWS/Group.cpp      /usr/include/uWS/Networking.cpp \
    /usr/include/uWS/Hub.cpp        /usr/include/uWS/Node.cpp       \
    /usr/include/uWS/WebSocket.cpp  /usr/include/uWS/HTTPSocket.cpp \
    /usr/include/uWS/Socket.cpp     /usr/include/uWS/Epoll.cpp      \
src/lib/K.cc

all: build

help:
	#
	# Available commands inside K directory:
	#   make help                      - show this help
	#
	#   make                           - compile K node module
	#   make build                     - compile K node module
	#   make target                    - download external src files
	#   make clean                     - remove external src files

build:
	mkdir -p build/targets
	NODEv=v7.1.0 ABIv=51 $(MAKE) `(uname -s)`
#	NODEv=v8.1.2 ABIv=57 $(MAKE) `(uname -s)`
	for K in app/server/lib/K*node; do chmod +x $$K; done

target:
ifndef NODEv
	@NODEv=v7.1.0 $(MAKE) $@
	@NODEv=v8.1.2 $(MAKE) $@
else
	curl https://nodejs.org/dist/$(NODEv)/node-$(NODEv)-headers.tar.gz | tar xz -C build/targets
endif

Linux:
	$(MAKE) target
	g++ $(G_SHARED_ARGS) -static-libstdc++ -static-libgcc -I build/targets/node-$(NODEv)/include/node -s -o app/server/lib/K.linux.$(ABIv).node

Darwin:
	$(MAKE) target
	g++ $(G_SHARED_ARGS) -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup -I build/targets/node-$(NODEv)/include/node -o app/server/lib/K.darwin.$(ABIv).node

clean:
	rm -rf build/targets

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: build target Linux Darwin clean asandwich