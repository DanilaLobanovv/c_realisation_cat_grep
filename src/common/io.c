#include "io.h"

#include <errno.h>
#include <string.h>

FILE* open_input(const char* program, const char* name) {
  FILE* stream = strcmp(name, "-") == 0 ? stdin : fopen(name, "r");
  if (stream == NULL)
    fprintf(stderr, "%s: %s: %s\n", program, name, strerror(errno));
  return stream;
}

int close_input(const char* program, const char* name, FILE* stream) {
  int failed = ferror(stream);
  if (stream != stdin && fclose(stream) != 0) failed = 1;
  if (failed) fprintf(stderr, "%s: %s: read error\n", program, name);
  return failed;
}
