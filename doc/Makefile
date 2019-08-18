KDOC = /var/lib/K/doc

all doc: clean
	@mkdir -p $(KDOC)
	doxygen
	@test -d $(KDOC)/html.css || git clone git://github.com/mosra/m.css $(KDOC)/html.css
	python3.6 $(KDOC)/html.css/documentation/doxygen.py Dopyfile
	@rm -rf $(KDOC)/xml

clean:
	@rm -vrf $(KDOC)/html{4,5}

CLEAN:
	@rm -vrf $(KDOC)/html*

.PHONY: all doc clean CLEAN
