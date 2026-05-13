#include "../../include/analyzer/domain.h"
#include "../../include/analyzer/domain_analyzer.h"
#include <string.h>
#include <stdio.h>

void show_domain(DomainAnalyzer *da, const char *name) {
  fprintf(da->output, "// domain: %s\n", name);
  for (Symbol *symbol = da->symbol_table->symbols; symbol; symbol = symbol->next) {
    show_symbol(da->output, symbol);
  }
  fprintf(da->output, "\n\n");
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
