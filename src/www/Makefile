ERR        = *** Unexpected MAKELEVEL = 0.
HINT       = This Makefile can't be used directly, consider cd ../.. before try again
$(if $(subst 0,,${MAKELEVEL}),,$(warning $(ERR));$(error $(HINT)))
ERR        = *** Unexpected MAKE_VERSION < 4.
HINT       = This Makefile can't be used with your make v${MAKE_VERSION}, consider to upgrade before try again
$(if $(shell ver=${MAKE_VERSION} && test $${ver%%.*} -lt 4 && echo 1),$(warning $(ERR));$(error $(HINT)))

KASSETS    = $(KHOME)/assets
NODE_PATH  = $(KHOME)/node_modules
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
	@cd $(abspath $(NODE_PATH)/..) && $@ install
ifndef GITHUB_ACTIONS
	@rm $(abspath $(NODE_PATH)/..)/$(basename $^)*
endif

client: npm ../bin/$(KSRC)/client/main.ts
	$(info $(STEP_2))
	NODE_PATH=$(NODE_PATH) tsc --baseUrl $(NODE_PATH) --typeRoots $(NODE_PATH)/@types --alwaysStrict \
	  --experimentalDecorators -t ES2018 -m commonjs --outDir $(KASSETS)/www/js ../bin/$(KSRC)/client/main.ts

js: client $(NODE_PATH)/.bin
	$(info $(STEP_3))
	NODE_PATH=$(NODE_PATH) browserify -t [ $(NODE_PATH)/babelify --presets [ $(NODE_PATH)/@babel/preset-env ] ] \
	  $(KASSETS)/www/js/bin/$(KSRC)/client/main.js | uglifyjs | gzip > $(KASSETS)/www/js/client.min.js

css: sass/*.scss
	$(info $(STEP_4))
	$(foreach scss,$(wildcard sass/*.scss), \
	  sass $(scss) $(KASSETS)/www/css/$(basename $(notdir $(scss))).min.css ;)
	cat $(NODE_PATH)/@ag-grid-community/all-modules/dist/styles/ag-grid.css \
	  >> $(KASSETS)/www/css/bootstrap.min.css

export define PACKAGE_JSON
{
  "dependencies": {
    "@ag-grid-community/all-modules": "^25.3.0",
    "@ag-grid-community/angular":     "^25.3.0",
    "@angular/common":                   "^12.0.4",
    "@angular/compiler":                 "^12.0.4",
    "@angular/core":                     "^12.0.4",
    "@angular/forms":                    "^12.0.4",
    "@angular/platform-browser":         "^12.0.4",
    "@angular/platform-browser-dynamic": "^12.0.4",
    "@angular/router":                   "^12.0.4",
    "@babel/core":       "^7.14.5",
    "@babel/preset-env": "^7.14.5",
    "@types/node": "^15.12.2",
    "babelify": "^10.0.0",
    "browserify": "^17.0.0",
    "highcharts": "^9.1.1",
    "highcharts-angular": "^2.10.0",
    "rxjs": "^6.6.6",
    "sass": "^1.34.1",
    "typescript": "^4.3.2",
    "uglify-js": "^3.13.9",
    "zone.js": "^0.11.4"
  },
  "description": "K",
  "homepage": "https://github.com/ctubio/Krypto-trading-bot",
  "bugs": "https://github.com/ctubio/Krypto-trading-bot/issues",
  "repository": {
    "type": "git",
    "url": "git@github.com:ctubio/Krypto-trading-bot.git"
  },
  "author": "Carles Tubio",
  "os": [
    "darwin",
    "linux",
    "win32"
  ],
  "license": "(ISC OR MIT)"
}
endef

package.json:
	@echo "$$PACKAGE_JSON" > $(abspath $(NODE_PATH)/../$@)

.PHONY: all $(KSRC) client css js npm package.json
