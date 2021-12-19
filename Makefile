all:
	g++ -Wall main.cpp -o comp -g -fsanitize=undefined

run:
	./comp
	nasm -felf64 out.asm
	ld -o out out.o
