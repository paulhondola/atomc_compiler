#include "../../include/vm/instruction.h"

#include <stddef.h>
#include <stdlib.h>

#include "../../include/utils/utils.h"

Instruction *add_instruction(Instruction **list, Opcode opcode) {
  Instruction *i = (Instruction *)safe_alloc(sizeof(Instruction));
  i->opcode      = opcode;
  i->next        = NULL;
  if (*list) {
    Instruction *i_list = *list;
    while (i_list->next) {
      i_list = i_list->next;
    }
    i_list->next = i;
  } else {
    *list = i;
  }
  return i;
}

Instruction *insert_instruction(Instruction *before, Opcode opcode) {
  Instruction *i = (Instruction *)safe_alloc(sizeof(Instruction));
  i->opcode      = opcode;
  i->next        = before->next;
  before->next   = i;
  return i;
}

void delete_instruction(Instruction *head) {
  Instruction *current = head;
  while (current) {
    Instruction *next = current->next;
    free(current);
    current = next;
  }
}

void delete_instructions_after(Instruction *after) {
  if (!after) {
    return;
  }
  Instruction *current = after->next;
  after->next          = NULL;
  while (current) {
    Instruction *next = current->next;
    free(current);
    current = next;
  }
}

Instruction *get_last_instruction(Instruction *list) {
  if (list) {
    while (list->next) {
      list = list->next;
    }
  }
  return list;
}

Instruction *add_instruction_with_int(Instruction **list, Opcode opcode, int arg_value) {
  Instruction *i            = add_instruction(list, opcode);
  i->argument.integer_value = arg_value;
  return i;
}

Instruction *add_instruction_with_double(Instruction **list, Opcode opcode, double arg_value) {
  Instruction *i                   = add_instruction(list, opcode);
  i->argument.floating_point_value = arg_value;
  return i;
}
