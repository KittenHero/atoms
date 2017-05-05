atoms: atoms.c atoms.h
	clang -Wall -Werror -pedantic-errors -std=c11 -g -fsanitize=address -o atoms atoms.c
