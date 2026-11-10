#define _POSIX_C_SOURCE 200809L
#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/io.h"

typedef struct {
  int ignore_case, invert, count, list, number;
  char** patterns;
  size_t pattern_count;
} Options;

typedef struct {
  regex_t* items;
  size_t count;
} Patterns;

static int add_pattern(Options* options, char* pattern) {
  char** items =
      realloc(options->patterns, (options->pattern_count + 1) * sizeof(*items));
  if (items == NULL) return 1;
  options->patterns = items;
  options->patterns[options->pattern_count++] = pattern;
  return 0;
}

static int parse_options(int argc, char** argv, Options* options) {
  int option, failed = 0;
  while (!failed && (option = getopt(argc, argv, "e:ivcln")) != -1) {
    switch (option) {
      case 'e':
        failed = add_pattern(options, optarg);
        break;
      case 'i':
        options->ignore_case = 1;
        break;
      case 'v':
        options->invert = 1;
        break;
      case 'c':
        options->count = 1;
        break;
      case 'l':
        options->list = 1;
        break;
      case 'n':
        options->number = 1;
        break;
      default:
        failed = 1;
    }
  }
  if (!failed && options->pattern_count == 0) {
    if (optind == argc) {
      fprintf(stderr, "grep: missing pattern\n");
      failed = 1;
    } else {
      failed = add_pattern(options, argv[optind++]);
    }
  }
  return failed;
}

static void free_patterns(Patterns* patterns) {
  for (size_t i = 0; i < patterns->count; ++i) regfree(&patterns->items[i]);
  free(patterns->items);
}

static int compile_patterns(const Options* options, Patterns* patterns) {
  int failed = 0;
  for (size_t i = 0; i < options->pattern_count && !failed; ++i) {
    const char* start = options->patterns[i];
    do {
      const char* end = strchr(start, '\n');
      size_t length = end ? (size_t)(end - start) : strlen(start);
      char* part = malloc(length + 1);
      regex_t* items =
          realloc(patterns->items, (patterns->count + 1) * sizeof(*items));
      if (items != NULL) patterns->items = items;
      if (part == NULL || items == NULL) {
        free(part);
        failed = 1;
      } else {
        memcpy(part, start, length);
        part[length] = '\0';
        int status =
            regcomp(&patterns->items[patterns->count], length ? part : ".*",
                    REG_NOSUB | (options->ignore_case ? REG_ICASE : 0));
        free(part);
        if (status != 0) {
          char message[256];
          regerror(status, &patterns->items[patterns->count], message,
                   sizeof(message));
          fprintf(stderr, "grep: %s\n", message);
          failed = 1;
        } else {
          ++patterns->count;
        }
      }
      start = end ? end + 1 : NULL;
    } while (start != NULL && !failed);
  }
  return failed;
}

static int matches(const char* line, const Patterns* patterns) {
  int found = 0;
  for (size_t i = 0; i < patterns->count && !found; ++i)
    found = regexec(&patterns->items[i], line, 0, NULL, 0) == 0;
  return found;
}

static int search_stream(FILE* stream, const char* name, int show_name,
                         const Options* options, const Patterns* patterns) {
  char* line = NULL;
  size_t capacity = 0;
  ssize_t length;
  unsigned long line_number = 0, selected = 0;
  while ((length = getline(&line, &capacity, stream)) != -1) {
    ++line_number;
    int had_newline = length > 0 && line[length - 1] == '\n';
    if (had_newline) line[length - 1] = '\0';
    int selected_line = matches(line, patterns) != options->invert;
    if (had_newline) line[length - 1] = '\n';
    if (selected_line) {
      ++selected;
      if (options->list) break;
      if (!options->count) {
        if (show_name) printf("%s:", name);
        if (options->number) printf("%lu:", line_number);
        fwrite(line, 1, (size_t)length, stdout);
        if (length > 0 && line[length - 1] != '\n') putchar('\n');
      }
    }
  }
  free(line);
  if (options->list && selected)
    printf("%s\n", name);
  else if (options->count && !options->list) {
    if (show_name) printf("%s:", name);
    printf("%lu\n", selected);
  }
  return selected != 0;
}

int main(int argc, char** argv) {
  Options options = {0};
  Patterns patterns = {0};
  int error = parse_options(argc, argv, &options);
  if (!error) error = compile_patterns(&options, &patterns);
  int found = 0;
  if (!error) {
    int file_count = argc - optind;
    if (!file_count) {
      found |= search_stream(stdin, "(standard input)", 0, &options, &patterns);
      error |= close_input("grep", "-", stdin);
    }
    for (int i = optind; i < argc; ++i) {
      FILE* stream = open_input("grep", argv[i]);
      if (stream == NULL) {
        error = 1;
      } else {
        const char* name =
            strcmp(argv[i], "-") == 0 ? "(standard input)" : argv[i];
        found |=
            search_stream(stream, name, file_count > 1, &options, &patterns);
        error |= close_input("grep", argv[i], stream);
      }
    }
    if (fflush(stdout) == EOF) error = 1;
  }
  free_patterns(&patterns);
  free(options.patterns);
  return error ? 2 : !found;
}
