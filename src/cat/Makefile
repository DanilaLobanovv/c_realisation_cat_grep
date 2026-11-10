CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror

.PHONY: all clean rebuild test
all: cat

cat: cat.c ../common/io.c ../common/io.h
	$(CC) $(CFLAGS) cat.c ../common/io.c -o $@

clean:
	rm -f cat

rebuild: clean all

test: cat
	../tests/integration.sh cat
