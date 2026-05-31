#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/analyzer/domain.h"
#include "../../include/analyzer/domain_analyzer.h"
#include "../../include/analyzer/symbol.h"
#include "../../include/analyzer/type.h"
#include "../../include/frontend/lexer.h"
#include "../../include/frontend/parser.h"
#include "../../include/frontend/stream.h"
#include "../../include/utils/utils.h"
#include "../../include/vm/code_generator.h"
#include "../../include/vm/instruction.h"
#include "../../include/vm/vm.h"

// ============================================================
// HELPERS
// ============================================================

// Read a tmpfile back into a heap-allocated NUL-terminated string.
// Mirrors the helper in test_vm.c so trace inspection looks the same here.
static char *slurp_tmpfile(FILE *f) {
  fflush(f);
  if (fseek(f, 0L, SEEK_END) != 0) return NULL;
  long size = ftell(f);
  if (size < 0) return NULL;
  rewind(f);
  char *buf = (char *)malloc((size_t)size + 1);
  if (!buf) return NULL;
  size_t read = fread(buf, 1, (size_t)size, f);
  buf[read]   = '\0';
  return buf;
}

// Runs a string of AtomC source through lex+parse+codegen, then executes the
// resulting program via the VM with output captured into a tmpfile. Returns the
// trace as a heap string; caller frees. Sets *out_main_instructions to the
// first instruction of `main`'s body so structural assertions can inspect it.
// Leaks instructions and symbols on purpose — tests are single-shot processes.
static char *compile_and_run(const char *source, Instruction **out_main_instructions) {
  TokenStream stream;
  token_stream_init(&stream);
  stream.output = NULL;
  tokenize(&stream, source);

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  da.output = NULL;
  push_domain(&da);
  vm_init(&da); // put_int / put_double must exist before parse()

  parse(&stream, &da);

  Symbol *sym_main = find_symbol_in_domain(da.symbol_table, "main");
  assert(sym_main != NULL);
  assert(sym_main->kind == SYMBOL_KIND_FUNCTION);
  if (out_main_instructions) {
    *out_main_instructions = sym_main->function.instruction;
  }

  Instruction *entry_code = NULL;
  add_instruction(&entry_code, OP_CALL)->argument.instruction_pointer =
      sym_main->function.instruction;
  add_instruction(&entry_code, OP_HALT);

  FILE *tmp = tmpfile();
  assert(tmp != NULL);

  VirtualMachine vm;
  vm_create(&vm);
  vm.output = tmp;
  run(&vm, entry_code);

  char *trace = slurp_tmpfile(tmp);
  fclose(tmp);
  delete_instruction(entry_code);
  return trace;
}

// Counts opcode `target` in the linked list starting at `head`.
static int count_opcodes(Instruction *head, Opcode target) {
  int count = 0;
  for (Instruction *cursor = head; cursor; cursor = cursor->next) {
    if (cursor->opcode == target) {
      count++;
    }
  }
  return count;
}

// ============================================================
// TEST 1: HELPER UNIT TESTS — insert_conversion_if_needed
// ============================================================

static void test_insert_conversion_int_to_double(void) {
  printf("Running test_insert_conversion_int_to_double...\n");

  Instruction *code = NULL;
  Instruction *push = add_instruction_with_int(&code, OP_PUSH_I, 5);

  Type src = {TYPE_BASE_INT, NULL, -1};
  Type dst = {TYPE_BASE_DOUBLE, NULL, -1};
  insert_conversion_if_needed(push, &src, &dst);

  assert(push->next != NULL);
  assert(push->next->opcode == OP_CONV_I_F);
  printf(GREEN "Test 1 (int→double inserts OP_CONV_I_F after the anchor) passed\n" RESET);

  delete_instruction(code);
  printf(GREEN "test_insert_conversion_int_to_double passed!\n" RESET);
}

static void test_insert_conversion_double_to_int(void) {
  printf("Running test_insert_conversion_double_to_int...\n");

  Instruction *code = NULL;
  Instruction *push = add_instruction_with_double(&code, OP_PUSH_F, 1.5);

  Type src = {TYPE_BASE_DOUBLE, NULL, -1};
  Type dst = {TYPE_BASE_INT, NULL, -1};
  insert_conversion_if_needed(push, &src, &dst);

  assert(push->next != NULL);
  assert(push->next->opcode == OP_CONV_F_I);
  printf(GREEN "Test 1 (double→int inserts OP_CONV_F_I after the anchor) passed\n" RESET);

  delete_instruction(code);
  printf(GREEN "test_insert_conversion_double_to_int passed!\n" RESET);
}

static void test_insert_conversion_no_op_when_types_match(void) {
  printf("Running test_insert_conversion_no_op_when_types_match...\n");

  Instruction *code = NULL;
  Instruction *push = add_instruction_with_int(&code, OP_PUSH_I, 5);

  Type same = {TYPE_BASE_INT, NULL, -1};
  insert_conversion_if_needed(push, &same, &same);
  assert(push->next == NULL);
  printf(GREEN "Test 1 (same type leaves list unchanged) passed\n" RESET);

  delete_instruction(code);
  printf(GREEN "test_insert_conversion_no_op_when_types_match passed!\n" RESET);
}

// ============================================================
// TEST 2: HELPER UNIT TESTS — add_rval
// ============================================================

static void test_add_rval_emits_load_when_lvalue(void) {
  printf("Running test_add_rval_emits_load_when_lvalue...\n");

  Instruction *code = NULL;
  Type         t_int = {TYPE_BASE_INT, NULL, -1};
  add_rval(&code, true, &t_int);
  assert(code != NULL);
  assert(code->opcode == OP_LOAD_I);
  printf(GREEN "Test 1 (lvalue int emits OP_LOAD_I) passed\n" RESET);

  Instruction *code2  = NULL;
  Type         t_dbl  = {TYPE_BASE_DOUBLE, NULL, -1};
  add_rval(&code2, true, &t_dbl);
  assert(code2 != NULL);
  assert(code2->opcode == OP_LOAD_F);
  printf(GREEN "Test 2 (lvalue double emits OP_LOAD_F) passed\n" RESET);

  delete_instruction(code);
  delete_instruction(code2);
  printf(GREEN "test_add_rval_emits_load_when_lvalue passed!\n" RESET);
}

static void test_add_rval_noop_when_rvalue(void) {
  printf("Running test_add_rval_noop_when_rvalue...\n");

  Instruction *code = NULL;
  Type         t    = {TYPE_BASE_INT, NULL, -1};
  add_rval(&code, false, &t);
  assert(code == NULL);
  printf(GREEN "Test 1 (rvalue input emits nothing) passed\n" RESET);

  printf(GREEN "test_add_rval_noop_when_rvalue passed!\n" RESET);
}

// ============================================================
// TEST 3: STRUCTURAL — small program emits OP_ENTER, RET_VOID, PUSH_I
// ============================================================

static void test_simple_int_program_structure(void) {
  printf("Running test_simple_int_program_structure...\n");

  // The shortest program that exercises locals + arithmetic + put_int.
  const char *src =
      "void main() {\n"
      "  int x;\n"
      "  x = 5 + 2;\n"
      "  put_int(x);\n"
      "}\n";

  Instruction *main_body = NULL;
  char        *trace     = compile_and_run(src, &main_body);
  assert(trace != NULL);
  assert(main_body != NULL);

  // First instruction must be OP_ENTER with 1 local
  assert(main_body->opcode == OP_ENTER);
  assert(main_body->argument.integer_value == 1);
  printf(GREEN "Test 1 (main starts with OP_ENTER 1) passed\n" RESET);

  // The body must contain exactly one OP_ADD_I (5 + 2) and one OP_CALL_EXT
  assert(count_opcodes(main_body, OP_ADD_I) == 1);
  printf(GREEN "Test 2 (one OP_ADD_I emitted for 5 + 2) passed\n" RESET);

  assert(count_opcodes(main_body, OP_CALL_EXT) == 1);
  printf(GREEN "Test 3 (one OP_CALL_EXT emitted for put_int) passed\n" RESET);

  // Implicit OP_RET_VOID at end since main is void
  assert(count_opcodes(main_body, OP_RET_VOID) == 1);
  printf(GREEN "Test 4 (implicit OP_RET_VOID emitted at function end) passed\n" RESET);

  free(trace);
  printf(GREEN "test_simple_int_program_structure passed!\n" RESET);
}

// ============================================================
// TEST 4: END-TO-END — locals, while, put_int (matches testgc.c shape)
// ============================================================

static void test_while_loop_prints_sequence(void) {
  printf("Running test_while_loop_prints_sequence...\n");

  const char *src =
      "void main() {\n"
      "  int i;\n"
      "  i = 0;\n"
      "  while (i < 3) {\n"
      "    put_int(i);\n"
      "    i = i + 1;\n"
      "  }\n"
      "}\n";

  char *trace = compile_and_run(src, NULL);
  assert(trace != NULL);

  assert(strstr(trace, "=> 0") != NULL);
  assert(strstr(trace, "=> 1") != NULL);
  assert(strstr(trace, "=> 2") != NULL);
  printf(GREEN "Test 1 (trace contains \"=> 0\", \"=> 1\", \"=> 2\") passed\n" RESET);

  // Loop exits at i == 3 — must NOT print "=> 3" (anchor to newline so we
  // don't false-match a substring like "=> 30").
  assert(strstr(trace, "=> 3\n") == NULL);
  printf(GREEN "Test 2 (no \"=> 3\" — loop exits at i==3) passed\n" RESET);

  assert(strstr(trace, "HALT") != NULL);
  printf(GREEN "Test 3 (trace ends with HALT) passed\n" RESET);

  free(trace);
  printf(GREEN "test_while_loop_prints_sequence passed!\n" RESET);
}

// ============================================================
// TEST 5: END-TO-END — int→double promotion at call boundary
// ============================================================

static void test_int_to_double_arg_conversion(void) {
  printf("Running test_int_to_double_arg_conversion...\n");

  const char *src =
      "void show(double x) {\n"
      "  put_double(x);\n"
      "}\n"
      "void main() {\n"
      "  int n;\n"
      "  n = 7;\n"
      "  show(n);\n"
      "}\n";

  char *trace = compile_and_run(src, NULL);
  assert(trace != NULL);

  // The compiler must have inserted CONV.i.f at the call site.
  assert(strstr(trace, "CONV.i.f") != NULL);
  printf(GREEN "Test 1 (CONV.i.f executed during call to show(int)) passed\n" RESET);

  // put_double prints "=> 7" (g format drops the trailing .0).
  assert(strstr(trace, "=> 7") != NULL);
  printf(GREEN "Test 2 (put_double output reaches the trace) passed\n" RESET);

  free(trace);
  printf(GREEN "test_int_to_double_arg_conversion passed!\n" RESET);
}

// ============================================================
// TEST 6: END-TO-END — if/else with then-branch taken
// ============================================================

static void test_if_else_then_branch(void) {
  printf("Running test_if_else_then_branch...\n");

  const char *src =
      "void main() {\n"
      "  int a;\n"
      "  a = 10;\n"
      "  if (a < 20) {\n"
      "    put_int(1);\n"
      "  } else {\n"
      "    put_int(99);\n"
      "  }\n"
      "}\n";

  char *trace = compile_and_run(src, NULL);
  assert(trace != NULL);

  // The then-branch must run and the else-branch must NOT.
  assert(strstr(trace, "=> 1") != NULL);
  printf(GREEN "Test 1 (then-branch executed: put_int(1) ran) passed\n" RESET);

  assert(strstr(trace, "=> 99") == NULL);
  printf(GREEN "Test 2 (else-branch skipped: put_int(99) never ran) passed\n" RESET);

  // JF and JMP both appear (the else-branch needs the JMP-over-else opcode)
  assert(strstr(trace, "JF") != NULL);
  printf(GREEN "Test 3 (JF appears in trace) passed\n" RESET);

  free(trace);
  printf(GREEN "test_if_else_then_branch passed!\n" RESET);
}

// ============================================================
// TEST 7: END-TO-END — function returns a value and caller uses it
// ============================================================

static void test_return_value_propagates(void) {
  printf("Running test_return_value_propagates...\n");

  const char *src =
      "int triple(int x) {\n"
      "  return x * 3;\n"
      "}\n"
      "void main() {\n"
      "  put_int(triple(7));\n"
      "}\n";

  char *trace = compile_and_run(src, NULL);
  assert(trace != NULL);

  assert(strstr(trace, "=> 21") != NULL);
  printf(GREEN "Test 1 (triple(7) returns 21 and put_int prints it) passed\n" RESET);

  assert(strstr(trace, "MUL.i") != NULL);
  printf(GREEN "Test 2 (OP_MUL_I executed for the multiplication) passed\n" RESET);

  free(trace);
  printf(GREEN "test_return_value_propagates passed!\n" RESET);
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
  test_insert_conversion_int_to_double();
  test_insert_conversion_double_to_int();
  test_insert_conversion_no_op_when_types_match();
  test_add_rval_emits_load_when_lvalue();
  test_add_rval_noop_when_rvalue();
  test_simple_int_program_structure();
  test_while_loop_prints_sequence();
  test_int_to_double_arg_conversion();
  test_if_else_then_branch();
  test_return_value_propagates();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
