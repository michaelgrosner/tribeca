all: clean
	doxygen
	@mv html html4
	@test -d html.css || git clone git://github.com/mosra/m.css html.css
	python3.6 html.css/doxygen/dox2html5.py Doxyfile-html5
	@mv html html5
	@rm -rf xml

clean:
	@rm -rf html4 html5

CLEAN:
	@rm -rf html*

.PHONY: all clean CLEAN
