CFLAGS=-I./libs/mflibs/src -g
LIBS=-lX11

all: build/mfwm

build/mfwm: src/main.cpp src/mfwm_x11.h src/mfwm_x11.cpp
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

run:
	xinit ./build/xinitrc -- /usr/bin/Xephyr :30 -wr -ac -screen 800x600 -host-cursor -reset +xinerama

clean:
	rm build/mfwm


