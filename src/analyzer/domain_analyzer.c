#include "../../include/analyzer/domain_analyzer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/utils/utils.h"

void domain_analyzer_init(DomainAnalyzer *da) {
  da->symbol_table = NULL;
}

void domain_analyzer_free(DomainAnalyzer *da) {
  while (da->symbol_table) {
    drop_domain(da);
  }
}

Domain *push_domain(DomainAnalyzer *da) {
  Domain *domain   = (Domain *)safe_alloc(sizeof(Domain));
  domain->symbols  = NULL;
  domain->parent   = da->symbol_table;
  da->symbol_table = domain;
  return domain;
}

void drop_domain(DomainAnalyzer *da) {
  Domain *domain   = da->symbol_table;
  da->symbol_table = domain->parent;
  free_symbols(domain->symbols);
  free(domain);
}

Symbol *find_symbol(DomainAnalyzer *da, const char *name) {
  for (Domain *domain = da->symbol_table; domain; domain = domain->parent) {
    Symbol *symbol = find_symbol_in_domain(domain, name);
    if (symbol) {
      return symbol;
    }
  }
  return NULL;
}

Symbol *add_extern_function(DomainAnalyzer *da, const char *name,
                            void (*extern_function_pointer)(struct VirtualMachine *),
                            Type return_type) {
  Symbol *function_pointer_symbol = new_symbol(name, SYMBOL_KIND_FUNCTION);
  function_pointer_symbol->function.external_function_pointer = extern_function_pointer;
  function_pointer_symbol->type                               = return_type;
  add_symbol_to_domain(da->symbol_table, function_pointer_symbol);
  return function_pointer_symbol;
}

Symbol *add_function_parameter(Symbol *function_symbol, const char *name, Type type) {
  Symbol *parameter_symbol      = new_symbol(name, SYMBOL_KIND_PARAMETER);
  parameter_symbol->type        = type;
  parameter_symbol->param_index = symbols_len(function_symbol->function.parameters);
  add_symbol_to_list(&function_symbol->function.parameters, duplicate_symbol(parameter_symbol));
  return parameter_symbol;
}
