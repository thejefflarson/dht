SOURCES = $(shell ls test_*.c)
TESTS = $(basename $(SOURCES))

all: $(TESTS)
coverage: coverage/index.html

$(TESTS): %: %.c ../dht.c ../dht.h tap.h util.h Makefile
	cc $< -Wall -g --coverage -fprofile-arcs -ftest-coverage -fsanitize=address -fno-omit-frame-pointer -o $@
	./$@

coverage/index.html: $(TESTS)
	lcov --capture --directory . --output-file coverage.info
	rm *.gcno
	rm *.gcda
	rm coverage.info

clean:
	rm $(TESTS)
	rm -rf coverage

.PHONY: all clean coverage