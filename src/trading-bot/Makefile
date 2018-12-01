ERR        = *** Unexpected MAKELEVEL = 0.
HINT       = This Makefile can't be used directly, consider cd ../.. before try again
$(if $(subst 0,,${MAKELEVEL}),,$(warning $(ERR));$(error $(HINT)))
ERR        = *** Unexpected MAKE_VERSION < 4.
HINT       = This Makefile can't be used with your make version, consider to upgrade before try again
$(if $(shell ver=${MAKE_VERSION} && test $${ver%%.*} -lt 4 || echo 1),,$(warning $(ERR));$(error $(HINT)))

NODE_PATH := $(abspath ./node_modules)
PATH      := $(NODE_PATH)/.bin:$(PATH)

STEPS     := $(shell grep "call\ STEP" Makefile | wc -l)
STEP       = $(shell tput setaf 2;tput setab 0)[$(1)/$(STEPS)]$(shell tput setaf 3;tput setab 0) Building client $(2)..$(shell tput sgr0)
STEP_1    := $(call STEP,1,npm packages)
STEP_2    := $(call STEP,2,ts files)
STEP_3    := $(call STEP,3,js zip file)
STEP_4    := $(call STEP,4,css files)

all $(KSRC): js css

npm: package.json
	$(info $(STEP_1))
	@$@ install
ifndef TRAVIS_DEPLOY
	@rm $(basename $^)*
endif

client: npm client/main.ts
	$(info $(STEP_2))
	tsc --alwaysStrict --experimentalDecorators -t ES2017 -m commonjs --outDir $(KASSETS)/www/js client/*.ts

js: client $(NODE_PATH)/.bin
	$(info $(STEP_3))
	NODE_PATH=$(NODE_PATH) browserify -t [ babelify --presets [ @babel/preset-env ] ] \
	  $(KASSETS)/www/js/main.js | uglifyjs | gzip > $(KASSETS)/www/js/client.min.js

css: www/sass/*.scss
	$(info $(STEP_4))
	$(foreach scss,$(wildcard www/sass/*.scss), \
	  sass $(scss) $(KASSETS)/www/css/$(basename $(notdir $(scss))).min.css ;)
	cat $(NODE_PATH)/ag-grid/dist/styles/ag-grid.css >> $(KASSETS)/www/css/bootstrap.min.css

export define PACKAGE_JSON
{
  "dependencies": {
    "@angular/common": "^7.1.1",
    "@angular/compiler": "^7.1.1",
    "@angular/core": "^7.1.1",
    "@angular/forms": "^7.1.1",
    "@angular/http": "^7.1.1",
    "@angular/platform-browser": "^7.1.1",
    "@angular/platform-browser-dynamic": "^7.1.1",
    "@angular/router": "^7.1.1",
    "@babel/core": "^7.1.6",
    "@babel/preset-env": "^7.1.6",
    "ag-grid": "^18.1.2",
    "ag-grid-angular": "^19.1.2",
    "ag-grid-community": "^19.1.4",
    "angular2-highcharts": "^0.5.5",
    "babelify": "^10.0.0",
    "browserify": "^16.2.3",
    "highcharts": "^6.2.0",
    "ng4-popover": "^1.0.23",
    "reflect-metadata": "^0.1.12",
    "rxjs": "6.3.3",
    "rxjs-compat": "^6.3.3",
    "sass": "^1.15.1",
    "typescript": "^3.2.1",
    "uglify-es": "^3.3.9",
    "zone.js": "^0.8.26"
  },
  "description": "K",
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
    "linux",
    "win32"
  ],
  "license": "(ISC OR MIT)"
}
endef

package.json:
	@echo "$$PACKAGE_JSON" > $@

.PHONY: all $(KSRC) client css js npm package.json
