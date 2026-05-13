#pragma once

#include <stdio.h>

#include "symbol.h"
#include "domain.h"

// context encapsulating all domain/scope analysis state
typedef struct DomainAnalyzer {
  Domain *symbol_table; // the top (current) domain in the scope stack
  FILE   *output;       // destination for show_domain
} DomainAnalyzer;

// initialises a DomainAnalyzer to an empty state
void domain_analyzer_init(DomainAnalyzer *domain_analyzer);

// frees all remaining domains and symbols
void domain_analyzer_free(DomainAnalyzer *domain_analyzer);

// adds a domain to the top of the domains's stack
Domain *push_domain(DomainAnalyzer *domain_analyzer);

// deletes the domain from the top of the domains's stack
void drop_domain(DomainAnalyzer *domain_analyzer);

// searches a symbol in all domains, starting with the current one
Symbol *find_symbol(DomainAnalyzer *domain_analyzer, const char *name);


// add in ST an extern function with the given name, address and return type
Symbol *add_extern_function(DomainAnalyzer *domain_analyzer, const char *name,
                            void (*extern_function_pointer)(struct VirtualMachine *),
                            Type return_type);
