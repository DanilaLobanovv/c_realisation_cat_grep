#ifndef COMMON_IO_H_
#define COMMON_IO_H_

#include <stdio.h>

FILE* open_input(const char* program, const char* name);
int close_input(const char* program, const char* name, FILE* stream);

#endif
