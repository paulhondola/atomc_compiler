#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/utils/utils.h"
#include "../../include/analyzer/domain_analyzer.h"
#include "../../include/analyzer/symbol.h"
#include "../../include/analyzer/type.h"
#include "../../include/vm/instruction.h"
#include "../../include/vm/vm.h"

// ============================================================
// HELPERS
// ============================================================

// Read a tmpfile back into a heap-allocated NUL-terminated string.
// Caller frees. Returns NULL on read failure.
static char *slurp_tmpfile(FILE *f) {
  fflush(f);
  if (fseek(f, 0L, SEEK_END) != 0) return NULL;
  long size = ftell(f);
  if (size < 0) return NULL;
  rewind(f);
  char *buf = (char *)malloc((size_t)size + 1);
  if (!buf) return NULL;
  size_t read = fread(buf, 1, (size_t)size, f);
  buf[read] = '\0';
  return buf;
}

// Forks and returns true iff the child exits with non-zero status. The child's
// stderr is muted so err()'s diagnostic doesn't pollute the test log.
static bool child_exits_nonzero(void (*body)(void)) {
  pid_t pid = fork();
  if (pid == 0) {
    freopen("/dev/null", "w", stderr);
    body();
    exit(EXIT_SUCCESS);
  }
  int status;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS;
}

static void body_pop_int_empty(void) {
  VirtualMachine vm;
  vm_create(&vm);
  pop_int(&vm);
}

static void body_pop_double_empty(void) {
  VirtualMachine vm;
  vm_create(&vm);
  pop_double(&vm);
}

static void body_pop_pointer_empty(void) {
  VirtualMachine vm;
  vm_create(&vm);
  pop_pointer(&vm);
}

static void body_push_int_overflow(void) {
  VirtualMachine vm;
  vm_create(&vm);
  for (int i = 0; i <= VM_STACK_SIZE; i++) {
    push_int(&vm, i);
  }
}

// ============================================================
// TEST 1: VM CREATE — initial state
// ============================================================

static void test_vm_create_initial_state(void) {
  printf("Running test_vm_create_initial_state...\n");

  VirtualMachine vm;
  vm_create(&vm);

  assert(vm.stack_pointer == vm.stack - 1);
  printf(GREEN "Test 1 (stack_pointer = stack - 1 after create) passed\n" RESET);

  assert(vm.function_pointer == NULL);
  printf(GREEN "Test 2 (function_pointer = NULL after create) passed\n" RESET);

  assert(vm.output == stdout);
  printf(GREEN "Test 3 (output defaults to stdout) passed\n" RESET);

  printf(GREEN "test_vm_create_initial_state passed!\n" RESET);
}

// ============================================================
// TEST 2: INT STACK — push/pop round trips
// ============================================================

static void test_stack_int_round_trip(void) {
  printf("Running test_stack_int_round_trip...\n");

  VirtualMachine vm;
  vm_create(&vm);

  push_int(&vm, 42);
  assert(vm.stack_pointer == vm.stack);
  assert(pop_int(&vm) == 42);
  assert(vm.stack_pointer == vm.stack - 1);
  printf(GREEN "Test 1 (single int round trip) passed\n" RESET);

  push_int(&vm, 1);
  push_int(&vm, 2);
  push_int(&vm, 3);
  assert(pop_int(&vm) == 3);
  assert(pop_int(&vm) == 2);
  assert(pop_int(&vm) == 1);
  assert(vm.stack_pointer == vm.stack - 1);
  printf(GREEN "Test 2 (LIFO order across 3 pushes) passed\n" RESET);

  push_int(&vm, -7);
  assert(pop_int(&vm) == -7);
  printf(GREEN "Test 3 (negative values preserved) passed\n" RESET);

  printf(GREEN "test_stack_int_round_trip passed!\n" RESET);
}

// ============================================================
// TEST 3: DOUBLE STACK — push/pop round trips
// ============================================================

static void test_stack_double_round_trip(void) {
  printf("Running test_stack_double_round_trip...\n");

  VirtualMachine vm;
  vm_create(&vm);

  push_double(&vm, 0.5);
  assert(vm.stack_pointer == vm.stack);
  assert(pop_double(&vm) == 0.5);
  assert(vm.stack_pointer == vm.stack - 1);
  printf(GREEN "Test 1 (single double round trip) passed\n" RESET);

  push_double(&vm, 1.5);
  push_double(&vm, 2.5);
  push_double(&vm, 3.5);
  assert(pop_double(&vm) == 3.5);
  assert(pop_double(&vm) == 2.5);
  assert(pop_double(&vm) == 1.5);
  printf(GREEN "Test 2 (LIFO order across 3 pushes) passed\n" RESET);

  push_double(&vm, 0.0);
  assert(pop_double(&vm) == 0.0);
  printf(GREEN "Test 3 (zero value preserved) passed\n" RESET);

  push_double(&vm, -1.25);
  assert(pop_double(&vm) == -1.25);
  printf(GREEN "Test 4 (negative double preserved) passed\n" RESET);

  printf(GREEN "test_stack_double_round_trip passed!\n" RESET);
}

// ============================================================
// TEST 4: POINTER STACK — push/pop round trips
// ============================================================

static void test_stack_pointer_round_trip(void) {
  printf("Running test_stack_pointer_round_trip...\n");

  VirtualMachine vm;
  vm_create(&vm);

  int   sentinel = 0;
  void *p        = &sentinel;
  push_pointer(&vm, p);
  assert(pop_pointer(&vm) == p);
  printf(GREEN "Test 1 (single pointer round trip) passed\n" RESET);

  push_pointer(&vm, NULL);
  assert(pop_pointer(&vm) == NULL);
  printf(GREEN "Test 2 (NULL pointer preserved) passed\n" RESET);

  printf(GREEN "test_stack_pointer_round_trip passed!\n" RESET);
}

// ============================================================
// TEST 5: UNIVERSAL VALUE — push_value/pop_value
// ============================================================

static void test_stack_universal_value(void) {
  printf("Running test_stack_universal_value...\n");

  VirtualMachine vm;
  vm_create(&vm);

  StackCellValue in;
  in.integer_value = 99;
  push_value(&vm, in);
  StackCellValue out = pop_value(&vm);
  assert(out.integer_value == 99);
  printf(GREEN "Test 1 (int slot of union preserved) passed\n" RESET);

  in.floating_point_value = 2.5;
  push_value(&vm, in);
  out = pop_value(&vm);
  assert(out.floating_point_value == 2.5);
  printf(GREEN "Test 2 (double slot of union preserved) passed\n" RESET);

  printf(GREEN "test_stack_universal_value passed!\n" RESET);
}

// ============================================================
// TEST 6: STACK UNDERFLOW / OVERFLOW — fork-based error tests
// ============================================================

static void test_stack_error_conditions(void) {
  printf("Running test_stack_error_conditions...\n");

  assert(child_exits_nonzero(body_pop_int_empty));
  printf(GREEN "Test 1 (pop_int on empty stack -> error) passed\n" RESET);

  assert(child_exits_nonzero(body_pop_double_empty));
  printf(GREEN "Test 2 (pop_double on empty stack -> error) passed\n" RESET);

  assert(child_exits_nonzero(body_pop_pointer_empty));
  printf(GREEN "Test 3 (pop_pointer on empty stack -> error) passed\n" RESET);

  assert(child_exits_nonzero(body_push_int_overflow));
  printf(GREEN "Test 4 (push beyond VM_STACK_SIZE -> error) passed\n" RESET);

  printf(GREEN "test_stack_error_conditions passed!\n" RESET);
}

// ============================================================
// TEST 7: INSTRUCTION LIST CONSTRUCTION
// ============================================================

static void test_instruction_list_construction(void) {
  printf("Running test_instruction_list_construction...\n");

  Instruction *code = NULL;
  Instruction *i1   = add_instruction(&code, OP_HALT);
  assert(code == i1);
  assert(i1->opcode == OP_HALT);
  assert(i1->next == NULL);
  printf(GREEN "Test 1 (first add sets head and returns new node) passed\n" RESET);

  Instruction *i2 = add_instruction(&code, OP_JMP);
  assert(code == i1);
  assert(i1->next == i2);
  assert(i2->next == NULL);
  printf(GREEN "Test 2 (subsequent adds append to tail) passed\n" RESET);

  Instruction *i3 = add_instruction(&code, OP_NOP);
  assert(i2->next == i3);
  assert(i3->opcode == OP_NOP);
  printf(GREEN "Test 3 (third instruction linked correctly) passed\n" RESET);

  printf(GREEN "test_instruction_list_construction passed!\n" RESET);
}

// ============================================================
// TEST 8: INSTRUCTION WITH INT ARGUMENT
// ============================================================

static void test_instruction_with_int(void) {
  printf("Running test_instruction_with_int...\n");

  Instruction *code = NULL;
  Instruction *ins  = add_instruction_with_int(&code, OP_PUSH_I, 7);
  assert(ins->opcode == OP_PUSH_I);
  assert(ins->argument.integer_value == 7);
  printf(GREEN "Test 1 (opcode and integer argument stored) passed\n" RESET);

  Instruction *ins2 = add_instruction_with_int(&code, OP_FPLOAD, -2);
  assert(ins2->argument.integer_value == -2);
  printf(GREEN "Test 2 (negative integer argument preserved) passed\n" RESET);

  printf(GREEN "test_instruction_with_int passed!\n" RESET);
}

// ============================================================
// TEST 9: INSTRUCTION WITH DOUBLE ARGUMENT
// ============================================================

static void test_instruction_with_double(void) {
  printf("Running test_instruction_with_double...\n");

  Instruction *code = NULL;
  Instruction *ins  = add_instruction_with_double(&code, OP_PUSH_F, 2.0);
  assert(ins->opcode == OP_PUSH_F);
  assert(ins->argument.floating_point_value == 2.0);
  printf(GREEN "Test 1 (opcode and double argument stored) passed\n" RESET);

  Instruction *ins2 = add_instruction_with_double(&code, OP_PUSH_F, 0.5);
  assert(ins2->argument.floating_point_value == 0.5);
  printf(GREEN "Test 2 (fractional double argument preserved) passed\n" RESET);

  printf(GREEN "test_instruction_with_double passed!\n" RESET);
}

// ============================================================
// TEST 10: VM_INIT — registers put_int and put_double externs
// ============================================================

static void test_vm_init_registers_externs(void) {
  printf("Running test_vm_init_registers_externs...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);
  vm_init(&da);

  Symbol *put_int_sym = find_symbol(&da, "put_int");
  assert(put_int_sym != NULL);
  assert(put_int_sym->kind == SYMBOL_KIND_FUNCTION);
  assert(put_int_sym->function.external_function_pointer != NULL);
  assert(put_int_sym->type.type_base == TYPE_BASE_VOID);
  printf(GREEN "Test 1 (put_int registered as void extern) passed\n" RESET);

  assert(symbols_len(put_int_sym->function.parameters) == 1);
  assert(put_int_sym->function.parameters->type.type_base == TYPE_BASE_INT);
  printf(GREEN "Test 2 (put_int has 1 int parameter) passed\n" RESET);

  Symbol *put_double_sym = find_symbol(&da, "put_double");
  assert(put_double_sym != NULL);
  assert(put_double_sym->kind == SYMBOL_KIND_FUNCTION);
  assert(put_double_sym->function.external_function_pointer != NULL);
  assert(put_double_sym->type.type_base == TYPE_BASE_VOID);
  printf(GREEN "Test 3 (put_double registered as void extern) passed\n" RESET);

  assert(symbols_len(put_double_sym->function.parameters) == 1);
  assert(put_double_sym->function.parameters->type.type_base == TYPE_BASE_DOUBLE);
  printf(GREEN "Test 4 (put_double has 1 double parameter) passed\n" RESET);

  assert(put_int_sym->function.external_function_pointer !=
         put_double_sym->function.external_function_pointer);
  printf(GREEN "Test 5 (put_int and put_double point to distinct functions) passed\n" RESET);

  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_vm_init_registers_externs passed!\n" RESET);
}

// ============================================================
// TEST 11: gen_test_program_int — structural assertions
// ============================================================

static void test_gen_test_program_int_structure(void) {
  printf("Running test_gen_test_program_int_structure...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);
  vm_init(&da);

  Instruction *code = gen_test_program_int(&da);
  assert(code != NULL);
  printf(GREEN "Test 1 (returns non-null code) passed\n" RESET);

  // First instruction: PUSH.i 2 (the argument passed to f(2))
  assert(code->opcode == OP_PUSH_I);
  assert(code->argument.integer_value == 2);
  printf(GREEN "Test 2 (first opcode is PUSH_I with arg 2) passed\n" RESET);

  // Second: CALL; Third: HALT
  assert(code->next != NULL && code->next->opcode == OP_CALL);
  assert(code->next->next != NULL && code->next->next->opcode == OP_HALT);
  printf(GREEN "Test 3 (CALL then HALT follow PUSH_I) passed\n" RESET);

  // CALL must target the ENTER that begins f's body
  Instruction *target = code->next->argument.instruction_pointer;
  assert(target != NULL);
  assert(target->opcode == OP_ENTER);
  assert(target->argument.integer_value == 1);
  printf(GREEN "Test 4 (CALL targets ENTER 1) passed\n" RESET);

  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_gen_test_program_int_structure passed!\n" RESET);
}

// ============================================================
// TEST 12: gen_test_program_double — structural assertions
// ============================================================

static void test_gen_test_program_double_structure(void) {
  printf("Running test_gen_test_program_double_structure...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);
  vm_init(&da);

  Instruction *code = gen_test_program_double(&da);
  assert(code != NULL);
  printf(GREEN "Test 1 (returns non-null code) passed\n" RESET);

  // First instruction: PUSH.f 2.0
  assert(code->opcode == OP_PUSH_F);
  assert(code->argument.floating_point_value == 2.0);
  printf(GREEN "Test 2 (first opcode is PUSH_F with arg 2.0) passed\n" RESET);

  // Second: CALL; Third: HALT
  assert(code->next != NULL && code->next->opcode == OP_CALL);
  assert(code->next->next != NULL && code->next->next->opcode == OP_HALT);
  printf(GREEN "Test 3 (CALL then HALT follow PUSH_F) passed\n" RESET);

  Instruction *target = code->next->argument.instruction_pointer;
  assert(target != NULL);
  assert(target->opcode == OP_ENTER);
  assert(target->argument.integer_value == 1);
  printf(GREEN "Test 4 (CALL targets ENTER 1) passed\n" RESET);

  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_gen_test_program_double_structure passed!\n" RESET);
}

// ============================================================
// TEST 13: End-to-end — run gen_test_program_int and check trace
// ============================================================

static void test_run_int_program(void) {
  printf("Running test_run_int_program...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);
  vm_init(&da);

  Instruction *code = gen_test_program_int(&da);

  FILE *tmp = tmpfile();
  assert(tmp != NULL);

  VirtualMachine vm;
  vm_create(&vm);
  vm.output = tmp;

  run(&vm, code);

  char *trace = slurp_tmpfile(tmp);
  assert(trace != NULL);

  // Loop runs for i = 0, 1 (since n = 2): prints "=> 0" and "=> 1"
  assert(strstr(trace, "=> 0") != NULL);
  printf(GREEN "Test 1 (trace contains \"=> 0\") passed\n" RESET);

  assert(strstr(trace, "=> 1") != NULL);
  printf(GREEN "Test 2 (trace contains \"=> 1\") passed\n" RESET);

  // Loop must exit at i = 2 (no "=> 2" output)
  assert(strstr(trace, "=> 2") == NULL);
  printf(GREEN "Test 3 (no \"=> 2\" — loop exits at i==n) passed\n" RESET);

  // Program ends with HALT
  assert(strstr(trace, "HALT") != NULL);
  printf(GREEN "Test 4 (trace ends with HALT) passed\n" RESET);

  free(trace);
  fclose(tmp);
  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_run_int_program passed!\n" RESET);
}

// ============================================================
// TEST 14: End-to-end — run gen_test_program_double and check trace
// ============================================================

static void test_run_double_program(void) {
  printf("Running test_run_double_program...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);
  vm_init(&da);

  Instruction *code = gen_test_program_double(&da);

  FILE *tmp = tmpfile();
  assert(tmp != NULL);

  VirtualMachine vm;
  vm_create(&vm);
  vm.output = tmp;

  run(&vm, code);

  char *trace = slurp_tmpfile(tmp);
  assert(trace != NULL);

  // Loop iterates for i = 0.0, 0.5, 1.0, 1.5 (since n = 2.0, step = 0.5)
  assert(strstr(trace, "=> 0") != NULL);
  printf(GREEN "Test 1 (trace contains \"=> 0\") passed\n" RESET);

  assert(strstr(trace, "=> 0.5") != NULL);
  printf(GREEN "Test 2 (trace contains \"=> 0.5\") passed\n" RESET);

  assert(strstr(trace, "=> 1") != NULL);
  printf(GREEN "Test 3 (trace contains \"=> 1\") passed\n" RESET);

  assert(strstr(trace, "=> 1.5") != NULL);
  printf(GREEN "Test 4 (trace contains \"=> 1.5\") passed\n" RESET);

  // No "=> 2" — loop exits when i == n
  // Use \n boundary so this doesn't match a substring of "=> 2.0..." etc.
  assert(strstr(trace, "=> 2\n") == NULL);
  printf(GREEN "Test 5 (no \"=> 2\" — loop exits at i==n) passed\n" RESET);

  // Float opcodes must appear in the trace (they're newly implemented)
  assert(strstr(trace, "PUSH.f") != NULL);
  printf(GREEN "Test 6 (PUSH.f appears in trace) passed\n" RESET);

  assert(strstr(trace, "ADD.f") != NULL);
  printf(GREEN "Test 7 (ADD.f appears in trace) passed\n" RESET);

  assert(strstr(trace, "LESS.f") != NULL);
  printf(GREEN "Test 8 (LESS.f appears in trace) passed\n" RESET);

  assert(strstr(trace, "HALT") != NULL);
  printf(GREEN "Test 9 (trace ends with HALT) passed\n" RESET);

  free(trace);
  fclose(tmp);
  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_run_double_program passed!\n" RESET);
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
  test_vm_create_initial_state();
  test_stack_int_round_trip();
  test_stack_double_round_trip();
  test_stack_pointer_round_trip();
  test_stack_universal_value();
  test_stack_error_conditions();
  test_instruction_list_construction();
  test_instruction_with_int();
  test_instruction_with_double();
  test_vm_init_registers_externs();
  test_gen_test_program_int_structure();
  test_gen_test_program_double_structure();
  test_run_int_program();
  test_run_double_program();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
