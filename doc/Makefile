all doc: clean
	doxygen
	@test -d html.css || git clone git://github.com/mosra/m.css html.css
	python3.6 html.css/doxygen/dox2html5.py Dopyfile
	@rm -rf xml

clean:
	@rm -vrf html4 html5

CLEAN:
	@rm -vrf html*

.PHONY: all doc clean CLEAN
