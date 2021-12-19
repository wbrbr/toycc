all:
	g++ main.cpp -o comp -g -fsanitize=undefined,address

run:
	./comp
	nasm -felf64 out.asm
	ld -o out out.o
