all test: units clean

units:
	cd .. && ./K.sh --version

clean:
ifndef GITHUB_ACTIONS
	@cd .. && rm -f *.gcda *.gcno
endif

.PHONY: all test units clean
