#pragma once

#include "symbol.h"

typedef struct Domain {
  struct Domain *parent;  // the parent domain
  Symbol        *symbols; // the symbols from this domain (single linked list)
} Domain;

// context encapsulating all domain/scope analysis state
typedef struct DomainAnalyzer {
  Domain *symbol_table; // the top (current) domain in the scope stack
} DomainAnalyzer;

// initialises a DomainAnalyzer to an empty state
void domain_analyzer_init(DomainAnalyzer *domain_analyzer);

// frees all remaining domains and symbols
void domain_analyzer_free(DomainAnalyzer *domain_analyzer);

// adds a domain to the top of the domains's stack
Domain *push_domain(DomainAnalyzer *domain_analyzer);

// deletes the domain from the top of the domains's stack
void drop_domain(DomainAnalyzer *domain_analyzer);

// shows the content of the given domain
void show_domain(Domain *domain, const char *name);

// search a symbol with the given name in the specified domain and returns it
// if no symbol find, returns NULL
Symbol *find_symbol_in_domain(Domain *domain, const char *name);

// searches a symbol in all domains, starting with the current one
Symbol *find_symbol(DomainAnalyzer *domain_analyzer, const char *name);

// adds a symbol to the current domain
Symbol *add_symbol_to_domain(Domain *domain, Symbol *symbol);

// add in ST an extern function with the given name, address and return type
Symbol *add_extern_function(DomainAnalyzer *domain_analyzer, const char *name,
                            void (*extern_function_pointer)(struct VirtualMachine *),
                            Type return_type);

// add to fn a parameter with the given name and type
// it doesn't verify for parameter redefinition
// returns the added parameter
Symbol *add_function_parameter(Symbol *function, const char *name, Type type);
