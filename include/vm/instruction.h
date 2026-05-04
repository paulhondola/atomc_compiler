#pragma once

#include <stdint.h>

// stack based virtual machine

// the instructions of the virtual machine
// FORMAT: OP_<name>.<data_type>    // [argument] effect
//		OP_ - common prefix (operation code)
//		<name> - instruction name
//		<data_type> - if present, the data type on which the instruction acts
//			.i - int
//			.f - double
//			.c - char
//			.p - pointer
//		[argument] - if present, the instruction argument
//		effect - the effect of the instruction
struct VirtualMachine;

typedef enum : uint8_t {
  OP_HALT     // ends the code execution
  ,
  OP_PUSH_I   // [ct.i] puts on stack the constant ct.i
  ,
  OP_CALL     // [instr] calls a VM function which starts with the given instruction
  ,
  OP_CALL_EXT // [native_addr] calls a host function (machine code) at the given address
  ,
  OP_ENTER    // [nb_locals] creates a function frame with the given number of local variables
  ,
  OP_RET // [nb_params] returns from a function which has the given number of parameters and returns
         // a value
  ,
  OP_RET_VOID // [nb_params] returns from a function which has the given number of parameters
              // without returning a value
  ,
  OP_CONV_I_F // converts the stack value from int to double
  ,
  OP_JMP      // [instr] unconditional jump to the specified instruction
  ,
  OP_JF       // [instr] jumps to the specified instruction if the stack value is false
  ,
  OP_JT       // [instr] jumps to the specified instruction if the stack value is true
  ,
  OP_FPLOAD   // [idx] puts on stack the value from FP[idx]
  ,
  OP_FPSTORE  // [idx] puts in FP[idx] the stack value
  ,
  OP_ADD_I    // adds 2 int values from stack and puts the result on stack
  ,
  OP_LESS_I   // compares 2 int values from stack and puts the result on stack as int

  // added instructions for code generation
  ,
  OP_PUSH_F   // [ct.f] puts on stack the constant ct.f
  ,
  OP_CONV_F_I // converts the stack value from double to int
  ,
  OP_LOAD_I   // take an adress from stack and puts back the int value from that address
  ,
  OP_LOAD_F   // take an adress from stack and puts back the double value from that address
  ,
  OP_STORE_I // takes from the stack an address and an int value and puts the value at the specified
             // address. Leaves the value on stack.
  ,
  OP_STORE_F // takes from the stack an address and a double value and puts the value at the
             // specified address. Leaves the value on stack.
  ,
  OP_ADDR    // [p] pushes on stack the given pointer
  ,
  OP_FPADDR_I // [idx] pushes on stack the address of FP[idx].i
  ,
  OP_FPADDR_F // [idx] pushes on stack the address of FP[idx].f
  ,
  OP_ADD_F    // adds 2 double values from stack and puts the result on stack
  ,
  OP_SUB_I    // subtracts 2 int values from the top of the stack and puts the result on stack
  ,
  OP_SUB_F    // subtracts 2 double values from the top of the stack and puts the result on stack
  ,
  OP_MUL_I    // multiplies 2 int values from the top of the stack and puts the result on stack
  ,
  OP_MUL_F    // multiplies 2 double values from the top of the stack and puts the result on stack
  ,
  OP_DIV_I    // divides 2 int values from the top of the stack and puts the result on stack
  ,
  OP_DIV_F    // divides 2 double values from the top of the stack and puts the result on stack
  ,
  OP_LESS_F   // compares 2 double values from stack and puts the result on stack as int
  ,
  OP_DROP     // deletes the top stack value
  ,
  OP_NOP      // no operation
} Opcode;

typedef struct Instruction Instruction;

// an universal value - used both as a stack cell and as an instruction argument
typedef union {
  int    integer_value;                                     // int and index values
  double floating_point_value;                              // float values
  void  *pointer_value;                                     // pointers
  void (*extern_function_pointer)(struct VirtualMachine *); // pointer to an extern (host) function
  Instruction *instruction_pointer;                         // pointer to an instruction
} StackCellValue;

// a VM instruction
struct Instruction {
  Opcode         opcode;   // opcode: OP_*
  StackCellValue argument; // argument: varies depending on the opcode
  Instruction   *next;     // the link to the next instruction in list
};
