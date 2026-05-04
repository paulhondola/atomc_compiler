#include <stdio.h>

#include "../../include/analyzer/symbol.h"

int type_base_size(Type *type) {
  switch (type->type_base) {
    case TYPE_BASE_INT:
      return sizeof(int);
    case TYPE_BASE_DOUBLE:
      return sizeof(double);
    case TYPE_BASE_CHAR:
      return sizeof(char);
    case TYPE_BASE_VOID:
      return 0;
    default: { // TYPE_BASE_STRUCT
      int size = 0;
      for (Symbol *symbol = type->symbol->struct_members; symbol; symbol = symbol->next) {
        size += type_size(&symbol->type);
      }
      return size;
    }
  }
}

int type_size(Type *type) {
  if (type->array_dimension < 0) {
    return type_base_size(type);
  }
  if (type->array_dimension == 0) {
    return sizeof(void *);
  }
  return type->array_dimension * type_base_size(type);
}

void show_named_type(Type *type, const char *name) {
  switch (type->type_base) {
    case TYPE_BASE_INT:
      printf("int");
      break;
    case TYPE_BASE_DOUBLE:
      printf("double");
      break;
    case TYPE_BASE_CHAR:
      printf("char");
      break;
    case TYPE_BASE_VOID:
      printf("void");
      break;
    default: // TYPE_BASE_STRUCT
      printf("struct %s", type->symbol->name);
  }
  if (name) {
    printf(" %s", name);
  }
  if (type->array_dimension == 0) {
    printf("[]");
  } else if (type->array_dimension > 0) {
    printf("[%d]", type->array_dimension);
  }
}