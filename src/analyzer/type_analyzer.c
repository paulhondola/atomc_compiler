#include "../../include/analyzer/type_analyzer.h"

#include <string.h>

#include "../../include/analyzer/type.h"

bool can_be_scalar(ReturnValue *return_value) {
  Type *t = &return_value->type;
  if (t->array_dimension >= 0) {
    return false;
  }
  switch (t->type_base) {
    case TYPE_BASE_INT:
    case TYPE_BASE_DOUBLE:
    case TYPE_BASE_CHAR:
      return true;
    default:
      return false;
  }
}

bool convert_to(Type *src_type, Type *dst_type) {
  // the pointers (arrays) can be converted one to another, but in nothing else
  if (src_type->array_dimension >= 0) {
    if (dst_type->array_dimension >= 0) {
      return true;
    }
    return false;
  }
  if (dst_type->array_dimension >= 0) {
    return false;
  }
  switch (src_type->type_base) {
    case TYPE_BASE_INT:
    case TYPE_BASE_DOUBLE:
    case TYPE_BASE_CHAR:
      switch (dst_type->type_base) {
        case TYPE_BASE_INT:
        case TYPE_BASE_CHAR:
        case TYPE_BASE_DOUBLE:
          return true;
        default:
          return false;
      }
    // a struct can be converted only to itself
    case TYPE_BASE_STRUCT:
      if (dst_type->type_base == TYPE_BASE_STRUCT && src_type->symbol == dst_type->symbol) {
        return true;
      }
      return false;
    default:
      return false;
  }
}

bool arithmetic_type_to(Type *first_type, Type *second_type, Type *dst_type) {
  // there are no arithmetic operations with pointers
  if (first_type->array_dimension >= 0 || second_type->array_dimension >= 0) {
    return false;
  }
  // the result of an arithmetic operation cannot be pointer or struct
  dst_type->symbol          = NULL;
  dst_type->array_dimension = -1;
  switch (first_type->type_base) {
    case TYPE_BASE_INT:
      switch (second_type->type_base) {
        case TYPE_BASE_INT:
        case TYPE_BASE_CHAR:
          dst_type->type_base = TYPE_BASE_INT;
          return true;
        case TYPE_BASE_DOUBLE:
          dst_type->type_base = TYPE_BASE_DOUBLE;
          return true;
        default:
          return false;
      }
    case TYPE_BASE_DOUBLE:
      switch (second_type->type_base) {
        case TYPE_BASE_INT:
        case TYPE_BASE_DOUBLE:
        case TYPE_BASE_CHAR:
          dst_type->type_base = TYPE_BASE_DOUBLE;
          return true;
        default:
          return false;
      }
    case TYPE_BASE_CHAR:
      switch (second_type->type_base) {
        case TYPE_BASE_INT:
        case TYPE_BASE_DOUBLE:
        case TYPE_BASE_CHAR:
          dst_type->type_base = second_type->type_base;
          return true;
        default:
          return false;
      }
    default:
      return false;
  }
}
