CC=g++
CFLAGS=-I./libs/mflibs/src -g -Wno-deprecated-declarations
LIBS=-lX11 -lXinerama -lXrender -lm -lstdc++
# SRCS=src/main.cpp src/mfwm_x11.h src/mfwm_x11.cpp src/mfwm_wm.h src/mfwm_wm.cpp config.h
SRCS=$(wildcard src/*.cpp)
SRCS+=$(wildcard src/*.h)
SRCS+=config.h

# CONTANTS
TEST_RESOLUTION=1600x900

all: build/mfwm

build/mfwm: $(SRCS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

test:
	$(CC) -I./libs/mflibs/src tests/tests.cpp -o build/tests && ./build/tests

tdd:
	ag -l --depth 1 | entr -c -s "make test"

run:
	xinit ./build/xinitrc -- /usr/bin/Xephyr :30 -wr -ac -screen $(TEST_RESOLUTION) -host-cursor -reset +xinerama

clean:
	rm build/mfwm


