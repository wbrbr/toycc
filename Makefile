CFLAGS = -Wall -Wextra -g -fsanitize=address

all:
	gcc -c dynarray.c -o dynarray.o $(CFLAGS)
	g++ -c main.cpp -o main.o $(CFLAGS)
	g++ dynarray.o main.o -o comp $(CFLAGS)

asm:
	nasm -felf64 out.asm
	ld -o out out.o
