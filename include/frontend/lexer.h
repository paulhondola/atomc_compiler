#pragma once

#include <stdio.h>

#include "stream.h"
#include "token.h"

// tokenizes a source code string
void tokenize(TokenStream *token_stream, const char *source_code);
