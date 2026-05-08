#include "../../include/analyzer/domain.h"
#include <string.h>
#include <stdio.h>

void show_domain(FILE *out, Domain *domain, const char *name) {
  fprintf(out, "// domain: %s\n", name);
  for (Symbol *symbol = domain->symbols; symbol; symbol = symbol->next) {
    show_symbol(out, symbol);
  }
  fprintf(out, "\n\n");
}

Symbol *add_symbol_to_domain(Domain *domain, Symbol *symbol) {
  return add_symbol_to_list(&domain->symbols, symbol);
}

Symbol *find_symbol_in_domain(Domain *domain, const char *name) {
  for (Symbol *symbol = domain->symbols; symbol; symbol = symbol->next) {
    if (!strcmp(symbol->name, name)) {
      return symbol;
    }
  }
  return NULL;
}
