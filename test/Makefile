all: test

test: units clean

units:
	cd .. && ./K.sh --version

clean:
ifndef TRAVIS_DEPLOY
	@cd .. && rm -f *.gcda *.gcno
endif

.PHONY: all test units clean
