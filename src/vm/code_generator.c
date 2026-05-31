#include "../../include/vm/code_generator.h"

#include "../../include/vm/instruction.h"

void insert_conversion_if_needed(Instruction *before, Type *src_type, Type *dst_type) {
  if (src_type->type_base == dst_type->type_base) {
    return;
  }
  if (src_type->type_base == TYPE_BASE_INT && dst_type->type_base == TYPE_BASE_DOUBLE) {
    insert_instruction(before, OP_CONV_I_F);
    return;
  }
  if (src_type->type_base == TYPE_BASE_DOUBLE && dst_type->type_base == TYPE_BASE_INT) {
    insert_instruction(before, OP_CONV_F_I);
    return;
  }
}

void add_rval(Instruction **code, bool is_left_value, Type *type) {
  if (!is_left_value) {
    return;
  }
  switch (type->type_base) {
    case TYPE_BASE_INT:
    case TYPE_BASE_CHAR:
      add_instruction(code, OP_LOAD_I);
      break;
    case TYPE_BASE_DOUBLE:
      add_instruction(code, OP_LOAD_F);
      break;
    default:
      break;
  }
}
