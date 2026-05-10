#include "../../include/vm/vm.h"

#include <stdio.h>

#include "../../include/analyzer/domain_analyzer.h"
#include "../../include/utils/utils.h"

void push_value(VirtualMachine *vm, StackCellValue value) {
  if (vm->stack_pointer + 1 == vm->stack + 10000) {
    err("trying to push into a full stack");
  }
  *++vm->stack_pointer = value;
}

StackCellValue pop_value(VirtualMachine *vm) {
  if (vm->stack_pointer == vm->stack - 1) {
    err("trying to pop from empty vm->stack");
  }
  return *vm->stack_pointer--;
}

void push_int(VirtualMachine *vm, int i) {
  if (vm->stack_pointer + 1 == vm->stack + 10000) {
    err("trying to push into a full stack");
  }
  (++vm->stack_pointer)->integer_value = i;
}

int pop_int(VirtualMachine *vm) {
  if (vm->stack_pointer == vm->stack - 1) {
    err("trying to pop from empty vm->stack");
  }
  return vm->stack_pointer--->integer_value;
}

void push_pointer(VirtualMachine *vm, void *p) {
  if (vm->stack_pointer + 1 == vm->stack + 10000) {
    err("trying to push into a full stack");
  }
  (++vm->stack_pointer)->pointer_value = p;
}

void *pop_pointer(VirtualMachine *vm) {
  if (vm->stack_pointer == vm->stack - 1) {
    err("trying to pop from empty vm->stack");
  }
  return vm->stack_pointer--->pointer_value;
}

void put_int(VirtualMachine *vm) {
  fprintf(vm->output, "=> %d", pop_int(vm));
}

void vm_init(DomainAnalyzer *da) {
  Symbol *function = add_extern_function(da, "put_int", put_int, (Type){TYPE_BASE_VOID, NULL, -1});
  add_function_parameter(function, "i", (Type){TYPE_BASE_INT, NULL, -1});
}

void run(VirtualMachine *vm, Instruction *instruction_pointer) {
  StackCellValue stack_cell_value;
  int            local_count;
  int            rhs;
  int            lhs;
  void (*extern_function_pointer)(VirtualMachine *);
  for (;;) {
    // shows the index of the current instruction and the number of values from vm->stack
    fprintf(vm->output, "%p/%d\t", (void *)instruction_pointer,
            (int)(vm->stack_pointer - vm->stack + 1));
    switch (instruction_pointer->opcode) {
      case OP_HALT:
        fprintf(vm->output, "HALT");
        return;
      case OP_PUSH_I:
        fprintf(vm->output, "PUSH.integer_value\t%d", instruction_pointer->argument.integer_value);
        push_int(vm, instruction_pointer->argument.integer_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_CALL:
        push_pointer(vm, instruction_pointer->next);
        fprintf(vm->output, "CALL\t%p", (void *)instruction_pointer->argument.instruction_pointer);
        instruction_pointer = instruction_pointer->argument.instruction_pointer;
        break;
      case OP_CALL_EXT:
        extern_function_pointer = instruction_pointer->argument.extern_function_pointer;
        fprintf(vm->output, "CALL_EXT\t%p\n", (void *)extern_function_pointer);
        extern_function_pointer(vm);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_ENTER:
        push_pointer(vm, vm->function_pointer);
        vm->function_pointer = vm->stack_pointer;
        vm->stack_pointer += instruction_pointer->argument.integer_value;
        fprintf(vm->output, "ENTER\t%d", instruction_pointer->argument.integer_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_RET_VOID:
        local_count = instruction_pointer->argument.integer_value;
        fprintf(vm->output, "RET_VOID\t%d", local_count);
        instruction_pointer  = vm->function_pointer[-1].pointer_value;
        vm->stack_pointer    = vm->function_pointer - local_count - 2;
        vm->function_pointer = vm->function_pointer[0].pointer_value;
        break;
      case OP_JMP:
        fprintf(vm->output, "JMP\t%p",
                (void *)instruction_pointer->argument.instruction_pointer);
        instruction_pointer = instruction_pointer->argument.instruction_pointer;
        break;
      case OP_JF:
        rhs = pop_int(vm);
        fprintf(vm->output, "JF\t%p\t// %d",
                (void *)instruction_pointer->argument.instruction_pointer, rhs);
        instruction_pointer =
            rhs ? instruction_pointer->next : instruction_pointer->argument.instruction_pointer;
        break;
      case OP_FPLOAD:
        stack_cell_value = vm->function_pointer[instruction_pointer->argument.integer_value];
        push_value(vm, stack_cell_value);
        fprintf(vm->output, "FPLOAD\t%d\t// i:%d, f:%g",
                instruction_pointer->argument.integer_value, stack_cell_value.integer_value,
                stack_cell_value.floating_point_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_FPSTORE:
        stack_cell_value                                                  = pop_value(vm);
        vm->function_pointer[instruction_pointer->argument.integer_value] = stack_cell_value;
        fprintf(vm->output, "FPSTORE\t%d\t// i:%d, f:%g",
                instruction_pointer->argument.integer_value, stack_cell_value.integer_value,
                stack_cell_value.floating_point_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_ADD_I:
        rhs = pop_int(vm);
        lhs = pop_int(vm);
        push_int(vm, lhs + rhs);
        fprintf(vm->output, "ADD.integer_value\t// %d+%d -> %d", lhs, rhs, lhs + rhs);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_LESS_I:
        rhs = pop_int(vm);
        lhs = pop_int(vm);
        push_int(vm, lhs < rhs);
        fprintf(vm->output, "LESS.integer_value\t// %d<%d -> %d", lhs, rhs, lhs < rhs);
        instruction_pointer = instruction_pointer->next;
        break;
      default:
        err("run: instructiune neimplementata: %d", instruction_pointer->opcode);
    }
    fputc('\n', vm->output);
  }
}

/* The program implements the following AtomC source code:
f(2);
void f(int n){		// vm->stack frame: n[-2] ret[-1] oldFP[0] i[1]
  int i=0;
  while(i<n){
    put_i(i);
    i=i+1;
    }
  }
*/
Instruction *gen_test_program(DomainAnalyzer *da) {
  Instruction *code = NULL;
  add_instruction_with_int(&code, OP_PUSH_I, 2);
  Instruction *callPos = add_instruction(&code, OP_CALL);
  add_instruction(&code, OP_HALT);
  callPos->argument.instruction_pointer = add_instruction_with_int(&code, OP_ENTER, 1);
  // int i=0;
  add_instruction_with_int(&code, OP_PUSH_I, 0);
  add_instruction_with_int(&code, OP_FPSTORE, 1);
  // while(i<n){
  Instruction *whilePos = add_instruction_with_int(&code, OP_FPLOAD, 1);
  add_instruction_with_int(&code, OP_FPLOAD, -2);
  add_instruction(&code, OP_LESS_I);
  Instruction *jfAfter = add_instruction(&code, OP_JF);
  // put_i(i);
  add_instruction_with_int(&code, OP_FPLOAD, 1);
  Symbol *s = find_symbol(da, "put_int");
  if (!s) {
    err("undefined: put_int");
  }
  add_instruction(&code, OP_CALL_EXT)->argument.extern_function_pointer =
      s->function.external_function_pointer;
  // i=i+1;
  add_instruction_with_int(&code, OP_FPLOAD, 1);
  add_instruction_with_int(&code, OP_PUSH_I, 1);
  add_instruction(&code, OP_ADD_I);
  add_instruction_with_int(&code, OP_FPSTORE, 1);
  // } ( the next iteration)
  add_instruction(&code, OP_JMP)->argument.instruction_pointer = whilePos;
  // returns from function
  jfAfter->argument.instruction_pointer = add_instruction_with_int(&code, OP_RET_VOID, 1);
  return code;
}

void vm_create(VirtualMachine *vm) {
  vm->stack_pointer    = vm->stack - 1;
  vm->function_pointer = NULL;
  vm->output           = stdout;
}
