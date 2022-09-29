CC=g++
CFLAGS=-I./libs/mflibs/src -I./libs/spdlog/include -g -Wno-deprecated-declarations
# CFLAGS+=-ftime-report
CFLAGS+=`pkg-config --cflags freetype2`
LIBS=-L./libs/spdlog/build/
LIBS+=-lX11 -lXinerama -lXrender -lm -lstdc++ -lXft -lspdlog
LIBS+=`pkg-config --libs freetype2`
# SRCS=src/main.cpp src/mfwm_x11.h src/mfwm_x11.cpp src/mfwm_wm.h src/mfwm_wm.cpp config.h
SRCS=$(wildcard src/*.cpp)
SRCS+=$(wildcard src/*.h)
SRCS+=config.h

# CONTANTS
TEST_RESOLUTION=800x600

all: build/mfwm

build/mfwm: $(SRCS)
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

test:
	$(CC) -I./libs/mflibs/src tests/tests.cpp -o build/tests && ./build/tests

tdd:
	ag -l --depth 1 | entr -c -s "make -s test"

run:
	xinit ./build/xinitrc -- /usr/bin/Xephyr :30 -wr -ac -screen $(TEST_RESOLUTION) -host-cursor -reset +xinerama

spdlog:
	cd libs/spdlog/ && mkdir build && cd build && cmake .. && make -j

clean:
	rm build/mfwm


