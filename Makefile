CFLAGS=$(shell pkgconf --cflags libusb-1.0) -O2 -Wall -Wextra
LDFLAGS=$(shell pkgconf --libs libusb-1.0)
CC=gcc

CFLAGS+=-Werror -g -fsanitize=address,undefined
LDFLAGS+=-fsanitize=address,undefined

opan: main.o usb_com.o term_io.o
	${CC} ${LDFLAGS} $^ -o $@

%.o: %.c
	${CC} ${CFLAGS} $< -c -o $@
