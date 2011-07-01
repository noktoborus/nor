# vim: ft=make ff=unix fenc=utf-8
# file: Makefile
LIBS=`pkg-config --cflags --libs egl gl`
CFLAGS=-Wall -pedantic -Wextra -std=c99 -g
BIN=./main
SRC=main.c

all: ${BIN}

run: ${BIN}
	EGL_DRIVER=egl_dri2 ${BIN}

clean:
	rm -rf ${BIN}

${BIN}: ${SRC}
	${CC} -o ${BIN} ${CFLAGS} ${SRC} ${LIBS}

