#include <getopt.h>
#include <stdio.h>

#include "../common/io.h"

typedef struct {
  int number_nonblank, number, squeeze, ends, tabs, visible;
} Options;

typedef struct {
  unsigned long line;
  int beginning, previous_blank, pending_cr;
} State;

static int parse_options(int argc, char** argv, Options* options) {
  static const struct option long_options[] = {
      {"number-nonblank", no_argument, NULL, 'b'},
      {"number", no_argument, NULL, 'n'},
      {"squeeze-blank", no_argument, NULL, 's'},
      {NULL, 0, NULL, 0}};
  int option;
  while ((option = getopt_long(argc, argv, "benstETv", long_options, NULL)) !=
         -1) {
    switch (option) {
      case 'b':
        options->number_nonblank = 1;
        break;
      case 'e':
        options->ends = options->visible = 1;
        break;
      case 'n':
        options->number = 1;
        break;
      case 's':
        options->squeeze = 1;
        break;
      case 't':
        options->tabs = options->visible = 1;
        break;
      case 'E':
        options->ends = 1;
        break;
      case 'T':
        options->tabs = 1;
        break;
      case 'v':
        options->visible = 1;
        break;
      default:
        return 1;
    }
  }
  return 0;
}

static void print_character(int ch, const Options* options, State* state) {
  if (state->pending_cr) {
    if (ch == '\n')
      fputs("^M", stdout);
    else
      putchar('\r');
    state->pending_cr = 0;
  }
  if (ch == '\r' && options->ends && !options->visible) {
    state->pending_cr = 1;
    return;
  }
  if (ch == '\n') {
    if (options->ends) putchar('$');
    putchar('\n');
  } else if (ch == '\t' && options->tabs) {
    fputs("^I", stdout);
  } else if (options->visible && ch != '\t') {
    if (ch >= 128) {
      fputs("M-", stdout);
      ch -= 128;
    }
    if (ch < 32) {
      putchar('^');
      putchar(ch + 64);
    } else if (ch == 127) {
      fputs("^?", stdout);
    } else {
      putchar(ch);
    }
  } else {
    putchar(ch);
  }
}

static void copy_stream(FILE* stream, const Options* options, State* state) {
  int ch;
  while ((ch = fgetc(stream)) != EOF) {
    if (state->beginning) {
      int blank = ch == '\n';
      if (blank && options->squeeze && state->previous_blank) continue;
      state->previous_blank = blank;
      if ((options->number_nonblank && !blank) ||
          (!options->number_nonblank && options->number)) {
        printf(options->visible ? "%6lu  " : "%6lu\t", ++state->line);
      }
      state->beginning = 0;
    }
    print_character(ch, options, state);
    if (ch == '\n') state->beginning = 1;
  }
}

int main(int argc, char** argv) {
  Options options = {0};
  State state = {0, 1, 0, 0};
  int failed = parse_options(argc, argv, &options);
  if (!failed) {
    if (optind == argc) {
      copy_stream(stdin, &options, &state);
      failed |= close_input("cat", "-", stdin);
    }
    for (int i = optind; i < argc; ++i) {
      FILE* stream = open_input("cat", argv[i]);
      if (stream == NULL) {
        failed = 1;
      } else {
        copy_stream(stream, &options, &state);
        if ((options.number || options.number_nonblank) && !options.visible &&
            !state.beginning) {
          if (state.pending_cr) {
            putchar('\r');
            state.pending_cr = 0;
          }
          putchar('\n');
          state.beginning = 1;
        }
        failed |= close_input("cat", argv[i], stream);
      }
    }
    if (state.pending_cr) putchar('\r');
    if (fflush(stdout) == EOF) failed = 1;
  }
  return failed;
}