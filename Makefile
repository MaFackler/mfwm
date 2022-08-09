CFLAGS=-I./libs/mflibs/src -g -Wno-deprecated-declarations
LIBS=-lX11

SRCS=src/main.cpp src/mfwm_x11.h src/mfwm_x11.cpp config.h

all: build/mfwm

build/mfwm: $(SRCS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

run:
	xinit ./build/xinitrc -- /usr/bin/Xephyr :30 -wr -ac -screen 800x600 -host-cursor -reset +xinerama

clean:
	rm build/mfwm


