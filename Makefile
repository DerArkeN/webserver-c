all: build run

build:
	gcc -lpthread -o server app/*.c

run:
	./server
