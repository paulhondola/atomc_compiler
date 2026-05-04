#include <assert.h>
#include <stdio.h>

#include "../../include/frontend/lexer.h"
#include "../../include/frontend/parser.h"
#include "../../include/utils/utils.h"

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
// Full program (mirrors tests/parser/test_parser_code_example.c)
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
            "    puti(i);"
            "    i = i / 2;"
            "  }"
            "}");
  printf(GREEN "Test 1 passed\n" RESET);

  printf(GREEN "test_parser_full_program passed!\n" RESET);
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

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
