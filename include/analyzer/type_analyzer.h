#pragma once

// types analysis

#include <stdbool.h>

#include "domain_analyzer.h"

typedef struct {
  Type type;          // the returned type
  bool is_left_value; // true if left-value
  bool is_constant;   // true if constant
} ReturnValue;

// returns true if r->type can be converted
// to a scalar value: int, double, char or address
bool can_be_scalar(ReturnValue *return_value);

// verifies if the source type can be converted to the destination type
// if yes, returns true
bool convert_to(Type *src, Type *dst);

// sets in dst the resulted type of an arithmetic operation
// having as operands the types t1 and t2
// returns true if t1 and t2 can be operands for an arithmetic operation
// ex: double + int -> double
bool arithmetic_type_to(Type *first_type, Type *second_type, Type *dst_type);

// searches a name in a list of symbols
// if it finds it, returns the correspondent symbol, else NULL
Symbol *find_symbol_in_list(Symbol *list, const char *name);
