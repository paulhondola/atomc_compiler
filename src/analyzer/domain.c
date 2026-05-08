#include "../../include/analyzer/domain.h"
#include <string.h>
#include <stdio.h>

void show_domain(Domain *domain, const char *name) {
  printf("// domain: %s\n", name);
  for (Symbol *symbol = domain->symbols; symbol; symbol = symbol->next) {
    show_symbol(symbol);
  }
  puts("\n");
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
