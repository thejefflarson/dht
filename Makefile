CC = clang
EXTRA ?= -O3
CFLAGS = $(shell pkg-config --cflags zlib) -fPIC $(EXTRA)
LDFLAGS = $(shell pkg-config --libs zlib) -shared
DYNAMIC = build/libdht.so
STATIC = build/libdht.a
SRCS = dht.c vendor/blake2b-ref.c vendor/tweetnacl.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: build ${DYNAMIC} ${STATIC}

build:
	mkdir build

$(DYNAMIC): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

$(STATIC): $(OBJS)
	ar rcs $@ $^

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

clean:
	rm -f ${DYNAMIC} ${OBJS} $(SRCS:.c=.d)
	rm -r build

.PHONY: clean