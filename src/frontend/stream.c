#include "../../include/frontend/stream.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/frontend/token.h"
#include "../../include/utils/utils.h"

void token_stream_init(TokenStream *stream) {
  memset(stream, 0, sizeof(TokenStream));
  stream->line   = 1;
  stream->output = stdout;
}

void token_stream_free(TokenStream *stream) {
  Token *token = stream->tokens.head;
  while (token) {
    Token *next = token->next;
    if (token->type == ID || token->type == STRING) {
      free(token->text);
    }
    free(token);
    token = next;
  }
  memset(stream, 0, sizeof(TokenStream));
}

void token_stream_show(const TokenStream *stream) {
  for (const Token *tk = stream->tokens.head; tk; tk = tk->next) {
    fprintf(stream->output, "%d\t", tk->line);
    token_print_name(stream->output, tk);
    fprintf(stream->output, "\n");
  }
}

void token_stream_error(const TokenStream *stream, const char *fmt, ...) {
  fprintf(stderr, RED "error in line %d: ", stream->tokens.iterator->line);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n" RESET);
  err("token error");
}
