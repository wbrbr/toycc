CFLAGS = -std=c99 -pedantic -Wall -Wextra -g -fsanitize=undefined

all: codegen.o dynarray.o hashmap.o lexer.o main.o parser.o type.o util.o xxhash.o
	c++ $^ -o toycc $(CFLAGS)
	cc -c tests/hashmap_tests.c -o tests/hashmap_tests.o $(CFLAGS)
	cc xxhash.o hashmap.o tests/hashmap_tests.o -o tests/hashmap_tests $(CFLAGS)

%.o: %.c
	cc -c $< -o $@ $(CFLAGS)

asm:
	nasm -felf64 out.s
	ld -o out out.o

dot:
	dot -Tsvg -o ast.svg ast.dot

clean:
	rm -f *.o
	rm -f out.s
	rm -f out
	rm -f ast.dot
	rm -f toycc
