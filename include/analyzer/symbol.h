#pragma once

#include <stdint.h>

#include "../vm/instruction.h"
#include "type.h"

struct VirtualMachine;
struct Symbol;
typedef struct Symbol Symbol;

typedef enum : uint8_t { // symbol's kind
  SYMBOL_KIND_VARIABLE,
  SYMBOL_KIND_PARAMETER,
  SYMBOL_KIND_FUNCTION,
  SYMBOL_KIND_STRUCT
} SymbolKind;

struct Symbol {
  const char *name; // symbol's name. The symbol doesn't own this pointer, but it is allocated //
                    // somewhere else (ex: in Token)
  SymbolKind kind;
  Type       type;

  // owner:
  //		- NULL for global symbols
  //		- a struct for variables defined in that struct
  //		- a function for parameters/variables local to that function
  Symbol *owner;
  Symbol *next; // the link to the next symbol in list
  union {       // specific data fo each kind of symbol
    // the index in fn.locals for local vars
    // the index in struct for struct members
    int var_index;
    // the variable memory for global vars (dynamically allocated)
    void *var_mem;
    // the index in fn.params for parameters
    int param_index;
    // the members of a struct
    Symbol *struct_members;
    struct {
      Symbol *parameters; // the parameters of a function
      Symbol *locals;     // all local vars of a function, including the ones from its inner domains
      void (*external_function_pointer)(struct VirtualMachine *); // !=NULL for extern functions
      Instruction *instruction; // used if external_function_pointer==NULL
    } function;
  };
};

// dynamically allocation of a new symbol
Symbol *new_symbol(const char *name, SymbolKind kind);

// duplicates the given symbol
Symbol *duplicate_symbol(Symbol *symbol);

// adds the symbol the the end of the list
// list - the address of the list where to add the symbol
Symbol *add_symbol_to_list(Symbol **list, Symbol *symbol);

// searches a name in a list of symbols
// if it finds it, returns the correspondent symbol, else NULL
Symbol *find_symbol_in_list(Symbol *list, const char *name);

// add to fn a parameter with the given name and type
// it doesn't verify for parameter redefinition
// returns the added parameter
Symbol *add_function_parameter(Symbol *function, const char *name, Type type);

// the number of the symbols in list
int symbols_len(Symbol *list);

void show_symbol(Symbol *symbol);

// frees the memory of a list of symbols
void free_symbols(Symbol *list);

// frees the memory of a symbol
void free_symbol(Symbol *symbol);
