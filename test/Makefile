all test: units clean

units:
	cd .. && ./K.sh --version

clean:
ifndef TRAVIS_OS_NAME
	@cd .. && rm -f *.gcda *.gcno
endif

.PHONY: all test units clean
