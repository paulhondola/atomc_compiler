#pragma once

#include "stream.h"
#include "../analyzer/domain_analyzer.h"

// parses a stream of tokens and performs domain analysis
void parse(TokenStream *token_stream, DomainAnalyzer *domain_analyzer);
