all:
	g++ -Wall -Wextra main.cpp -o comp -g -fsanitize=undefined

asm:
	nasm -felf64 out.asm
	ld -o out out.o
