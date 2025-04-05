main: src/main.c
	gcc -ggdb -Wall -o rpn src/main.c

run: main
	./rpn "2 1 2 * 3 4 * - -"

debug: main
	gf2 --args ./rpn "2 1 2 * 3 4 * - -"

