#pragma once

#include "token.h"

typedef struct TokenStream {
  struct {
    Token *head; // first element in the list
    Token *tail; // last element in the list
    Token *iterator;
    Token *consumed;
  } tokens;
  int line; // current line in the input file
} TokenStream;

// initialises a TokenStream to an empty state
void token_stream_init(TokenStream *token_stream);

// frees all remaining tokens
void token_stream_free(TokenStream *token_stream);

// prints the tokens from the stream to a given output file
void token_stream_show(FILE *output_file, const TokenStream *token_stream);

// prints an error message with the current line number
void token_stream_error(const TokenStream *token_stream, const char *format, ...);
