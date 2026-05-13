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

void push_double(VirtualMachine *vm, double f) {
  if (vm->stack_pointer + 1 == vm->stack + 10000) {
    err("trying to push into a full stack");
  }
  (++vm->stack_pointer)->floating_point_value = f;
}

double pop_double(VirtualMachine *vm) {
  if (vm->stack_pointer == vm->stack - 1) {
    err("trying to pop from empty vm->stack");
  }
  return vm->stack_pointer--->floating_point_value;
}

void put_int(VirtualMachine *vm) {
  fprintf(vm->output, "=> %d", pop_int(vm));
}

void put_double(VirtualMachine *vm) {
  fprintf(vm->output, "=> %g", pop_double(vm));
}

void vm_init(DomainAnalyzer *da) {
  Symbol *function = add_extern_function(da, "put_int", put_int, (Type){TYPE_BASE_VOID, NULL, -1});
  add_function_parameter(function, "i", (Type){TYPE_BASE_INT, NULL, -1});

  Symbol *put_double_function =
      add_extern_function(da, "put_double", put_double, (Type){TYPE_BASE_VOID, NULL, -1});
  add_function_parameter(put_double_function, "f", (Type){TYPE_BASE_DOUBLE, NULL, -1});
}

void run(VirtualMachine *vm, Instruction *instruction_pointer) {
  StackCellValue stack_cell_value;
  StackCellValue return_value;
  int            local_count;
  int            rhs_int;
  int            lhs_int;
  double         rhs_double;
  double         lhs_double;
  void          *address;
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
        rhs_int = pop_int(vm);
        fprintf(vm->output, "JF\t%p\t// %d",
                (void *)instruction_pointer->argument.instruction_pointer, rhs_int);
        instruction_pointer =
            rhs_int ? instruction_pointer->next : instruction_pointer->argument.instruction_pointer;
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
        rhs_int = pop_int(vm);
        lhs_int = pop_int(vm);
        push_int(vm, lhs_int + rhs_int);
        fprintf(vm->output, "ADD.integer_value\t// %d+%d -> %d", lhs_int, rhs_int, lhs_int + rhs_int);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_LESS_I:
        rhs_int = pop_int(vm);
        lhs_int = pop_int(vm);
        push_int(vm, lhs_int < rhs_int);
        fprintf(vm->output, "LESS.integer_value\t// %d<%d -> %d", lhs_int, rhs_int, lhs_int < rhs_int);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_PUSH_F:
        fprintf(vm->output, "PUSH.f\t%g", instruction_pointer->argument.floating_point_value);
        push_double(vm, instruction_pointer->argument.floating_point_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_ADD_F:
        rhs_double = pop_double(vm);
        lhs_double = pop_double(vm);
        push_double(vm, lhs_double + rhs_double);
        fprintf(vm->output, "ADD.f\t// %g+%g -> %g", lhs_double, rhs_double, lhs_double + rhs_double);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_LESS_F:
        rhs_double = pop_double(vm);
        lhs_double = pop_double(vm);
        push_int(vm, lhs_double < rhs_double);
        fprintf(vm->output, "LESS.f\t// %g<%g -> %d", lhs_double, rhs_double, lhs_double < rhs_double);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_RET:
        local_count          = instruction_pointer->argument.integer_value;
        return_value         = pop_value(vm);
        fprintf(vm->output, "RET\t%d", local_count);
        instruction_pointer  = vm->function_pointer[-1].pointer_value;
        vm->stack_pointer    = vm->function_pointer - local_count - 2;
        vm->function_pointer = vm->function_pointer[0].pointer_value;
        push_value(vm, return_value);
        break;
      case OP_JT:
        rhs_int = pop_int(vm);
        fprintf(vm->output, "JT\t%p\t// %d",
                (void *)instruction_pointer->argument.instruction_pointer, rhs_int);
        instruction_pointer =
            rhs_int ? instruction_pointer->argument.instruction_pointer : instruction_pointer->next;
        break;
      case OP_CONV_I_F:
        lhs_int = pop_int(vm);
        push_double(vm, (double)lhs_int);
        fprintf(vm->output, "CONV.i.f\t// %d -> %g", lhs_int, (double)lhs_int);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_CONV_F_I:
        lhs_double = pop_double(vm);
        push_int(vm, (int)lhs_double);
        fprintf(vm->output, "CONV.f.i\t// %g -> %d", lhs_double, (int)lhs_double);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_LOAD_I:
        address = pop_pointer(vm);
        push_int(vm, *(int *)address);
        fprintf(vm->output, "LOAD.i\t// [%p] -> %d", address, *(int *)address);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_LOAD_F:
        address = pop_pointer(vm);
        push_double(vm, *(double *)address);
        fprintf(vm->output, "LOAD.f\t// [%p] -> %g", address, *(double *)address);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_STORE_I:
        rhs_int           = pop_int(vm);
        address           = pop_pointer(vm);
        *(int *)address   = rhs_int;
        push_int(vm, rhs_int);
        fprintf(vm->output, "STORE.i\t// %d -> [%p]", rhs_int, address);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_STORE_F:
        rhs_double          = pop_double(vm);
        address             = pop_pointer(vm);
        *(double *)address  = rhs_double;
        push_double(vm, rhs_double);
        fprintf(vm->output, "STORE.f\t// %g -> [%p]", rhs_double, address);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_ADDR:
        push_pointer(vm, instruction_pointer->argument.pointer_value);
        fprintf(vm->output, "ADDR\t%p", instruction_pointer->argument.pointer_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_FPADDR_I:
        push_pointer(vm,
                     &vm->function_pointer[instruction_pointer->argument.integer_value].integer_value);
        fprintf(vm->output, "FPADDR.i\t%d", instruction_pointer->argument.integer_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_FPADDR_F:
        push_pointer(
            vm,
            &vm->function_pointer[instruction_pointer->argument.integer_value].floating_point_value);
        fprintf(vm->output, "FPADDR.f\t%d", instruction_pointer->argument.integer_value);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_SUB_I:
        rhs_int = pop_int(vm);
        lhs_int = pop_int(vm);
        push_int(vm, lhs_int - rhs_int);
        fprintf(vm->output, "SUB.i\t// %d-%d -> %d", lhs_int, rhs_int, lhs_int - rhs_int);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_SUB_F:
        rhs_double = pop_double(vm);
        lhs_double = pop_double(vm);
        push_double(vm, lhs_double - rhs_double);
        fprintf(vm->output, "SUB.f\t// %g-%g -> %g", lhs_double, rhs_double, lhs_double - rhs_double);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_MUL_I:
        rhs_int = pop_int(vm);
        lhs_int = pop_int(vm);
        push_int(vm, lhs_int * rhs_int);
        fprintf(vm->output, "MUL.i\t// %d*%d -> %d", lhs_int, rhs_int, lhs_int * rhs_int);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_MUL_F:
        rhs_double = pop_double(vm);
        lhs_double = pop_double(vm);
        push_double(vm, lhs_double * rhs_double);
        fprintf(vm->output, "MUL.f\t// %g*%g -> %g", lhs_double, rhs_double, lhs_double * rhs_double);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_DIV_I:
        rhs_int = pop_int(vm);
        lhs_int = pop_int(vm);
        if (rhs_int == 0) {
          err("run: integer division by zero");
        }
        push_int(vm, lhs_int / rhs_int);
        fprintf(vm->output, "DIV.i\t// %d/%d -> %d", lhs_int, rhs_int, lhs_int / rhs_int);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_DIV_F:
        rhs_double = pop_double(vm);
        lhs_double = pop_double(vm);
        push_double(vm, lhs_double / rhs_double);
        fprintf(vm->output, "DIV.f\t// %g/%g -> %g", lhs_double, rhs_double, lhs_double / rhs_double);
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_DROP:
        (void)pop_value(vm);
        fprintf(vm->output, "DROP");
        instruction_pointer = instruction_pointer->next;
        break;
      case OP_NOP:
        fprintf(vm->output, "NOP");
        instruction_pointer = instruction_pointer->next;
        break;
      default:
        err("run: instructiune neimplementata: %d", instruction_pointer->opcode);
    }
    fputc('\n', vm->output);
  }
}

Instruction *gen_test_program_int(DomainAnalyzer *da) {
  Instruction *code = NULL;
  add_instruction_with_int(&code, OP_PUSH_I, 2);
  Instruction *call_pos = add_instruction(&code, OP_CALL);
  add_instruction(&code, OP_HALT);
  call_pos->argument.instruction_pointer = add_instruction_with_int(&code, OP_ENTER, 1);
  // int i=0;
  add_instruction_with_int(&code, OP_PUSH_I, 0);
  add_instruction_with_int(&code, OP_FPSTORE, 1);
  // while(i<n){
  Instruction *while_pos = add_instruction_with_int(&code, OP_FPLOAD, 1);
  add_instruction_with_int(&code, OP_FPLOAD, -2);
  add_instruction(&code, OP_LESS_I);
  Instruction *jf_after = add_instruction(&code, OP_JF);
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
  add_instruction(&code, OP_JMP)->argument.instruction_pointer = while_pos;
  // returns from function
  jf_after->argument.instruction_pointer = add_instruction_with_int(&code, OP_RET_VOID, 1);
  return code;
}

/*
f(2.0);
void f(double n){		// stack frame: n[-2] ret[-1] oldFP[0] i[1]
  double i=0.0;
  while(i<n){
    put_d(i);
    i=i+0.5;
    }
  }
*/
Instruction *gen_test_program_double(DomainAnalyzer *da) {
  Instruction *code = NULL;
  add_instruction_with_double(&code, OP_PUSH_F, 2.0);
  Instruction *call_pos = add_instruction(&code, OP_CALL);
  add_instruction(&code, OP_HALT);
  call_pos->argument.instruction_pointer = add_instruction_with_int(&code, OP_ENTER, 1);
  // double i=0.0;
  add_instruction_with_double(&code, OP_PUSH_F, 0.0);
  add_instruction_with_int(&code, OP_FPSTORE, 1);
  // while(i<n){
  Instruction *while_pos = add_instruction_with_int(&code, OP_FPLOAD, 1);
  add_instruction_with_int(&code, OP_FPLOAD, -2);
  add_instruction(&code, OP_LESS_F);
  Instruction *jf_after = add_instruction(&code, OP_JF);
  // put_d(i);
  add_instruction_with_int(&code, OP_FPLOAD, 1);
  Symbol *s = find_symbol(da, "put_double");
  if (!s) {
    err("undefined: put_double");
  }
  add_instruction(&code, OP_CALL_EXT)->argument.extern_function_pointer =
      s->function.external_function_pointer;
  // i=i+0.5;
  add_instruction_with_int(&code, OP_FPLOAD, 1);
  add_instruction_with_double(&code, OP_PUSH_F, 0.5);
  add_instruction(&code, OP_ADD_F);
  add_instruction_with_int(&code, OP_FPSTORE, 1);
  // } (next iteration)
  add_instruction(&code, OP_JMP)->argument.instruction_pointer = while_pos;
  // returns from function
  jf_after->argument.instruction_pointer = add_instruction_with_int(&code, OP_RET_VOID, 1);
  return code;
}

void vm_create(VirtualMachine *vm) {
  vm->stack_pointer    = vm->stack - 1;
  vm->function_pointer = NULL;
  vm->output           = stdout;
}
