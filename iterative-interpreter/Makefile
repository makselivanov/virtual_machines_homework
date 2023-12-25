CC=gcc
CXX=g++
CFLAGS:=-I include -O3 -m32 -g2 -fstack-protector-all

all: build/main.o build/gc_runtime.o build/byterun.o build/runtime.o build/iterative_interpreter.o
	$(CXX) $(CFLAGS) build/gc.o build/runtime.o build/byterun.o build/iterative_interpreter.o build/main.o -o build/main

build/main.o: build src/main.cpp
	$(CXX) $(CFLAGS) -c src/main.cpp -o build/main.o

build/iterative_interpreter.o: build src/iterative_interpreter.cpp
	$(CXX) $(CFLAGS) -c src/iterative_interpreter.cpp -o build/iterative_interpreter.o

build/byterun.o: build src/byterun.c
	$(CC) $(CFLAGS) -c src/byterun.c -o build/byterun.o

build/runtime.o: build src/runtime.c
	$(CC) $(CFLAGS) -c src/runtime.c -o build/runtime.o

build/gc_runtime.o: build src/gc.c
	$(CC) $(CFLAGS) -c src/gc.c -o build/gc.o

build:
	mkdir build

clean:
	$(MAKE) clean -C regression
	rm -r build

regression-all: regression regression-expressions

regression: all
	$(MAKE) clean check -j8 -C regression
	# $(MAKE) clean check -j8 -C stdlib/regression

regression-expressions: all
	$(MAKE) clean check -j8 -C regression/expressions
	$(MAKE) clean check -j8 -C regression/deep-expressions