all: build run

build:
	gcc -lpthread -o server src/*.c

run:
	./server
