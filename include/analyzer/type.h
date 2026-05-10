#pragma once

#include <stdint.h>

struct Symbol;
typedef struct Symbol Symbol;

typedef enum : uint8_t { // base type
  TYPE_BASE_INT,
  TYPE_BASE_DOUBLE,
  TYPE_BASE_CHAR,
  TYPE_BASE_VOID,
  TYPE_BASE_STRUCT
} TypeBase;

typedef struct {   // the type of a symbol
  TypeBase type_base;
  Symbol  *symbol; // for TB_STRUCT, the struct's symbol

  // array_dimension - the dimension for an array
  //							array_dimension < 0 - no array
  //							array_dimension == 0 - array without specified dimension: int v[]
  //							array_dimension > 0 - array with specified dimension: double v[10]
  int array_dimension;
} Type;

// returns the size of a type in bytes
int type_size(Type *type);

int type_base_size(Type *type);

// returns the natural alignment of a type in bytes
int type_alignment(Type *type);

// rounds offset up to the nearest multiple of alignment
int align_up(int offset, int alignment);

#include <stdio.h>

void show_named_type(FILE *out, Type *type, const char *name);
