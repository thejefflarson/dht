SOURCES = $(shell ls test_*.c)
TESTS = $(basename $(SOURCES))
CC ?= clang

test: coverage/index.html

$(TESTS): %: %.c ../dht.c ../dht.h tap.h util.h Makefile
	$(CC) $< -Wall -g -fprofile-arcs -ftest-coverage -fsanitize=address -fno-omit-frame-pointer -o $@

%.gcno: %
	./$<

coverage.info: $(addsuffix .gcno, $(TESTS)) $(TESTS)
	lcov --capture --directory . --output-file $@

coverage/index.html: coverage.info
	genhtml -o coverage $<

clean:
	rm $(TESTS)
	rm -rf coverage
	rm -f *.gcno
	rm -f *.gcda
	rm -f coverage.info

.PHONY: test clean