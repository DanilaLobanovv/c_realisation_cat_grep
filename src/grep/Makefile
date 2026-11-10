CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror

.PHONY: all clean rebuild test
all: grep

grep: grep.c ../common/io.c ../common/io.h
	$(CC) $(CFLAGS) grep.c ../common/io.c -o $@

clean:
	rm -f grep

rebuild: clean all

test: grep
	../tests/integration.sh grep
