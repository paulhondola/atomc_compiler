#pragma once

#include "symbol.h"

typedef struct Domain {
  struct Domain *parent;  // the parent domain
  Symbol        *symbols; // the symbols from this domain (single linked list)
} Domain;

// adds a symbol to the current domain
Symbol *add_symbol_to_domain(Domain *domain, Symbol *symbol);

// search a symbol with the given name in the specified domain and returns it
// if no symbol find, returns NULL
Symbol *find_symbol_in_domain(Domain *domain, const char *name);

struct DomainAnalyzer;

// shows the content of the current domain using da->output
void show_domain(struct DomainAnalyzer *da, const char *name);
