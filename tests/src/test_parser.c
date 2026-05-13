#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/frontend/lexer.h"
#include "../../include/frontend/parser.h"
#include "../../include/analyzer/domain_analyzer.h"

#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"

static void parse_src(const char *src) {
  TokenStream stream;
  DomainAnalyzer domain_analyzer;
  token_stream_init(&stream);
  domain_analyzer_init(&domain_analyzer);
  tokenize(&stream, src);
  push_domain(&domain_analyzer);
  parse(&stream, &domain_analyzer);
  drop_domain(&domain_analyzer);
  token_stream_free(&stream);
  domain_analyzer_free(&domain_analyzer);
}

// Runs parse_src in a child process; returns true when the child exits with
// non-zero status (i.e. err() / exit(EXIT_FAILURE) was triggered).
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

// ---------------------------------------------------------------------------
// Variable definitions
// ---------------------------------------------------------------------------

static void test_parser_var_def(void) {
  printf("Running test_parser_var_def...\n");

  parse_src("int x;");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("double y;");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("char c;");
  printf(GREEN "Test 3 passed\n" RESET);

  parse_src("int arr[10];");
  printf(GREEN "Test 4 passed\n" RESET);

  parse_src("struct Point {}; struct Point p;"); // struct-type variable at unit level
  printf(GREEN "Test 5 passed\n" RESET);

  parse_src("struct Point {}; struct Point pts[5];"); // struct-type array at unit level
  printf(GREEN "Test 6 passed\n" RESET);

  printf(GREEN "test_parser_var_def passed\n" RESET);
}

// ---------------------------------------------------------------------------
// Struct definitions
// ---------------------------------------------------------------------------

static void test_parser_struct_def(void) {
  printf("Running test_parser_struct_def...\n");

  parse_src("struct Empty {};");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("struct Point { int x; int y; };");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("struct Node { int val; int next[10]; };");
  printf(GREEN "Test 3 passed\n" RESET);

  parse_src("struct Mixed { int i; double d; char c; };");
  printf(GREEN "Test 4 passed\n" RESET);

  printf(GREEN "test_parser_struct_def passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Function definitions
// ---------------------------------------------------------------------------

static void test_parser_fn_def(void) {
  printf("Running test_parser_fn_def...\n");

  parse_src("void f() {}");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("int g() { return 0; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("double h(int a, double b) { return a; }");
  printf(GREEN "Test 3 passed\n" RESET);

  parse_src("void arr_param(char s[], int n[10]) {}");
  printf(GREEN "Test 4 passed\n" RESET);

  parse_src("int main() { int x; x = 1; return x; }");
  printf(GREEN "Test 5 passed\n" RESET);

  printf(GREEN "test_parser_fn_def passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------------

static void test_parser_stm_if(void) {
  printf("Running test_parser_stm_if...\n");

  parse_src("void f() { if (1) ; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { if (1) ; else ; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("void f() { int x; if (x) { x = 1; } else { x = 0; } }");
  printf(GREEN "Test 3 passed\n" RESET);

  // dangling else — binds to nearest if
  parse_src("void f() { int x; if (x) if (x) ; else ; }");
  printf(GREEN "Test 4 passed\n" RESET);

  // nested if-else
  parse_src("void f() { int a; int b;"
            "  if (a) { if (b) ; else ; } else ; }");
  printf(GREEN "Test 5 passed\n" RESET);

  printf(GREEN "test_parser_stm_if passed!\n" RESET);
}

static void test_parser_stm_while(void) {
  printf("Running test_parser_stm_while...\n");

  parse_src("void f() { while (1) ; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int i; while (i != 0) { i = i - 1; } }");
  printf(GREEN "Test 2 passed\n" RESET);

  // nested while
  parse_src("void f() { int i; int j;"
            "  while (i) { while (j) ; i = i - 1; } }");
  printf(GREEN "Test 3 passed\n" RESET);

  printf(GREEN "test_parser_stm_while passed!\n" RESET);
}

static void test_parser_stm_return(void) {
  printf("Running test_parser_stm_return...\n");

  parse_src("void f() { return; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("int g() { return 42; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("double h() { return 3.14; }");
  printf(GREEN "Test 3 passed\n" RESET);

  parse_src("char k() { return 'a'; }");
  printf(GREEN "Test 4 passed\n" RESET);

  printf(GREEN "test_parser_stm_return passed!\n" RESET);
}

static void test_parser_stm_compound(void) {
  printf("Running test_parser_stm_compound...\n");

  // empty compound
  parse_src("void f() { {} }");
  printf(GREEN "Test 1 passed\n" RESET);

  // vars before stmts
  parse_src("void f() { int x; int y; x = 1; y = 2; }");
  printf(GREEN "Test 2 passed\n" RESET);

  // nested compound
  parse_src("void f() { int x; { int y; y = 0; } x = 1; }");
  printf(GREEN "Test 3 passed\n" RESET);

  printf(GREEN "test_parser_stm_compound passed!\n" RESET);
}

static void test_parser_stm_empty(void) {
  printf("Running test_parser_stm_empty...\n");

  parse_src("void f() { ; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { ; ; ; }");
  printf(GREEN "Test 2 passed\n" RESET);

  printf(GREEN "test_parser_stm_empty passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------------

static void test_parser_expr_arithmetic(void) {
  printf("Running test_parser_expr_arithmetic...\n");

  parse_src("void f() { int x; x = 1 + 2; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int x; x = 1 + 2 * 3; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("void f() { int x; x = (1 + 2) * 3; }");
  printf(GREEN "Test 3 passed\n" RESET);

  parse_src("void f() { int x; x = 10 / 2 - 1; }");
  printf(GREEN "Test 4 passed\n" RESET);

  parse_src("void f() { int x; x = -x; }");
  printf(GREEN "Test 5 passed\n" RESET);

  parse_src("void f() { double d; d = (double)1 / 2; }");
  printf(GREEN "Test 6 passed\n" RESET);

  printf(GREEN "test_parser_expr_arithmetic passed!\n" RESET);
}

static void test_parser_expr_logical(void) {
  printf("Running test_parser_expr_logical...\n");

  parse_src("void f() { int x; if (x && 1) ; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int x; if (x || 0) ; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("void f() { int x; if (!x) ; }");
  printf(GREEN "Test 3 passed\n" RESET);

  parse_src("void f() { int a; int b; if (a && b || !a) ; }");
  printf(GREEN "Test 4 passed\n" RESET);

  printf(GREEN "test_parser_expr_logical passed!\n" RESET);
}

static void test_parser_expr_relational(void) {
  printf("Running test_parser_expr_relational...\n");

  parse_src("void f() { int x; if (x == 0) ; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int x; if (x != 1) ; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("void f() { int x; if (x < 10) ; }");
  printf(GREEN "Test 3 passed\n" RESET);

  parse_src("void f() { int x; if (x <= 10) ; }");
  printf(GREEN "Test 4 passed\n" RESET);

  parse_src("void f() { int x; if (x > 0) ; }");
  printf(GREEN "Test 5 passed\n" RESET);

  parse_src("void f() { int x; if (x >= 0) ; }");
  printf(GREEN "Test 6 passed\n" RESET);

  printf(GREEN "test_parser_expr_relational passed!\n" RESET);
}

static void test_parser_expr_assignment(void) {
  printf("Running test_parser_expr_assignment...\n");

  parse_src("void f() { int x; x = 5; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int x; int y; x = y = 5; }");
  printf(GREEN "Test 2 passed\n" RESET);

  printf(GREEN "test_parser_expr_assignment passed!\n" RESET);
}

static void test_parser_expr_cast(void) {
  printf("Running test_parser_expr_cast...\n");

  parse_src("void f() { double d; d = (double)1; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int x; x = (int)3.14; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("void f() { int arr[10]; int x; x = (int)arr[0]; }");
  printf(GREEN "Test 3 passed\n" RESET);

  printf(GREEN "test_parser_expr_cast passed!\n" RESET);
}

static void test_parser_expr_postfix(void) {
  printf("Running test_parser_expr_postfix...\n");

  // array indexing
  parse_src("void f() { int arr[10]; arr[0] = 1; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int arr[10]; int i; arr[i] = arr[i - 1]; }");
  printf(GREEN "Test 2 passed\n" RESET);

  // member access
  parse_src("struct Point { int x; int y; };"
            "void f() { struct Point p; p.x = 1; p.y = 2; }");
  printf(GREEN "Test 3 passed\n" RESET);

  // function call — no args
  parse_src("int g() { return 0; } void f() { g(); }");
  printf(GREEN "Test 4 passed\n" RESET);

  // function call — with args
  parse_src("int add(int a, int b) { return a; }"
            "void f() { add(1, 2 + 3); }");
  printf(GREEN "Test 5 passed\n" RESET);

  printf(GREEN "test_parser_expr_postfix passed!\n" RESET);
}

static void test_parser_expr_unary(void) {
  printf("Running test_parser_expr_unary...\n");

  parse_src("void f() { int x; x = -1; }");
  printf(GREEN "Test 1 passed\n" RESET);

  parse_src("void f() { int x; if (!x) ; }");
  printf(GREEN "Test 2 passed\n" RESET);

  parse_src("void f() { int x; x = --x; }"); // double unary minus
  printf(GREEN "Test 3 passed\n" RESET);

  printf(GREEN "test_parser_expr_unary passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Full program
// ---------------------------------------------------------------------------

static void test_parser_full_program(void) {
  printf("Running test_parser_full_program...\n");

  parse_src("struct Pt { int x; int y; };"
            "struct Pt points[10];"
            "double max(double a, double b) {"
            "  if (a > b) return a;"
            "  else return b;"
            "}"
            "int len(char s[]) {"
            "  int i;"
            "  i = 0;"
            "  while (s[i]) i = i + 1;"
            "  return i;"
            "}"
            "int main() {"
            "  int i;"
            "  i = 10;"
            "  while (i != 0) {"
            "    i = i / 2;"
            "  }"
            "}");
  printf(GREEN "Test 1 passed\n" RESET);

  printf(GREEN "test_parser_full_program passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Error: syntax structure errors
// ---------------------------------------------------------------------------

static void test_parser_error_syntax(void) {
  printf("Running test_parser_error_syntax...\n");

  // Missing semicolon after variable definition
  assert(exits_with_error("int x"));
  printf(GREEN "Test 1 (missing ; after var def → error) passed\n" RESET);

  // Missing closing brace in function body
  assert(exits_with_error("void f() { int x;"));
  printf(GREEN "Test 2 (unclosed function body → error) passed\n" RESET);

  // Missing ( after if
  assert(exits_with_error("void f() { if 1 ; }"));
  printf(GREEN "Test 3 (missing ( after if → error) passed\n" RESET);

  // Missing ) after if condition
  assert(exits_with_error("void f() { if (1 ; }"));
  printf(GREEN "Test 4 (missing ) after if condition → error) passed\n" RESET);

  // Missing ( after while
  assert(exits_with_error("void f() { while 1 ; }"));
  printf(GREEN "Test 5 (missing ( after while → error) passed\n" RESET);

  // Missing ) after while condition
  assert(exits_with_error("void f() { while (1 ; }"));
  printf(GREEN "Test 6 (missing ) after while condition → error) passed\n" RESET);

  // Prototype is valid; redefinition after a full definition must still error
  assert(!exits_with_error("void f();"));
  printf(GREEN "Test 7 (prototype without body → valid) passed\n" RESET);

  assert(exits_with_error("void f() {} void f() {}"));
  printf(GREEN "Test 7b (duplicate full definition → error) passed\n" RESET);

  // Missing ; after struct definition
  assert(exits_with_error("struct S {}"));
  printf(GREEN "Test 8 (missing ; after struct def → error) passed\n" RESET);

  // Missing ] in array declaration
  assert(exits_with_error("int arr[10;"));
  printf(GREEN "Test 9 (missing ] in array declaration → error) passed\n" RESET);

  printf(GREEN "test_parser_error_syntax passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Error: return type mismatches
// ---------------------------------------------------------------------------

static void test_parser_error_return(void) {
  printf("Running test_parser_error_return...\n");

  // void function returning a value
  assert(exits_with_error("void f() { return 1; }"));
  printf(GREEN "Test 1 (void returning value → error) passed\n" RESET);

  // non-void function returning nothing
  assert(exits_with_error("int f() { return; }"));
  printf(GREEN "Test 2 (non-void returning nothing → error) passed\n" RESET);

  // double function returning char — type conversion mismatch
  // char → double is allowed (widening), so this MUST succeed:
  parse_src("double f() { return 'a'; }");
  printf(GREEN "Test 3 (char → double return: valid) passed\n" RESET);

  printf(GREEN "test_parser_error_return passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Error: expression / type errors
// ---------------------------------------------------------------------------

static void test_parser_error_exprs(void) {
  printf("Running test_parser_error_exprs...\n");

  // Indexing a non-array scalar
  assert(exits_with_error("void f() { int x; x[0] = 1; }"));
  printf(GREEN "Test 1 (indexing scalar → error) passed\n" RESET);

  // Accessing a field from a non-struct value
  assert(exits_with_error("void f() { int x; x.y = 1; }"));
  printf(GREEN "Test 2 (field from non-struct → error) passed\n" RESET);

  // Accessing an undefined struct field
  assert(exits_with_error("struct S { int v; }; void f() { struct S s; s.z = 1; }"));
  printf(GREEN "Test 3 (undefined struct field → error) passed\n" RESET);

  // Cast to struct type is not allowed
  assert(exits_with_error("struct S {}; void f() { int x; x = (struct S)x; }"));
  printf(GREEN "Test 4 (cast to struct → error) passed\n" RESET);

  // Cast from struct is not allowed
  assert(exits_with_error("struct S {}; void f() { struct S s; int x; x = (int)s; }"));
  printf(GREEN "Test 5 (cast from struct → error) passed\n" RESET);

  // Assigning to a constant (right-value on left side)
  assert(exits_with_error("void f() { 1 = 2; }"));
  printf(GREEN "Test 6 (assign to constant → error) passed\n" RESET);

  printf(GREEN "test_parser_error_exprs passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Error: function call errors
// ---------------------------------------------------------------------------

static void test_parser_error_calls(void) {
  printf("Running test_parser_error_calls...\n");

  // Calling an undefined identifier
  assert(exits_with_error("void f() { g(); }"));
  printf(GREEN "Test 1 (call to undefined id → error) passed\n" RESET);

  // Calling a non-function variable
  assert(exits_with_error("void f() { int x; x(); }"));
  printf(GREEN "Test 2 (calling non-function → error) passed\n" RESET);

  // Too many arguments
  assert(exits_with_error("void g() {} void f() { g(1); }"));
  printf(GREEN "Test 3 (too many args → error) passed\n" RESET);

  // Too few arguments
  assert(exits_with_error("void g(int x) {} void f() { g(); }"));
  printf(GREEN "Test 4 (too few args → error) passed\n" RESET);

  // Referencing a function without calling it
  assert(exits_with_error("int g() { return 0; } void f() { int x; x = g; }"));
  printf(GREEN "Test 5 (function ref without call → error) passed\n" RESET);

  printf(GREEN "test_parser_error_calls passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Error: semantic / redefinition errors
// ---------------------------------------------------------------------------

static void test_parser_error_semantic(void) {
  printf("Running test_parser_error_semantic...\n");

  // Global variable redefinition
  assert(exits_with_error("int x; int x;"));
  printf(GREEN "Test 1 (global var redefinition → error) passed\n" RESET);

  // Struct redefinition
  assert(exits_with_error("struct S {}; struct S {};"));
  printf(GREEN "Test 2 (struct redefinition → error) passed\n" RESET);

  // Function redefinition
  assert(exits_with_error("void f() {} void f() {}"));
  printf(GREEN "Test 3 (function redefinition → error) passed\n" RESET);

  // Duplicate parameter names
  assert(exits_with_error("void f(int x, int x) {}"));
  printf(GREEN "Test 4 (duplicate parameter names → error) passed\n" RESET);

  // Parameter and local variable sharing name (same domain)
  assert(exits_with_error("void f(int x) { int x; }"));
  printf(GREEN "Test 5 (param/local name clash → error) passed\n" RESET);

  // Duplicate local variables
  assert(exits_with_error("void f() { int x; int x; }"));
  printf(GREEN "Test 6 (local var redefinition → error) passed\n" RESET);

  // Struct member redefinition
  assert(exits_with_error("struct S { int x; int x; };"));
  printf(GREEN "Test 7 (struct member redefinition → error) passed\n" RESET);

  // Using an undefined struct type
  assert(exits_with_error("struct Unknown v;"));
  printf(GREEN "Test 8 (undefined struct type → error) passed\n" RESET);

  // Array variable without a dimension is not allowed
  assert(exits_with_error("int v[];"));
  printf(GREEN "Test 9 (array var without dimension → error) passed\n" RESET);

  printf(GREEN "test_parser_error_semantic passed!\n" RESET);
}

// ---------------------------------------------------------------------------

int main(void) {
  test_parser_var_def();
  test_parser_struct_def();
  test_parser_fn_def();
  test_parser_stm_if();
  test_parser_stm_while();
  test_parser_stm_return();
  test_parser_stm_compound();
  test_parser_stm_empty();
  test_parser_expr_arithmetic();
  test_parser_expr_logical();
  test_parser_expr_relational();
  test_parser_expr_assignment();
  test_parser_expr_cast();
  test_parser_expr_postfix();
  test_parser_expr_unary();
  test_parser_full_program();
  test_parser_error_syntax();
  test_parser_error_return();
  test_parser_error_exprs();
  test_parser_error_calls();
  test_parser_error_semantic();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
