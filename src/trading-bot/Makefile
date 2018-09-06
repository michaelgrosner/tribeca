ERR     := *** Unexpected MAKELEVEL = 0.
HINT    := This Makefile can't be used directly, consider cd ../.. before try again
$(if $(shell test ${MAKELEVEL} -eq 0 || echo 1),,$(warning $(ERR));$(error $(HINT)))
ERR     := *** Unexpected MAKE_VERSION < 4.
HINT    := This Makefile can't be used with your make version, consider to upgrade before try again
$(if $(shell test "`echo ${MAKE_VERSION} | cut -d. -f1`" -lt 4 || echo 1),,$(warning $(ERR));$(error $(HINT)))

KLOCAL    := $(realpath ../../build-$(CHOST)/local)
NODE_PATH := $(abspath ./node_modules)
PATH      := $(NODE_PATH)/.bin:$(PATH)

STEPS     := $(shell grep "call\ STEP" Makefile | wc -l)
STEP       = $(shell tput setaf 2;tput setab 0)[$(1)/$(STEPS)]$(shell tput sgr0)
STEP_1    := $(call STEP,1) Building npm packages..
STEP_2    := $(call STEP,2) Building client dynamic files..
STEP_3    := $(call STEP,3) Building client CSS files..
STEP_4    := $(call STEP,4) Building js bundle zip and document root lib..

all trading-bot: client css js

assets: trading-bot.S
	$(CHOST)-g++ -Wa,-I$(realpath $(KLOCAL)/var),-I$(realpath ./) -c $^ -o $(KLOCAL)/lib/K-$(CHOST)-assets.o

npm:
	$(info $(STEP_1))
	@$(MAKE) package.json
	@npm install
ifndef TRAVIS_DEPLOY
	@rm package*.json
endif

client: npm client/main.ts
	$(info $(STEP_2))
	rm -rf   $(KLOCAL)/var
	mkdir -p $(KLOCAL)/var/www
	tsc --alwaysStrict --experimentalDecorators -t ES2017 -m commonjs --outDir $(KLOCAL)/var/www/js client/*.ts

css: www/sass/*.scss
	$(info $(STEP_3))
	rm -rf   $(KLOCAL)/var/www/css
	mkdir -p $(KLOCAL)/var/www/css
	cp -R www/sass $(KLOCAL)/var/www
	ls -1 www/sass/*.scss | sed -r 's/(.*)(\.scss)$$/\1\2 \1\.min.css/' | xargs -I % sh -c 'sass %;' \
	  && rm www/sass/*.min.css.map && mv www/sass/*.min.css $(KLOCAL)/var/www/css/                   \
	  && cat $(NODE_PATH)/ag-grid/dist/styles/ag-grid.css >> $(KLOCAL)/var/www/css/bootstrap.min.css
	rm -rf $(KLOCAL)/var/www/sass

js: $(NODE_PATH)/.bin
	$(info $(STEP_4))
	mkdir -p $(KLOCAL)/var/www/js/client
	NODE_PATH=$(NODE_PATH) browserify -t [ babelify --presets [ babili env ] ] \
	  $(KLOCAL)/var/www/js/main.js | uglifyjs | gzip > $(KLOCAL)/var/www/js/client/bundle.min.js
	rm -rf $(KLOCAL)/var/www/js/*.js
	echo $(CARCH) | tr ' ' "\n" | xargs -I % echo % | grep -v $(CHOST) \
	  | xargs -I % sh -c 'if test -d ../../build-%; then rm -rf ../../build-%/local/var;mkdir -p ../../build-%/local/var;cp -R $(KLOCAL)/var ../../build-%/local; fi'
	echo $(CARCH) | tr ' ' "\n" | xargs -I % echo % \
	  | xargs -I % sh -c 'if test -d ../../build-%; then CHOST=% $(MAKE) assets; fi'

export define PACKAGE_JSON
{
  "name": "K.UI",
  "version": "4.0.0",
  "description": "Self-hosted crypto trading bot (automated high frequency market making) in node.js, angular, typescript and c++",
  "dependencies": {
    "@angular/common": "^6.1.6",
    "@angular/compiler": "^6.1.6",
    "@angular/core": "^6.1.6",
    "@angular/forms": "^6.1.6",
    "@angular/http": "^6.1.6",
    "@angular/platform-browser": "^6.1.6",
    "@angular/platform-browser-dynamic": "^6.1.6",
    "@angular/router": "^6.1.6",
    "ag-grid": "^18.1.2",
    "ag-grid-angular": "^18.1.0",
    "angular2-highcharts": "^0.5.5",
    "babel-core": "^6.26.3",
    "babel-preset-babili": "0.1.4",
    "babel-preset-env": "^1.7.0",
    "babelify": "^8.0.0",
    "browserify": "^16.2.2",
    "highcharts": "^6.1.1",
    "ng4-popover": "^1.0.23",
    "reflect-metadata": "^0.1.12",
    "rxjs": "6.3.2",
    "rxjs-compat": "^6.3.2",
    "sass": "^1.13.1",
    "typescript": "3.0.3",
    "uglify-es": "^3.3.9",
    "zone.js": "^0.8.26"
  },
  "homepage": "https://github.com/ctubio/Krypto-trading-bot",
  "bugs": "https://github.com/ctubio/Krypto-trading-bot/issues",
  "repository": {
    "type": "git",
    "url": "git@github.com:ctubio/Krypto-trading-bot.git"
  },
  "author": "Michael Grosner",
  "contributors": [
    {
      "name": "Carles Tubio"
    }
  ],
  "os": [
    "darwin",
    "linux"
  ],
  "license": "(ISC OR MIT)"
}
endef

package.json:
	@echo "$$PACKAGE_JSON" > $@

.PHONY: all trading-bot assets client css js npm package.json
