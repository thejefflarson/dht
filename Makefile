CC = clang
EXTRA ?= 
CFLAGS = $(shell pkg-config --cflags zlib) -fPIC $(EXTRA)
LDFLAGS = $(shell pkg-config --libs zlib) -shared
TARGET_LIB = build/libdht.so

SRCS = dht.c vendor/blake2b-ref.c #vendor/tweetnacl.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: build ${TARGET_LIB}

build:
	mkdir build

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

clean:
	rm -f ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)
	rm -r build

.PHONY: clean