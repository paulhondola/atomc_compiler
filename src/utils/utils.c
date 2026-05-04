#include "../../include/utils/utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void err(const char *fmt, ...) {
  fprintf(stderr, RED "error: ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n" RESET);
  exit(EXIT_FAILURE);
}

// duplicates the source characters in [begin, end) into a new heap string
char *extract(const char *begin, const char *end) {
  size_t len = (size_t)(end - begin);
  char  *buf = safe_alloc(len + 1);
  memcpy(buf, begin, len);
  buf[len] = '\0';
  return buf;
}

void *safe_alloc(const size_t n_bytes) {
  void *ptr = malloc(n_bytes);
  if (!ptr) {
    err("not enough memory");
  }
  return ptr;
}

char *load_file(const char *file_name) {
  FILE *fis = fopen(file_name, "rb");
  if (!fis) {
    err("unable to open %s", file_name);
  }

  fseek(fis, 0, SEEK_END);
  const size_t file_bytes = (size_t)ftell(fis);
  fseek(fis, 0, SEEK_SET);

  char        *buf    = safe_alloc(file_bytes + 1);
  const size_t n_read = fread(buf, sizeof(char), file_bytes, fis);

  fclose(fis);

  if (file_bytes != n_read) {
    free(buf);
    err("cannot read all the content of %s", file_name);
    exit(EXIT_FAILURE);
  }

  buf[file_bytes] = '\0';
  return buf;
}
