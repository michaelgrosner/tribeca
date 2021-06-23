ERR        = *** Unexpected MAKELEVEL = 0.
HINT       = This Makefile can't be used directly, consider cd ../../.. before try again
$(if $(subst 0,,${MAKELEVEL}),,$(warning $(ERR));$(error $(HINT)))
ERR        = *** Unexpected MAKE_VERSION < 4.
HINT       = This Makefile can't be used with your make v${MAKE_VERSION}, consider to upgrade before try again
$(if $(shell ver=${MAKE_VERSION} && test $${ver%%.*} -lt 4 && echo 1),$(warning $(ERR));$(error $(HINT)))

KASSETS    = $(KHOME)/assets/www
NODE_PATH  = $(KHOME)/node_modules
PATH      := $(NODE_PATH)/.bin:$(PATH)

TSC       := tsc --baseUrl $(NODE_PATH) --typeRoots $(NODE_PATH)/@types \
               --alwaysStrict --experimentalDecorators --target ES2018 --module commonjs
BROWSERIFY:= browserify --transform [ $(NODE_PATH)/babelify --presets [ $(NODE_PATH)/@babel/preset-env ] ]

STEPS     := $(shell grep "call\ STEP" Makefile | wc -l)
STEP       = $(shell tput setaf 2;tput setab 0)[$(1)/$(STEPS)]$(shell tput setaf 3;tput setab 0) Building client $(2)..$(shell tput sgr0)
STEP_1    := $(call STEP,1,npm packages)
STEP_2    := $(call STEP,2,ts files)
STEP_3    := $(call STEP,3,js zip file)
STEP_4    := $(call STEP,4,css files)

all $(KSRC): js css

npm: package.json
	@echo $(STEP_1)
	@cd $(KHOME) && $@ install
ifndef GITHUB_ACTIONS
	@rm $(KHOME)/$(basename $<)*
endif

lib: www/js npm
	@echo $(STEP_2)
	cp -R $< $(NODE_PATH)/$@
	$(TSC) $(NODE_PATH)/$@/*.ts

js: ../../bin/$(KSRC)/$(KSRC).client/bootstrap.ts lib
	$(TSC) --outDir $(KASSETS)/$@ $<
	@echo $(STEP_3)
	$(BROWSERIFY) $(KASSETS)/$@/bootstrap.$@ | uglifyjs | gzip > $(KASSETS)/$@/client.min.$@

css: www/css/*.scss
	@echo $(STEP_4)
	$(foreach scss,$^,sass $(scss) $(KASSETS)/$@/$(basename $(notdir $(scss))).min.$@ ;)
	cat $(NODE_PATH)/@ag-grid-community/all-modules/dist/styles/ag-grid.$@ >> $(KASSETS)/$@/bootstrap.min.$@

export define PACKAGE_JSON
{
  "dependencies": {
    "@ag-grid-community/all-modules": "^25.3.0",
    "@ag-grid-community/angular":     "^25.3.0",
    "@angular/common":                   "^12.0.5",
    "@angular/compiler":                 "^12.0.5",
    "@angular/core":                     "^12.0.5",
    "@angular/forms":                    "^12.0.5",
    "@angular/platform-browser":         "^12.0.5",
    "@angular/platform-browser-dynamic": "^12.0.5",
    "@angular/router":                   "^12.0.5",
    "@babel/core":       "^7.14.6",
    "@babel/preset-env": "^7.14.5",
    "@types/node": "^15.12.2",
    "babelify": "^10.0.0",
    "browserify": "^17.0.0",
    "highcharts": "^9.1.2",
    "highcharts-angular": "^2.10.0",
    "rxjs": "^6.6.6",
    "sass": "^1.35.1",
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
  "license": "MIT"
}
endef

package.json:
	@echo "$$PACKAGE_JSON" > $(KHOME)/$@

.PHONY: all $(KSRC) lib css js npm package.json
