CFLAGS = -Wall -Wextra -g -fsanitize=address,undefined

all:
	gcc -c dynarray.c -o dynarray.o $(CFLAGS)
	gcc -c hashmap.c -o hashmap.o $(CFLAGS)
	gcc -c xxhash.c -o xxhash.o $(CFLAGS)
	gcc -c codegen.c -o codegen.o $(CFLAGS)
	g++ -c main.cpp -o main.o $(CFLAGS)
	g++ xxhash.o hashmap.o dynarray.o codegen.o main.o -o comp $(CFLAGS)
	gcc -c tests/hashmap_tests.c -o tests/hashmap_tests.o $(CFLAGS)
	gcc xxhash.o hashmap.o tests/hashmap_tests.o -o tests/hashmap_tests $(CFLAGS)

asm:
	nasm -felf64 out.asm
	ld -o out out.o
