atoms: atoms.c atoms.h
	clang -Wall -Werror -g -fsanitize=address -o atoms atoms.c
