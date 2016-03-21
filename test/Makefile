SOURCES = $(shell ls test_*.c)
TESTS = $(basename $(SOURCES))
DEPS = ../vendor/blake2b-ref.o ../vendor/tweetnacl.o
CFLAGS = -Wall -g -fprofile-arcs -ftest-coverage -fsanitize=address -fno-omit-frame-pointer
CC ?= clang

test: coverage/index.html

$(DEPS):
	cd .. && make && cd test

$(TESTS): %: %.c ../dht.c ../dht.h $(DEPS) tap.h Makefile
	$(CC) $< $(DEPS) $(CFLAGS) -lz -o $@

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