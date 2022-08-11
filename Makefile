CFLAGS=-I./libs/mflibs/src -g -Wno-deprecated-declarations
LIBS=-lX11 -lXinerama
SRCS=src/main.cpp src/mfwm_x11.h src/mfwm_x11.cpp config.h

# CONTANTS
TEST_RESOLUTION=1600x900

all: build/mfwm

build/mfwm: $(SRCS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

run:
	xinit ./build/xinitrc -- /usr/bin/Xephyr :30 -wr -ac -screen $(TEST_RESOLUTION) -host-cursor -reset +xinerama

clean:
	rm build/mfwm


