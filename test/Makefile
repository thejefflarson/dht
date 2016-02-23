SOURCES = $(shell ls test_*.c)
TESTS = $(basename $(SOURCES))

all: $(TESTS)

$(TESTS): %: %.c ../dht.c ../dht.h tap.h util.h
	cc $< -Wall -Wpedantic -Werror -g -fsanitize=address -fno-omit-frame-pointer -o $@
	./$@

.PHONY: all