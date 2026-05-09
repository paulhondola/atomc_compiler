#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/utils/utils.h"
#include "../../include/analyzer/type_analyzer.h"
#include "../../include/analyzer/type.h"
#include "../../include/analyzer/symbol.h"
#include "../../include/analyzer/domain_analyzer.h"
#include "../../include/frontend/lexer.h"
#include "../../include/frontend/parser.h"
#include "../../include/frontend/stream.h"

// ============================================================
// HELPERS
// ============================================================

static void parse_src(const char *src) {
  TokenStream    stream;
  DomainAnalyzer da;
  token_stream_init(&stream);
  domain_analyzer_init(&da);
  tokenize(&stream, src);
  push_domain(&da);
  parse(&stream, &da);
  drop_domain(&da);
  token_stream_free(&stream);
  domain_analyzer_free(&da);
}

static bool exits_with_error(const char *src) {
  pid_t pid = fork();
  if (pid == 0) {
    freopen("/dev/null", "w", stderr);
    parse_src(src);
    exit(EXIT_SUCCESS);
  }
  int status;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS;
}

// ============================================================
// TEST 1: can_be_scalar
// ============================================================

static void test_can_be_scalar(void) {
  printf("Running test_can_be_scalar...\n");

  ReturnValue rv;

  // Scalar INT → true
  rv = (ReturnValue){ .type = {TYPE_BASE_INT,    NULL, -1}, .is_left_value = false };
  assert(can_be_scalar(&rv) == true);
  printf(GREEN "Test 1 (scalar INT → true) passed\n" RESET);

  // Scalar DOUBLE → true
  rv = (ReturnValue){ .type = {TYPE_BASE_DOUBLE, NULL, -1} };
  assert(can_be_scalar(&rv) == true);
  printf(GREEN "Test 2 (scalar DOUBLE → true) passed\n" RESET);

  // Scalar CHAR → true
  rv = (ReturnValue){ .type = {TYPE_BASE_CHAR,   NULL, -1} };
  assert(can_be_scalar(&rv) == true);
  printf(GREEN "Test 3 (scalar CHAR → true) passed\n" RESET);

  // Scalar VOID → false
  rv = (ReturnValue){ .type = {TYPE_BASE_VOID,   NULL, -1} };
  assert(can_be_scalar(&rv) == false);
  printf(GREEN "Test 4 (scalar VOID → false) passed\n" RESET);

  // STRUCT scalar → false
  Symbol *s  = new_symbol("S", SYMBOL_KIND_STRUCT);
  s->type    = (Type){TYPE_BASE_STRUCT, s, -1};
  rv = (ReturnValue){ .type = {TYPE_BASE_STRUCT, s, -1} };
  assert(can_be_scalar(&rv) == false);
  printf(GREEN "Test 5 (STRUCT scalar → false) passed\n" RESET);
  free_symbol(s);

  // INT array with explicit dim → false
  rv = (ReturnValue){ .type = {TYPE_BASE_INT, NULL, 5} };
  assert(can_be_scalar(&rv) == false);
  printf(GREEN "Test 6 (INT[5] → false) passed\n" RESET);

  // INT array without dim (dim=0) → false
  rv = (ReturnValue){ .type = {TYPE_BASE_INT, NULL, 0} };
  assert(can_be_scalar(&rv) == false);
  printf(GREEN "Test 7 (INT[] → false) passed\n" RESET);

  // DOUBLE array → false
  rv = (ReturnValue){ .type = {TYPE_BASE_DOUBLE, NULL, 3} };
  assert(can_be_scalar(&rv) == false);
  printf(GREEN "Test 8 (DOUBLE[3] → false) passed\n" RESET);

  printf(GREEN "test_can_be_scalar passed!\n" RESET);
}

// ============================================================
// TEST 2: convert_to
// ============================================================

static void test_convert_to(void) {
  printf("Running test_convert_to...\n");

  Type src, dst;

  // INT → INT
  src = (Type){TYPE_BASE_INT,    NULL, -1};
  dst = (Type){TYPE_BASE_INT,    NULL, -1};
  assert(convert_to(&src, &dst) == true);
  printf(GREEN "Test 1 (INT→INT) passed\n" RESET);

  // INT → DOUBLE
  src = (Type){TYPE_BASE_INT,    NULL, -1};
  dst = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  assert(convert_to(&src, &dst) == true);
  printf(GREEN "Test 2 (INT→DOUBLE) passed\n" RESET);

  // DOUBLE → CHAR
  src = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  dst = (Type){TYPE_BASE_CHAR,   NULL, -1};
  assert(convert_to(&src, &dst) == true);
  printf(GREEN "Test 3 (DOUBLE→CHAR) passed\n" RESET);

  // CHAR → INT
  src = (Type){TYPE_BASE_CHAR, NULL, -1};
  dst = (Type){TYPE_BASE_INT,  NULL, -1};
  assert(convert_to(&src, &dst) == true);
  printf(GREEN "Test 4 (CHAR→INT) passed\n" RESET);

  // INT → VOID → false
  src = (Type){TYPE_BASE_INT,  NULL, -1};
  dst = (Type){TYPE_BASE_VOID, NULL, -1};
  assert(convert_to(&src, &dst) == false);
  printf(GREEN "Test 5 (INT→VOID → false) passed\n" RESET);

  // VOID → INT → false
  src = (Type){TYPE_BASE_VOID, NULL, -1};
  dst = (Type){TYPE_BASE_INT,  NULL, -1};
  assert(convert_to(&src, &dst) == false);
  printf(GREEN "Test 6 (VOID→INT → false) passed\n" RESET);

  // STRUCT → same struct symbol → true
  Symbol *sa = new_symbol("A", SYMBOL_KIND_STRUCT);
  sa->type   = (Type){TYPE_BASE_STRUCT, sa, -1};
  src = (Type){TYPE_BASE_STRUCT, sa, -1};
  dst = (Type){TYPE_BASE_STRUCT, sa, -1};
  assert(convert_to(&src, &dst) == true);
  printf(GREEN "Test 7 (STRUCT→same STRUCT) passed\n" RESET);

  // STRUCT → different struct symbol → false
  Symbol *sb = new_symbol("B", SYMBOL_KIND_STRUCT);
  sb->type   = (Type){TYPE_BASE_STRUCT, sb, -1};
  dst = (Type){TYPE_BASE_STRUCT, sb, -1};
  assert(convert_to(&src, &dst) == false);
  printf(GREEN "Test 8 (STRUCT→different STRUCT → false) passed\n" RESET);

  free_symbol(sa);
  free_symbol(sb);

  // array → array (any base, any dim) → true
  src = (Type){TYPE_BASE_INT,  NULL, 5};
  dst = (Type){TYPE_BASE_CHAR, NULL, 0};
  assert(convert_to(&src, &dst) == true);
  printf(GREEN "Test 9 (array→array) passed\n" RESET);

  // array → scalar → false
  src = (Type){TYPE_BASE_INT, NULL, 5};
  dst = (Type){TYPE_BASE_INT, NULL, -1};
  assert(convert_to(&src, &dst) == false);
  printf(GREEN "Test 10 (array→scalar → false) passed\n" RESET);

  printf(GREEN "test_convert_to passed!\n" RESET);
}

// ============================================================
// TEST 3: arithmetic_type_to
// ============================================================

static void test_arithmetic_type_to(void) {
  printf("Running test_arithmetic_type_to...\n");

  Type a, b, dst;

  // INT + INT → INT
  a = (Type){TYPE_BASE_INT, NULL, -1}; b = a;
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_INT);
  assert(dst.array_dimension == -1);
  assert(dst.symbol == NULL);
  printf(GREEN "Test 1 (INT+INT→INT) passed\n" RESET);

  // INT + DOUBLE → DOUBLE
  a = (Type){TYPE_BASE_INT,    NULL, -1};
  b = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_DOUBLE);
  printf(GREEN "Test 2 (INT+DOUBLE→DOUBLE) passed\n" RESET);

  // INT + CHAR → INT
  a = (Type){TYPE_BASE_INT,  NULL, -1};
  b = (Type){TYPE_BASE_CHAR, NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_INT);
  printf(GREEN "Test 3 (INT+CHAR→INT) passed\n" RESET);

  // DOUBLE + INT → DOUBLE
  a = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  b = (Type){TYPE_BASE_INT,    NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_DOUBLE);
  printf(GREEN "Test 4 (DOUBLE+INT→DOUBLE) passed\n" RESET);

  // DOUBLE + DOUBLE → DOUBLE
  a = (Type){TYPE_BASE_DOUBLE, NULL, -1}; b = a;
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_DOUBLE);
  printf(GREEN "Test 5 (DOUBLE+DOUBLE→DOUBLE) passed\n" RESET);

  // DOUBLE + CHAR → DOUBLE
  a = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  b = (Type){TYPE_BASE_CHAR,   NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_DOUBLE);
  printf(GREEN "Test 6 (DOUBLE+CHAR→DOUBLE) passed\n" RESET);

  // CHAR + CHAR → CHAR
  a = (Type){TYPE_BASE_CHAR, NULL, -1}; b = a;
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_CHAR);
  printf(GREEN "Test 7 (CHAR+CHAR→CHAR) passed\n" RESET);

  // CHAR + INT → INT
  a = (Type){TYPE_BASE_CHAR, NULL, -1};
  b = (Type){TYPE_BASE_INT,  NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_INT);
  printf(GREEN "Test 8 (CHAR+INT→INT) passed\n" RESET);

  // CHAR + DOUBLE → DOUBLE
  a = (Type){TYPE_BASE_CHAR,   NULL, -1};
  b = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == true);
  assert(dst.type_base == TYPE_BASE_DOUBLE);
  printf(GREEN "Test 9 (CHAR+DOUBLE→DOUBLE) passed\n" RESET);

  // VOID + INT → false
  a = (Type){TYPE_BASE_VOID, NULL, -1};
  b = (Type){TYPE_BASE_INT,  NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == false);
  printf(GREEN "Test 10 (VOID+INT → false) passed\n" RESET);

  // INT + VOID → false
  a = (Type){TYPE_BASE_INT,  NULL, -1};
  b = (Type){TYPE_BASE_VOID, NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == false);
  printf(GREEN "Test 11 (INT+VOID → false) passed\n" RESET);

  // INT array + INT → false (array rejected)
  a = (Type){TYPE_BASE_INT, NULL, 3};
  b = (Type){TYPE_BASE_INT, NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == false);
  printf(GREEN "Test 12 (INT[3]+INT → false) passed\n" RESET);

  // INT + INT array → false
  a = (Type){TYPE_BASE_INT, NULL, -1};
  b = (Type){TYPE_BASE_INT, NULL, 2};
  assert(arithmetic_type_to(&a, &b, &dst) == false);
  printf(GREEN "Test 13 (INT+INT[2] → false) passed\n" RESET);

  // STRUCT + INT → false
  Symbol *s  = new_symbol("S", SYMBOL_KIND_STRUCT);
  s->type    = (Type){TYPE_BASE_STRUCT, s, -1};
  a = s->type;
  b = (Type){TYPE_BASE_INT, NULL, -1};
  assert(arithmetic_type_to(&a, &b, &dst) == false);
  printf(GREEN "Test 14 (STRUCT+INT → false) passed\n" RESET);

  // INT + STRUCT → false
  a = (Type){TYPE_BASE_INT, NULL, -1};
  b = s->type;
  assert(arithmetic_type_to(&a, &b, &dst) == false);
  printf(GREEN "Test 15 (INT+STRUCT → false) passed\n" RESET);
  free_symbol(s);

  printf(GREEN "test_arithmetic_type_to passed!\n" RESET);
}

// ============================================================
// TEST 4: INTEGRATION — valid type conversions and assignments
// ============================================================

static void test_integration_valid_type_conversions(void) {
  printf("Running test_integration_valid_type_conversions...\n");

  // int ← double (narrowing is allowed)
  parse_src("void f() { int x; double y; x = y; }");
  printf(GREEN "Test 1 (int = double) passed\n" RESET);

  // double ← int
  parse_src("void f() { double x; int y; x = y; }");
  printf(GREEN "Test 2 (double = int) passed\n" RESET);

  // char ← int
  parse_src("void f() { char c; int i; c = i; }");
  printf(GREEN "Test 3 (char = int) passed\n" RESET);

  // int ← char
  parse_src("void f() { int i; char c; i = c; }");
  printf(GREEN "Test 4 (int = char) passed\n" RESET);

  // char ← double
  parse_src("void f() { char c; double d; c = d; }");
  printf(GREEN "Test 5 (char = double) passed\n" RESET);

  // double ← char
  parse_src("void f() { double d; char c; d = c; }");
  printf(GREEN "Test 6 (double = char) passed\n" RESET);

  // return int from double function — both scalar, convert_to(INT, DOUBLE) ok
  parse_src("double f() { int x; return x; }");
  printf(GREEN "Test 7 (double fn returns int) passed\n" RESET);

  // return char from int function
  parse_src("int f() { char c; return c; }");
  printf(GREEN "Test 8 (int fn returns char) passed\n" RESET);

  // struct argument to function expecting same struct type — convert_to(S,S) = true
  parse_src("struct S { int v; }; void g(struct S s) {} void f() { struct S s; g(s); }");
  printf(GREEN "Test 9 (same struct passed to param) passed\n" RESET);

  printf(GREEN "test_integration_valid_type_conversions passed!\n" RESET);
}

// ============================================================
// TEST 5: INTEGRATION — valid arithmetic
// ============================================================

static void test_integration_valid_arithmetic(void) {
  printf("Running test_integration_valid_arithmetic...\n");

  // INT + INT
  parse_src("void f() { int a; int b; int c; c = a + b; }");
  printf(GREEN "Test 1 (int+int) passed\n" RESET);

  // INT * DOUBLE
  parse_src("void f() { int a; double b; double c; c = a * b; }");
  printf(GREEN "Test 2 (int*double) passed\n" RESET);

  // CHAR - CHAR
  parse_src("void f() { char a; char b; char c; c = a - b; }");
  printf(GREEN "Test 3 (char-char) passed\n" RESET);

  // DOUBLE / INT
  parse_src("void f() { double a; int b; double c; c = a / b; }");
  printf(GREEN "Test 4 (double/int) passed\n" RESET);

  // Unary minus on int
  parse_src("void f() { int a; int b; b = -a; }");
  printf(GREEN "Test 5 (unary minus int) passed\n" RESET);

  // Unary ! on int
  parse_src("void f() { int a; int b; b = !a; }");
  printf(GREEN "Test 6 (unary ! int) passed\n" RESET);

  // Unary minus on double
  parse_src("void f() { double a; double b; b = -a; }");
  printf(GREEN "Test 7 (unary minus double) passed\n" RESET);

  // Relational: int < double
  parse_src("void f() { int a; double b; int c; c = a < b; }");
  printf(GREEN "Test 8 (int < double) passed\n" RESET);

  // Equality: char == int
  parse_src("void f() { char a; int b; int c; c = a == b; }");
  printf(GREEN "Test 9 (char == int) passed\n" RESET);

  // Complex expression: (int + double) * char
  parse_src("void f() { int a; double b; char c; double d; d = (a + b) * c; }");
  printf(GREEN "Test 10 (mixed arithmetic) passed\n" RESET);

  // Array indexing returns scalar — usable in arithmetic
  parse_src("void f() { int arr[5]; int i; int x; x = arr[i] + 1; }");
  printf(GREEN "Test 11 (array element in arithmetic) passed\n" RESET);

  printf(GREEN "test_integration_valid_arithmetic passed!\n" RESET);
}

// ============================================================
// TEST 6: INTEGRATION — valid conditions
// ============================================================

static void test_integration_valid_conditions(void) {
  printf("Running test_integration_valid_conditions...\n");

  // if with int
  parse_src("void f() { int x; if (x) {} }");
  printf(GREEN "Test 1 (if int) passed\n" RESET);

  // if with double
  parse_src("void f() { double d; if (d) {} }");
  printf(GREEN "Test 2 (if double) passed\n" RESET);

  // if with char
  parse_src("void f() { char c; if (c) {} }");
  printf(GREEN "Test 3 (if char) passed\n" RESET);

  // while with int
  parse_src("void f() { int x; while (x) {} }");
  printf(GREEN "Test 4 (while int) passed\n" RESET);

  // while with double
  parse_src("void f() { double d; while (d) {} }");
  printf(GREEN "Test 5 (while double) passed\n" RESET);

  // if with arithmetic result
  parse_src("void f() { int a; int b; if (a + b) {} }");
  printf(GREEN "Test 6 (if arithmetic) passed\n" RESET);

  // if with relational result
  parse_src("void f() { int a; int b; if (a < b) {} }");
  printf(GREEN "Test 7 (if relational) passed\n" RESET);

  // if with array element — arr[i] is scalar
  parse_src("void f() { int arr[3]; int i; if (arr[i]) {} }");
  printf(GREEN "Test 8 (if array element) passed\n" RESET);

  // nested if
  parse_src("void f() { int x; int y; if (x) { if (y) {} } }");
  printf(GREEN "Test 9 (nested if scalar conditions) passed\n" RESET);

  printf(GREEN "test_integration_valid_conditions passed!\n" RESET);
}

// ============================================================
// TEST 7: ERRORS — non-scalar in conditions
// ============================================================

static void test_error_nonscalar_conditions(void) {
  printf("Running test_error_nonscalar_conditions...\n");

  // int array in if condition — "the if condition must be a scalar value"
  assert(exits_with_error("void f() { int arr[3]; if (arr) {} }"));
  printf(GREEN "Test 1 (if array → error) passed\n" RESET);

  // struct in if condition
  assert(exits_with_error("struct S { int v; }; void f() { struct S s; if (s) {} }"));
  printf(GREEN "Test 2 (if struct → error) passed\n" RESET);

  // int array in while condition — "the while condition must be a scalar value"
  assert(exits_with_error("void f() { int arr[3]; while (arr) {} }"));
  printf(GREEN "Test 3 (while array → error) passed\n" RESET);

  // struct in while condition
  assert(exits_with_error("struct S { int v; }; void f() { struct S s; while (s) {} }"));
  printf(GREEN "Test 4 (while struct → error) passed\n" RESET);

  printf(GREEN "test_error_nonscalar_conditions passed!\n" RESET);
}

// ============================================================
// TEST 8: ERRORS — assignment type violations
// ============================================================

static void test_error_assignment_types(void) {
  printf("Running test_error_assignment_types...\n");

  // Struct on left side — "the assign destination must be scalar"
  assert(exits_with_error(
    "struct S { int v; }; void f() { struct S s; int i; s = i; }"));
  printf(GREEN "Test 1 (struct = int → error) passed\n" RESET);

  // Two different structs — still struct on left, still "must be scalar"
  assert(exits_with_error(
    "struct A { int v; }; struct B { int v; }; "
    "void f() { struct A a; struct B b; a = b; }"));
  printf(GREEN "Test 2 (struct A = struct B → error) passed\n" RESET);

  // Array variable on left side — "the assign destination cannot be constant"
  assert(exits_with_error("void f() { int arr[5]; int x; arr = x; }"));
  printf(GREEN "Test 3 (array = int → error) passed\n" RESET);

  printf(GREEN "test_error_assignment_types passed!\n" RESET);
}

// ============================================================
// TEST 9: ERRORS — return type violations
// ============================================================

static void test_error_return_types(void) {
  printf("Running test_error_return_types...\n");

  // Return array from int function — "the return value must be a scalar value"
  assert(exits_with_error("int f() { int arr[3]; return arr; }"));
  printf(GREEN "Test 1 (return array from int fn → error) passed\n" RESET);

  // Return struct from int function — "the return value must be a scalar value"
  assert(exits_with_error(
    "struct S { int v; }; int f() { struct S s; return s; }"));
  printf(GREEN "Test 2 (return struct from int fn → error) passed\n" RESET);

  printf(GREEN "test_error_return_types passed!\n" RESET);
}

// ============================================================
// TEST 10: ERRORS — arithmetic type violations
// ============================================================

static void test_error_arithmetic_types(void) {
  printf("Running test_error_arithmetic_types...\n");

  // int array as first operand of + — "invalid operand type for + or -"
  assert(exits_with_error("void f() { int arr[3]; int x; x = arr + 1; }"));
  printf(GREEN "Test 1 (int[]+int → error) passed\n" RESET);

  // int array as second operand of +
  assert(exits_with_error("void f() { int arr[3]; int x; x = 1 + arr; }"));
  printf(GREEN "Test 2 (int+int[] → error) passed\n" RESET);

  // struct in + operand — "invalid operand type for + or -"
  assert(exits_with_error(
    "struct S { int v; }; void f() { struct S s; int x; x = s + 1; }"));
  printf(GREEN "Test 3 (struct+int → error) passed\n" RESET);

  // struct as second operand of -
  assert(exits_with_error(
    "struct S { int v; }; void f() { struct S s; int x; x = 1 - s; }"));
  printf(GREEN "Test 4 (int-struct → error) passed\n" RESET);

  // struct * struct — "invalid operand type for * or /"
  assert(exits_with_error(
    "struct S { int v; }; void f() { struct S a; struct S b; int x; x = a * b; }"));
  printf(GREEN "Test 5 (struct*struct → error) passed\n" RESET);

  // array in < relational — "invalid operand type for <, <=, >, >="
  assert(exits_with_error("void f() { int arr[3]; int x; x = arr < 1; }"));
  printf(GREEN "Test 6 (array<int relational → error) passed\n" RESET);

  // struct in == equality — "invalid operand type for == or !="
  assert(exits_with_error(
    "struct S { int v; }; void f() { struct S s; int x; x = s == 1; }"));
  printf(GREEN "Test 7 (struct==int equality → error) passed\n" RESET);

  // void function result in + — "invalid operand type for + or -"
  // arithmetic_type_to(VOID, INT) returns false
  assert(exits_with_error("void g() {} void f() { int x; x = g() + 1; }"));
  printf(GREEN "Test 8 (void fn result + int → error) passed\n" RESET);

  printf(GREEN "test_error_arithmetic_types passed!\n" RESET);
}

// ============================================================
// TEST 11: ERRORS — unary op on non-scalar
// ============================================================

static void test_error_unary_types(void) {
  printf("Running test_error_unary_types...\n");

  // Unary minus on array — "unary - or ! must have a scalar operand"
  assert(exits_with_error("void f() { int arr[3]; int x; x = -arr; }"));
  printf(GREEN "Test 1 (unary minus array → error) passed\n" RESET);

  // Unary ! on array
  assert(exits_with_error("void f() { int arr[3]; int x; x = !arr; }"));
  printf(GREEN "Test 2 (unary ! array → error) passed\n" RESET);

  // Unary minus on struct
  assert(exits_with_error(
    "struct S { int v; }; void f() { struct S s; int x; x = -s; }"));
  printf(GREEN "Test 3 (unary minus struct → error) passed\n" RESET);

  // Unary ! on struct
  assert(exits_with_error(
    "struct S { int v; }; void f() { struct S s; int x; x = !s; }"));
  printf(GREEN "Test 4 (unary ! struct → error) passed\n" RESET);

  printf(GREEN "test_error_unary_types passed!\n" RESET);
}

// ============================================================
// TEST 12: ERRORS — array index type
// ============================================================

static void test_error_array_index_type(void) {
  printf("Running test_error_array_index_type...\n");

  // Double as array index — "the index is not convertible to int"
  assert(exits_with_error("void f() { int arr[5]; double d; int x; x = arr[d]; }"));
  printf(GREEN "Test 1 (array[double] → error) passed\n" RESET);

  // Array as index
  assert(exits_with_error("void f() { int arr[5]; int idx[3]; int x; x = arr[idx]; }"));
  printf(GREEN "Test 2 (array[int_array] → error) passed\n" RESET);

  // Struct as array index
  assert(exits_with_error(
    "struct S { int v; }; void f() { int arr[5]; struct S s; int x; x = arr[s]; }"));
  printf(GREEN "Test 3 (array[struct] → error) passed\n" RESET);

  printf(GREEN "test_error_array_index_type passed!\n" RESET);
}

// ============================================================
// TEST 13: ERRORS — function call argument types
// ============================================================

static void test_error_call_arg_types(void) {
  printf("Running test_error_call_arg_types...\n");

  // Passing array where scalar expected — convert_to(INT_ARRAY, INT) = false
  // "in call, cannot convert the argument type to the parameter type"
  assert(exits_with_error(
    "void g(int x) {} void f() { int arr[3]; g(arr); }"));
  printf(GREEN "Test 1 (pass array to scalar param → error) passed\n" RESET);

  // Passing struct-B where struct-A expected — convert_to(B, A) = false (different symbols)
  assert(exits_with_error(
    "struct A { int v; }; struct B { int v; }; "
    "void g(struct A a) {} void f() { struct B b; g(b); }"));
  printf(GREEN "Test 2 (pass struct B to struct A param → error) passed\n" RESET);

  printf(GREEN "test_error_call_arg_types passed!\n" RESET);
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
  test_can_be_scalar();
  test_convert_to();
  test_arithmetic_type_to();
  test_integration_valid_type_conversions();
  test_integration_valid_arithmetic();
  test_integration_valid_conditions();
  test_error_nonscalar_conditions();
  test_error_assignment_types();
  test_error_return_types();
  test_error_arithmetic_types();
  test_error_unary_types();
  test_error_array_index_type();
  test_error_call_arg_types();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
