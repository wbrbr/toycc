CFLAGS = -Wall -Wextra -g -fsanitize=address,undefined

all: codegen.o dynarray.o hashmap.o main.o parser.o xxhash.o
	c++ $^ -o toycc $(CFLAGS)
	cc -c tests/hashmap_tests.c -o tests/hashmap_tests.o $(CFLAGS)
	cc xxhash.o hashmap.o tests/hashmap_tests.o -o tests/hashmap_tests $(CFLAGS)

%.o: %.c
	cc -c $< -o $@ $(CFLAGS)

%.o: %.cpp
	c++ -c $< -o $@ $(CFLAGS)

asm:
	nasm -felf64 out.asm
	ld -o out out.o
