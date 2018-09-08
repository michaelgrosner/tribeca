ERR     := *** Unexpected MAKELEVEL = 0.
HINT    := This Makefile can't be used directly, consider cd ../.. before try again
$(if $(shell test ${MAKELEVEL} -eq 0 || echo 1),,$(warning $(ERR));$(error $(HINT)))
ERR     := *** Unexpected MAKE_VERSION < 4.
HINT    := This Makefile can't be used with your make version, consider to upgrade before try again
$(if $(shell test "`echo ${MAKE_VERSION} | cut -d. -f1`" -lt 4 || echo 1),,$(warning $(ERR));$(error $(HINT)))

KLIB      := $(realpath ../../build-$(CHOST)/local/lib)
KTMP      := $(abspath $(KLIB)/../tmp)
NODE_PATH := $(abspath ./node_modules)
PATH      := $(NODE_PATH)/.bin:$(PATH)
,         := ,
STEPS     := $(shell grep "call\ STEP" Makefile | wc -l)
STEP       = $(shell tput setaf 2;tput setab 0)[$(1)/$(STEPS)]$(shell tput setaf 3) Building client $(2)..$(shell tput sgr0)
STEP_1    := $(call STEP,1,npm packages)
STEP_2    := $(call STEP,2,ts files)
STEP_3    := $(call STEP,3,js zip file)
STEP_4    := $(call STEP,4,css files)
STEP_5    := $(call STEP,5,document root assets lib)

all trading-bot: js css
	$(info $(STEP_5))
	$(foreach chost,$(subst $(CHOST),,$(CARCH)),          \
	  tmp=$(subst build-$(CHOST),build-$(chost),$(KTMP)); \
	  $(if $(shell test -d $$tmp/.. && echo 1),           \
	    rm -rf $$tmp;                                     \
	    cp -R $(KTMP) $$tmp;                              \
	  )                                                   \
	)
	$(foreach chost,$(CARCH),! test -d ../../build-$(chost) \
	  || CHOST=$(chost) $(MAKE) assets clean ;)

assets: trading-bot.S
	$(CHOST)-g++ $(addprefix -Wa$(,)-I,$(realpath ./ $(KTMP))) \
	  -c $^ -o $(KLIB)/$(patsubst %.S,K-%-$(CHOST)-$@.o,$^)

clean:
	@rm -rf $(KTMP)

npm:
	$(info $(STEP_1))
	@$(MAKE) package.json
	@$@ install
ifndef TRAVIS_DEPLOY
	@rm package*.json
endif

client: npm clean client/main.ts
	$(info $(STEP_2))
	tsc --alwaysStrict --experimentalDecorators -t ES2017 -m commonjs --outDir $(KTMP)/www/js client/*.ts

js: client $(NODE_PATH)/.bin
	$(info $(STEP_3))
	NODE_PATH=$(NODE_PATH) browserify -t [ babelify --presets [ babili env ] ] \
	  $(KTMP)/www/js/main.js | uglifyjs | gzip > $(KTMP)/www/js/client.min.js

css: www/sass/*.scss
	$(info $(STEP_4))
	$(foreach scss,$(wildcard www/sass/*.scss), \
	  sass $(scss) $(KTMP)/www/css/$(basename $(notdir $(scss))).min.css ;)
	cat $(NODE_PATH)/ag-grid/dist/styles/ag-grid.css >> $(KTMP)/www/css/bootstrap.min.css

export define PACKAGE_JSON
{
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

.PHONY: all trading-bot assets clean client css js npm package.json
