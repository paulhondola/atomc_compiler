#include "../../include/analyzer/symbol.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/utils/utils.h"

// free from memory a list of symbols
void free_symbols(Symbol *list) {
  for (Symbol *next; list; list = next) {
    next = list->next;
    free_symbol(list);
  }
}

Symbol *new_symbol(const char *name, SymbolKind kind) {
  Symbol *symbol = (Symbol *)safe_alloc(sizeof(Symbol));
  // sets all the fields to 0/NULL
  memset(symbol, 0, sizeof(Symbol));
  symbol->name = name;
  symbol->kind = kind;
  return symbol;
}

Symbol *duplicate_symbol(Symbol *symbol) {
  Symbol *sym = (Symbol *)safe_alloc(sizeof(Symbol));
  *sym        = *symbol;
  sym->next   = NULL;
  return sym;
}

// s->next is already NULL from new_symbol
Symbol *add_symbol_to_list(Symbol **list, Symbol *symbol) {
  Symbol *iter = *list;
  if (iter) {
    while (iter->next) {
      iter = iter->next;
    }
    iter->next = symbol;
  } else {
    *list = symbol;
  }
  return symbol;
}

// searches a name in a list of symbols
// if it finds it, returns the correspondent symbol, else NULL
Symbol *find_symbol_in_list(Symbol *list, const char *name) {
  for (Symbol *symbol = list; symbol; symbol = symbol->next) {
    if (!strcmp(symbol->name, name)) {
      return symbol;
    }
  }
  return NULL;
}

// returns the number of symbols in the list
int symbols_len(Symbol *list) {
  int length = 0;
  for (; list; list = list->next) {
    length++;
  }
  return length;
}

// prints a symbol's information to stdout
void show_symbol(Symbol *symbol) {
  switch (symbol->kind) {
    case SYMBOL_KIND_VARIABLE:
      show_named_type(&symbol->type, symbol->name);
      if (symbol->owner) {
        printf(";\t// size=%d, idx=%d\n", type_size(&symbol->type), symbol->var_index);
      } else {
        printf(";\t// size=%d, mem=%p\n", type_size(&symbol->type), symbol->var_mem);
      }
      break;
    case SYMBOL_KIND_PARAMETER: {
      show_named_type(&symbol->type, symbol->name);
      printf(" /*size=%d, idx=%d*/", type_size(&symbol->type), symbol->param_index);
    } break;
    case SYMBOL_KIND_FUNCTION: {
      show_named_type(&symbol->type, symbol->name);
      printf("(");
      bool next = false;
      for (Symbol *param = symbol->function.parameters; param; param = param->next) {
        if (next) {
          printf(", ");
        }
        show_symbol(param);
        next = true;
      }
      printf("){\n");
      for (Symbol *local = symbol->function.locals; local; local = local->next) {
        printf("\t");
        show_symbol(local);
      }
      printf("\t}\n");
    } break;
    case SYMBOL_KIND_STRUCT: {
      printf("struct %s{\n", symbol->name);
      for (Symbol *member = symbol->struct_members; member; member = member->next) {
        printf("\t");
        show_symbol(member);
      }
      printf("\t};\t// size=%d\n", type_size(&symbol->type));
    } break;
  }
}

// frees a symbol's memory
void free_symbol(Symbol *symbol) {
  switch (symbol->kind) {
    case SYMBOL_KIND_VARIABLE:
      if (!symbol->owner) {
        free(symbol->var_mem);
      }
      break;
    case SYMBOL_KIND_FUNCTION:
      free_symbols(symbol->function.parameters);
      free_symbols(symbol->function.locals);
      break;
    case SYMBOL_KIND_STRUCT:
      free_symbols(symbol->struct_members);
      break;
    case SYMBOL_KIND_PARAMETER:
      break;
  }
  free(symbol);
}
