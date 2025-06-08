sources=main.c
CFLAGS=$(shell pkgconf --cflags libusb-1.0) -O2 -Wall -Wextra
LDFLAGS=$(shell pkgconf --libs libusb-1.0)
CC=gcc

#CFLAGS+=-Werror -fsanitize=address

opan: main.o
	${CC} ${LDFLAGS} $< -o $@

%.o: %.c
	${CC} ${CFLAGS} $< -c -o $@
