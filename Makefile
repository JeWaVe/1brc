build:
	gcc -O3 -march=native main.c
	chmod u+x a.out

debug: 
	gcc -g -ggdb main.c
	./a.out

run: build
	./a.out