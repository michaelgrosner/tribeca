ERR     := *** Unexpected MAKELEVEL=0
HINT    := This Makefile can't be used directly, consider cd ../.. before try again
$(if $(shell test ${MAKELEVEL} -eq 0 || echo 1),,$(warning $(ERR));$(error $(HINT)))

NPM     := $(abspath ./node_modules)
PATH    := $(NPM)/.bin:$(PATH)
KLOCAL  := $(abspath ../../build-$(CHOST)/local)

STEP      = $(shell tput setaf 2;tput setab 0)[X/5]$(shell tput sgr0)
STEP_1   := $(subst X,1,$(STEP)) Building npm packages..
STEP_2   := $(subst X,2,$(STEP)) Building client dynamic files..
STEP_3   := $(subst X,3,$(STEP)) Building client static files..
STEP_4   := $(subst X,4,$(STEP)) Building client CSS files..
STEP_5   := $(subst X,5,$(STEP)) Building client bundle zip and docroot lib..

all trading-bot: bundle

docroot:
	cd $(KLOCAL) && $(CHOST)-g++ -c $(realpath trading-bot.S) -o $(KLOCAL)/lib/K-$(CHOST)-incbin.o

npm:
	$(info $(STEP_1))
	@$(MAKE) package.json
	@npm install
ifndef TRAVIS_DEPLOY
	@rm package*.json
endif

client: npm client/main.ts
	rm -rf $(KLOCAL)/var
	mkdir -p $(KLOCAL)/var/www
	$(info $(STEP_2))
	tsc --alwaysStrict --experimentalDecorators -t ES2017 -m commonjs --outDir $(KLOCAL)/var/www/js client/*.ts

www: www/index.html
	$(info $(STEP_3))
	cp -R www/* $(KLOCAL)/var/www

css: www/sass/*.scss
	$(info $(STEP_4))
	rm -rf $(KLOCAL)/var/www/css
	mkdir -p $(KLOCAL)/var/www/css
	ls -1 www/sass/*.scss | sed -r 's/(.*)(\.scss)$$/\1\2 \1\.min.css/' | xargs -I % sh -c 'sass %;' \
		&& rm www/sass/*.min.css.map && mv www/sass/*.min.css $(KLOCAL)/var/www/css/                   \
		&& cat $(NPM)/ag-grid/dist/styles/ag-grid.css >> $(KLOCAL)/var/www/css/bootstrap.min.css

bundle: client www css
	$(info $(STEP_5))
	mkdir -p $(KLOCAL)/var/www/js/client
	NODE_PATH=$(NPM) browserify -t [ babelify --presets [ babili env ] ] \
		$(KLOCAL)/var/www/js/main.js | uglifyjs | gzip > $(KLOCAL)/var/www/js/client/bundle.min.js
	rm -rf $(KLOCAL)/var/www/js/*.js $(KLOCAL)/var/www/sass
	echo $(CARCH) | tr ' ' "\n" | xargs -I % echo % | grep -v $(CHOST) \
		| xargs -I % sh -c 'if test -d ../../build-%; then rm -rf ../../build-%/local/var;mkdir -p ../../build-%/local/var;cp -R $(KLOCAL)/var ../../build-%/local; fi'
	echo $(CARCH) | tr ' ' "\n" | xargs -I % echo % \
		| xargs -I % sh -c 'if test -d ../../build-%; then CHOST=% $(MAKE) docroot; fi'

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
    "rxjs": "6.3.1",
    "rxjs-compat": "^6.3.1",
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

.PHONY: all trading-bot docroot client www css bundle npm package.json
