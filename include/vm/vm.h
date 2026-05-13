#pragma once

#include <stdio.h>

#include "instruction.h"

struct DomainAnalyzer;

#define VM_STACK_SIZE 10000

typedef struct VirtualMachine {
  StackCellValue  stack[VM_STACK_SIZE];
  StackCellValue *stack_pointer;
  StackCellValue *function_pointer;
  FILE           *output;
} VirtualMachine;

// adds a new instruction to the end of list and sets its "op" field
// returns the newly added instruction
Instruction *add_instruction(Instruction **list, Opcode opcode);

// add an instruction which has an argument of type int
Instruction *add_instruction_with_int(Instruction **list, Opcode opcode, int arg_value);

// add an instruction which has an argument of type double
Instruction *add_instruction_with_double(Instruction **list, Opcode opcode, double arg_value);

// Virtual Machine initialisation
void vm_create(VirtualMachine *virtual_machine);
void vm_init(struct DomainAnalyzer *domain_analyzer);

// Stack helpers (used by extern functions and tests)
void           push_value(VirtualMachine *virtual_machine, StackCellValue value);
StackCellValue pop_value(VirtualMachine *virtual_machine);
void           push_int(VirtualMachine *virtual_machine, int value);
int            pop_int(VirtualMachine *virtual_machine);
void           push_double(VirtualMachine *virtual_machine, double value);
double         pop_double(VirtualMachine *virtual_machine);
void           push_pointer(VirtualMachine *virtual_machine, void *pointer);
void          *pop_pointer(VirtualMachine *virtual_machine);

// executes the code starting with the given instruction (IP - Instruction Pointer)
void run(VirtualMachine *virtual_machine, Instruction *instruction_pointer);

// generates a test program (int variant)
Instruction *gen_test_program_int(struct DomainAnalyzer *domain_analyzer);

// generates a test program (double variant — lab A6 deliverable)
Instruction *gen_test_program_double(struct DomainAnalyzer *domain_analyzer);
