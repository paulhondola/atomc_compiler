#pragma once

#include <stdio.h>

#include "token.h"

typedef struct TokenStream {
  struct {
    Token *head; // first element in the list
    Token *tail; // last element in the list
    Token *iterator;
    Token *consumed;
  } tokens;
  int   line;   // current line in the input file
  FILE *output; // destination for token_stream_show
} TokenStream;

// initialises a TokenStream to an empty state (output defaults to stdout)
void token_stream_init(TokenStream *token_stream);

// frees all remaining tokens
void token_stream_free(TokenStream *token_stream);

// prints the tokens from the stream to token_stream->output
void token_stream_show(const TokenStream *token_stream);

// prints an error message with the current line number
void token_stream_error(const TokenStream *token_stream, const char *format, ...);
