SOURCES = $(shell ls test_*.c)
TESTS = $(basename $(SOURCES))
FUZZ = create_fuzz_cases fuzz
CASES = fuzz-cases/1.fuzz fuzz-cases/2.fuzz
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

fuzz-cases:
	mkdir -p $@

$(CASES): create_fuzz_cases
	./$<

$(FUZZ): %: %.c ../dht.c ../dht.h $(DEPS) tap.h Makefile
	$(CC) $< $(DEPS) -Wall -g -lz -o $@

fuzzer: $(FUZZ)

clean:
	rm -f $(TESTS)
	rm -f $(CASES)
	rm -f $(FUZZ)
	rm -rf coverage
	rm -f *.gcno
	rm -f *.gcda
	rm -f coverage.info
	rm -rf fuzz-casses

.PHONY: test clean fuzzer
