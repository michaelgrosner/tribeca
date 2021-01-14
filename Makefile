K         ?= K.sh
MAJOR      = 0
MINOR      = 5
PATCH      = 4
BUILD      = 40

OBLIGATORY = DISCLAIMER: This is strict non-violent software: \
           \nif you hurt other living creatures, please stop; \
           \notherwise remove all copies of the software now.

PERMISSIVE = This is free software: the UI and quoting engine are open source, \
           \nfeel free to hack both as you need.                               \
                                                                               \
           \nThis is non-free software: built-in gateway exchange integrations \
           \nare licensed by/under the law of my grandma (since last century), \
           \nfeel free to crack all as you need.

SOURCE    := $(notdir $(wildcard src/bin/*))
CARCH      = x86_64-linux-gnu      \
             arm-linux-gnueabihf   \
             aarch64-linux-gnu     \
             x86_64-apple-darwin17 \
             x86_64-w64-mingw32

CHOST     ?= $(shell test -n "`command -v g++`" && g++ -dumpmachine \
               || echo $(subst build-,,$(firstword $(wildcard build-*))))
ABI       ?= $(shell echo '\#include <string>'             \
               | $(CHOST)-g++ -x c++ -dM -E - 2> /dev/null \
               | grep '_GLIBCXX_USE_CXX11_ABI 1'           \
               | wc -l                                     )

KHOST     := $(shell echo $(CHOST)                               \
               | sed 's/-\([a-z_0-9]*\)-\(linux\)$$/-\2-\1/'     \
               | sed 's/\([a-z_0-9]*\)-\([a-z_0-9]*\)-.*/\2-\1/' \
               | sed 's/^w64/win64/'                             )
KLOCAL    := build-$(KHOST)/local

ERR        = *** K require g++ v7 or greater, but it was not found.
HINT      := consider a symlink at /usr/bin/$(CHOST)-g++ pointing to your g++-7 or g++-8 executable
STEP       = $(shell tput setaf 2;tput setab 0)Building $(1)..$(shell tput sgr0)

KARGS     := -std=c++17 -O3 -pthread -D'K_HEAD="$(shell  \
    git rev-parse HEAD 2>/dev/null || echo HEAD          \
  )"' -D'K_BUILD="$(KHOST)"' -D'K_SOURCE="K-$(KSRC)"'    \
  -D'K_STAMP="$(shell date "+%Y-%m-%d %H:%M:%S")"'       \
  -D'K_0_DAY="v$(MAJOR).$(MINOR).$(PATCH)+$(BUILD)"'     \
  -I$(KLOCAL)/include $(addprefix $(KLOCAL)/lib/,        \
    K-$(KHOST).$(ABI).a                                  \
    libncurses.a                                         \
    libsqlite3.a                                         \
    libcurl.a                                            \
    libssl.a  libcrypto.a                                \
    libz.a                                               \
  ) $(wildcard $(addprefix $(KLOCAL)/lib/,               \
    K-$(KSRC)-assets.o                                   \
    libuv.dll.a libuv.a                                  \
  )) $(addprefix -include src/lib/Krypto.ninja-,         \
       $(addsuffix .h,                                   \
         lang                                            \
         data                                            \
         apis                                            \
         bots                                            \
       )                                                 \
  ) -D'DEBUG_FRAMEWORK="Krypto.ninja-test.h"'            \
    -D'DEBUG_SCENARIOS=<$(or                             \
      $(realpath src/bin/$(KSRC)/$(KSRC).test.h),        \
      /dev/null                                          \
    )>'                                                  \
    -D'using_Makefile(x)=<$(abspath                      \
      src/bin/$(KSRC)                                    \
    )/using_\#\#x>'                                      \
    -D'using_data=$(KSRC).data.h'                        \
    -D'using_main=$(KSRC).main.h'                        \
-D'OBLIGATORY_analpaper_SOFTWARE_LICENSE="$(OBLIGATORY)"'\
-D'PERMISSIVE_analpaper_SOFTWARE_LICENSE="$(PERMISSIVE)"'

all K: $(SOURCE)

hlep hepl help:
	#                                                  #
	# Available commands inside K top level directory: #
	#  make help         - show this help              #
	#                                                  #
	#  make              - compile K sources           #
	#  make K            - compile K sources           #
	#  KALL=1 make K     - compile K sources           #
	#  make trading-bot  - compile K sources           #
	#                                                  #
	#  make dist         - compile K dependencies      #
	#  KALL=1 make dist  - compile K dependencies      #
	#  make packages     - provide K dependencies      #
	#  make install      - install K application       #
	#  make docker       - install K application       #
	#  make reinstall    - upgrade K application       #
	#  make doc          - compile K documentation     #
	#  make test         - run tests                   #
	#  make test-c       - run static tests            #
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
	#  make download     - download K src precompiled  #
	#  make clean        - remove external src files   #
	#  KALL=1 make clean - remove external src files   #
	#  make cleandb      - remove databases            #
	#  make uninstall    - remove /usr/local/bin/K-*   #
	#                                                  #

doc test:
	@$(MAKE) -sC $@

clean check dist:
ifdef KALL
	unset KALL $(foreach chost,$(CARCH),&& $(MAKE) $@ CHOST=$(chost))
else
	$(if $(subst 8,,$(subst 7,,$(shell $(CHOST)-g++ -dumpversion | cut -d. -f1))),$(warning $(ERR));$(error $(HINT)))
	@$(MAKE) -C src/lib $@ CHOST=$(CHOST) KHOST=$(KHOST)
endif

$(SOURCE):
	$(info $(call STEP,$@))
	$(MAKE) $(shell ! test -f src/bin/$@/Makefile || echo assets) src KSRC=$@

assets: src/bin/$(KSRC)/Makefile
	$(info $(call STEP,$(KSRC) $@))
	$(MAKE) -C src/bin/$(KSRC)
	$(foreach chost,$(CARCH), \
	  build=build-$(shell echo $(chost) | sed 's/-\([a-z_0-9]*\)-\(linux\)$$/-\2-\1/' | sed 's/\([a-z_0-9]*\)-\([a-z_0-9]*\)-.*/\2-\1/' | sed 's/^w64/win64/') \
	  && ! test -d $${build} || ((test -d $${build}/local/assets \
	  || cp -R /var/lib/K/assets $${build}/local/assets)         \
	  && $(MAKE) assets.o CHOST=$(chost) chost=$(shell test -n "`command -v $(chost)-g++`" && echo $(chost)- || :) \
	  && rm -rf $${build}/local/assets) \
	;)
	rm -rf /var/lib/K/assets

assets.o: src/bin/$(KSRC)/$(KSRC).disk.S
	$(chost)g++ -Wa,-I,$(KLOCAL)/assets,-I,src/bin/$(KSRC) -c $^ \
	  -o $(KLOCAL)/lib/K-$(notdir $(basename $(basename $^)))-$@

src: src/lib/Krypto.ninja-main.cxx src/bin/$(KSRC)/$(KSRC).main.h
ifdef KALL
	unset KALL $(foreach chost,$(CARCH),&& $(MAKE) $@ CHOST=$(chost))
else
	$(info $(call STEP,$(KSRC) $@ $(CHOST)))
	$(if $(subst 8,,$(subst 7,,$(shell $(CHOST)-g++ -dumpversion | cut -d. -f1))),$(warning $(ERR));$(error $(HINT)))
	@$(CHOST)-g++ --version
	@mkdir -p $(KLOCAL)/bin
	-@egrep ₿ src test -lR | xargs -r sed -i 's/₿/\\u20BF/g'
	$(MAKE) $(shell test -n "`echo $(CHOST) | grep darwin`" && echo Darwin || (test -n "`echo $(CHOST) | grep mingw32`" && echo Win32 || uname -s)) CHOST=$(CHOST)
	-@egrep \\u20BF src test -lR | xargs -r sed -i 's/\\u20BF/₿/g'
	@chmod +x $(KLOCAL)/bin/K-$(KSRC)*
	@$(MAKE) system_install -s
endif

Linux: src/lib/Krypto.ninja-main.cxx src/bin/$(KSRC)/$(KSRC).main.h
ifdef TRAVIS_OS_NAME
	@unset TRAVIS_OS_NAME && $(MAKE) KCOV="--coverage" $@
else ifdef KUNITS
	@unset KUNITS && $(MAKE) KTEST="$(KCOV) -DCATCH_CONFIG_FAST_COMPILE test/unit_testing_framework.cxx" $@
else ifndef KTEST
	@$(MAKE) KTEST="-DNDEBUG" $@
else
	$(CHOST)-g++ -s $(KTEST) -o $(KLOCAL)/bin/K-$(KSRC) \
	  -static-libstdc++ -static-libgcc -rdynamic        \
	  $< $(KARGS) -ldl -Wall -Wextra
endif

Darwin: src/lib/Krypto.ninja-main.cxx src/bin/$(KSRC)/$(KSRC).main.h
	-@egrep \\u20BF src -lR | xargs -r sed -i 's/\\\(u20BF\)/\1/g'
	$(CHOST)-g++ -s -DNDEBUG -o $(KLOCAL)/bin/K-$(KSRC)                          \
	  -msse4.1 -maes -mpclmul -mmacosx-version-min=10.13 -nostartfiles -rdynamic \
	  $< $(KARGS) -ldl
	-@egrep u20BF src -lR | xargs -r sed -i 's/\(u20BF\)/\\\1/g'

Win32: src/lib/Krypto.ninja-main.cxx src/bin/$(KSRC)/$(KSRC).main.h
	$(CHOST)-g++-posix -s -DNDEBUG -o $(KLOCAL)/bin/K-$(KSRC).exe \
	  -D_POSIX -DCURL_STATICLIB -DSIGUSR1=SIGABRT                 \
	  $< $(KARGS)                                                 \
	  -static -lstdc++ -lgcc -lwldap32 -lws2_32

download:
	curl -L https://github.com/ctubio/Krypto-trading-bot/releases/download/$(MAJOR).$(MINOR).x/K-$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(KHOST).tar.gz | tar xz
	@$(MAKE) system_install -s
	@test -n "`ls *.sh 2>/dev/null`" || (cp etc/K.sh.dist K.sh && chmod +x K.sh)
	@$(MAKE) upgrade_old_installations -s

upgrade_old_installations:
	-@$(foreach json,$(wildcard /var/lib/K/cache/handshake.*), rm $(json) || :;)
	-@test "1" = "$(ABI)" || (echo                                                            \
	&& echo This app will crash because was compiled with CXX11 ABI, missing in your system.. \
	&& echo A temporary solution is to recompile the app in your own system with: make dist K \
	&& echo A permanent solution is to upgrade your OS to a newer version.                    )

cleandb: /var/lib/K/db/K*
	rm -rf /var/lib/K/db/K*.db

packages:
	test -n "`command -v apt-get`" && sudo apt-get -y install g++ build-essential automake autoconf libtool libxml2 libxml2-dev zlib1g-dev python curl gzip screen doxygen graphviz \
	|| (test -n "`command -v yum`" && sudo yum -y install gcc-c++ automake autoconf libtool libxml2 libxml2-devel python curl gzip screen) \
	|| (test -n "`command -v brew`" && (xcode-select --install || :) && (brew install automake autoconf libxml2 zlib python curl gzip screen proctools doxygen graphviz || brew upgrade || :)) \
	|| (test -n "`command -v pacman`" && sudo pacman --noconfirm -S --needed base-devel libxml2 zlib curl python gzip screen)

uninstall:
	rm -vrf /var/lib/K/cache /var/lib/K/node_modules
	@$(foreach bin,$(addprefix /usr/local/bin/,$(notdir $(wildcard $(KLOCAL)/bin/K-*))), sudo rm -v $(bin);)

system_install:
	$(info Checking if sudo           is allowed  at /usr/local/bin.. $(shell sudo ls -ld /usr/local/bin > /dev/null 2>&1 && echo OK || echo ERROR))
	$(info Checking if /usr/local/bin is already  in your PATH..      $(if $(shell echo $$PATH | grep /usr/local/bin),OK))
	$(if $(shell echo $$PATH | grep /usr/local/bin),,$(info $(subst ..,,$(subst Building ,,$(call STEP,Warning! you MUST add /usr/local/bin to your PATH!)))))
	$(info Checking if /etc/ssl/certs is readable by curl..           $(shell (test -d /etc/ssl/certs && echo OK) || (sudo mkdir -p /etc/ssl/certs && echo OK)))
	$(info Checking if /var/lib/K/db  is writable by sqlite..         $(shell (test -d /var/lib/K/db && echo OK) || (sudo mkdir -p /var/lib/K/db && sudo chown -R $(shell id -u) /var/lib/K && echo OK)))
	$(info )
	$(info List of installed K binaries:)
	@sudo cp -f $(wildcard $(KLOCAL)/bin/K-$(KSRC)*) /usr/local/bin
	@LS_COLORS="ex=40;92" CLICOLOR="Yes" ls $(shell ls --color > /dev/null 2>&1 && echo --color) -lah $(addprefix /usr/local/bin/,$(notdir $(wildcard $(KLOCAL)/bin/K-$(KSRC)*)))
	@echo
	@sudo curl -s --time-cond /etc/ssl/certs/ca-certificates.crt https://curl.haxx.se/ca/cacert.pem -o /etc/ssl/certs/ca-certificates.crt
	@mkdir -p /var/lib/K/cache

install: packages
	@yes = | head -n`expr $(shell tput cols) / 2` | xargs echo && echo " _  __\n| |/ /  v$(MAJOR).$(MINOR).$(PATCH)+$(BUILD)\n| ' /\n| . \\   Select your (beloved) architecture\n|_|\\_\\  to download pre-compiled binaries:\n"
	@echo $(CARCH) | tr ' ' "\n" | cat -n && echo "\n(Hint! uname says \"`uname -sm`\", and win32 auto-install does not work yet)\n"
	@read -p "[$(shell seq -s / `echo $(CARCH) | tr ' ' "\n" | wc -l`)]: " chost && $(MAKE) download CHOST=`echo $(CARCH) | cut -d ' ' -f$${chost}`

docker: packages download
	@sed -i "/Usage/,+86d" K.sh

reinstall:
	test -d .git && ((test -n "`git diff`" && (echo && echo !!Local changes will be lost!! press CTRL-C to abort. && echo && sleep 5) || :) \
	&& git fetch && git merge FETCH_HEAD || (git reset FETCH_HEAD && git checkout .)) || curl https://raw.githubusercontent.com/ctubio/Krypto-trading-bot/master/Makefile > Makefile
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
	@test -n "`screen -list | grep "\.$(K)	("`"         \
	&& (echo $(K) is already running.. && screen -list)  \
	|| (screen -dmS $(K) ./$(K) && echo START $(K) DONE)

screen:
	@test -n "`screen -list | grep "\.$(K)	("`" && (    \
	echo Detach screen hotkey: holding CTRL hit A then D \
	&& sleep 2 && screen -r $(K)) || screen -list || :

diff: .git
	@_() { echo $$2 $$3 version: `git rev-parse $$1`; }; git remote update && _ @ Local running && _ @{u} Latest remote
	@$(MAKE) changelog -s

latest: .git diff
	@_() { git rev-parse $$1; }; test `_ @` != `_ @{u}` && $(MAKE) reinstall || :

changelog: .git
	@_() { echo `git rev-parse $$1`; }; echo && git --no-pager log --graph --oneline @..@{u} && test `_ @` != `_ @{u}` || echo No need to upgrade, both versions are equal.

test-c:
ifndef KSRC
	@$(foreach src,$(SOURCE),$(MAKE) -s $@ KSRC=$(src);)
else
	@pvs-studio-analyzer credentials PVS-Studio Free FREE-FREE-FREE-FREE > /dev/null 2>&1
	@pvs-studio-analyzer analyze -e src/bin/$(KSRC)/$(KSRC).test.h -e src/lib/Krypto.ninja-test.h -e $(KLOCAL)/include --source-file test/static_code_analysis.cxx --cl-params $(KARGS) test/static_code_analysis.cxx && \
	  (echo $(KSRC) `plog-converter -a GA:1,2 -t tasklist -o report.tasks PVS-Studio.log | tail -n+8 | sed '/Total messages/d'` && cat report.tasks | sed '/Help: The documentation/d' && rm report.tasks) || :
	@clang-tidy -header-filter=$(realpath src) -checks='modernize-*' test/static_code_analysis.cxx -- $(KARGS) 2> /dev/null
	@rm -f PVS-Studio.log > /dev/null 2>&1
endif

#png: etc/${PNG}.png etc/${PNG}.json
#	convert etc/${PNG}.png -set "K.conf" "`cat etc/${PNG}.json`" K: etc/${PNG}.png 2>/dev/null || :
#	@$(MAKE) png-check -s

#png-check: etc/${PNG}.png
#	@test -n "`identify -verbose etc/${PNG}.png | grep 'K\.conf'`" && echo Configuration injected into etc/${PNG}.png OK, feel free to remove etc/${PNG}.json anytime. || echo nope, injection failed.

push:
	@date=`date` && (git diff || :) && git status && read -p "KMOD: " KMOD \
	&& git add . && git commit -S -m "$${KMOD}"                            \
	&& ((KALL=1 $(MAKE) K doc release && git push) || git reset HEAD^1)    \
	&& echo $${date} && date

MAJOR:
	@sed -i "s/^\(MAJOR *=\).*$$/\1 $(shell expr $(MAJOR) + 1)/" Makefile
	@sed -i "s/^\(MINOR *=\).*$$/\1 0/"                          Makefile
	@sed -i "s/^\(PATCH *=\).*$$/\1 0/"                          Makefile
	@sed -i "s/^\(BUILD *=\).*$$/\1 0/"                          Makefile
	@$(MAKE) push

MINOR:
	@sed -i "s/^\(MINOR *=\).*$$/\1 $(shell expr $(MINOR) + 1)/" Makefile
	@sed -i "s/^\(PATCH *=\).*$$/\1 0/"                          Makefile
	@sed -i "s/^\(BUILD *=\).*$$/\1 0/"                          Makefile
	@$(MAKE) push

PATCH:
	@sed -i "s/^\(PATCH *=\).*$$/\1 $(shell expr $(PATCH) + 1)/" Makefile
	@sed -i "s/^\(BUILD *=\).*$$/\1 0/"                          Makefile
	@$(MAKE) push

BUILD:
	@sed -i "s/^\(BUILD *=\).*$$/\1 $(shell expr $(BUILD) + 1)/" Makefile
	@$(MAKE) push

release:
ifdef KALL
	unset KALL $(foreach chost,$(CARCH),&& $(MAKE) $@ CHOST=$(chost))
else ifndef KTARGZ
	@$(MAKE) KTARGZ="K-$(MAJOR).$(MINOR).$(PATCH).$(BUILD)-$(KHOST).tar.gz" $@
else
	@tar -cvzf $(KTARGZ) $(KLOCAL)/bin/K-* $(KLOCAL)/lib/K-* LICENSE COPYING README.md Makefile doc etc test                      \
	$(shell test -n "`echo $(CHOST) | grep mingw32`" && echo $(KLOCAL)/bin/*dll || :) src                                         \
	&& curl -s -n -H "Content-Type:application/octet-stream" -H "Authorization: token ${KRELEASE}"                                \
	--data-binary "@$(PWD)/$(KTARGZ)" "https://uploads.github.com/repos/ctubio/Krypto-trading-bot/releases/$(shell curl -s        \
	https://api.github.com/repos/ctubio/Krypto-trading-bot/releases/latest | grep id | head -n1 | cut -d ' ' -f4 | cut -d ',' -f1 \
	)/assets?name=$(KTARGZ)" && rm -v $(KTARGZ)
endif

md5: src
	find src/lib -type f -exec md5sum "{}" + > src/lib.md5

asandwich:
	@test `whoami` = 'root' && echo OK || echo make it yourself!

.PHONY: all K $(SOURCE) hlep hepl help doc test src assets assets.o clean check dist download cleandb list screen start stop restart startall stopall restartall packages system_install uninstall install docker reinstall diff latest changelog test-c push MAJOR MINOR PATCH BUILD release md5 asandwich
