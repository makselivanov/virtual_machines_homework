CC=gcc
CXX=g++
CFLAGS:=-I include -O3 -m32 -g2 -fstack-protector-all

all: build/main.o build/byterun.o
	$(CXX) -std=c++17 $(CFLAGS) build/byterun.o build/main.o -o build/main

build/main.o: build src/main.cpp
	$(CXX) -std=c++17 $(CFLAGS) -c src/main.cpp -o build/main.o

build/byterun.o: build src/byterun.c
	$(CC) $(CFLAGS) -c src/byterun.c -o build/byterun.o
build:
	mkdir build

clean:
	$(MAKE) clean -C regression
	rm -r build
