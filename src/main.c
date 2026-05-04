#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/analyzer/domain_analyzer.h"
#include "../include/frontend/lexer.h"
#include "../include/frontend/parser.h"
#include "../include/utils/utils.h"

int main(const int argc, const char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input_file> [output_file]\n", argv[0]);
    return 1;
  }

  const char *src_string = load_file(argv[1]);

  TokenStream    token_stream;
  DomainAnalyzer domain_analyzer;
  token_stream_init(&token_stream);
  domain_analyzer_init(&domain_analyzer);

  tokenize(&token_stream, src_string);

  FILE *out = stdout;
  if (argc >= 3) {
    out = fopen(argv[2], "w");
    if (out == NULL) {
      err("cannot open output file %s", argv[2]);
    }
  }

  token_stream_show(out, &token_stream);

  push_domain(&domain_analyzer);
  parse(&token_stream, &domain_analyzer);
  show_domain(domain_analyzer.symbol_table, "global");
  drop_domain(&domain_analyzer);

  if (out != stdout) {
    fclose(out);
  }

  free((void *)src_string);
  token_stream_free(&token_stream);
  domain_analyzer_free(&domain_analyzer);

  printf(GREEN "Parsing finished successfully!" RESET "\n");

  return 0;
}
