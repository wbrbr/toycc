all:
	g++ main.cpp -o comp -g -fsanitize=undefined,address
