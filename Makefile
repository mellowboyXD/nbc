CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c23 -ggdb

build: src/nbc.c
	$(CC) $(CFLAGS) src/nbc.c -o nbc
