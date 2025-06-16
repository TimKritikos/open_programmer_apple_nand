SOURCES=main.c usb_com.c term_io.c

OBJECTS=$(subst .c,.o,${SOURCES})
CFLAGS=$(shell pkgconf --cflags libusb-1.0) -O2 -Wall -Wextra
LDFLAGS=$(shell pkgconf --libs libusb-1.0)
CC=gcc

CFLAGS+=-Werror -g -fsanitize=address,undefined
LDFLAGS+=-fsanitize=address,undefined

opan: ${OBJECTS}
	${CC} ${LDFLAGS} $^ -o $@

clean:
	rm -f ${OBJECTS} opan

%.o: %.c
	${CC} ${CFLAGS} $< -c -o $@
