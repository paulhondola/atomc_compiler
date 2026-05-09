#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/utils/utils.h"
#include "../../include/analyzer/domain_analyzer.h"
#include "../../include/analyzer/domain.h"
#include "../../include/analyzer/symbol.h"
#include "../../include/analyzer/type.h"
#include "../../include/frontend/lexer.h"
#include "../../include/frontend/parser.h"

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

// Runs parse_src in a child process; returns true if the child exits with
// non-zero status (i.e., a semantic error was triggered via err()/exit).
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

// Stub used for add_extern_function tests — signature must match
// void (*)(struct VirtualMachine *).
static void dummy_extern_fn(struct VirtualMachine *vm) { (void)vm; }

// ============================================================
// TEST 1: LIFECYCLE — init / free
// ============================================================

static void test_init_lifecycle(void) {
  printf("Running test_init_lifecycle...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  assert(da.symbol_table == NULL);
  printf(GREEN "Test 1 (init sets symbol_table=NULL) passed\n" RESET);

  // free on empty analyzer must not crash
  domain_analyzer_free(&da);
  printf(GREEN "Test 2 (free empty analyzer is safe) passed\n" RESET);

  printf(GREEN "test_init_lifecycle passed!\n" RESET);
}

// ============================================================
// TEST 2: PUSH / DROP DOMAIN
// ============================================================

static void test_push_drop_domain(void) {
  printf("Running test_push_drop_domain...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);

  Domain *d1 = push_domain(&da);
  assert(da.symbol_table == d1);
  assert(d1 != NULL);
  assert(d1->parent == NULL);
  assert(d1->symbols == NULL);
  printf(GREEN "Test 1 (first push: parent=NULL, symbols=NULL) passed\n" RESET);

  Domain *d2 = push_domain(&da);
  assert(da.symbol_table == d2);
  assert(d2->parent == d1);
  printf(GREEN "Test 2 (second push: parent=d1) passed\n" RESET);

  drop_domain(&da);
  assert(da.symbol_table == d1);
  printf(GREEN "Test 3 (drop restores parent domain) passed\n" RESET);

  drop_domain(&da);
  assert(da.symbol_table == NULL);
  printf(GREEN "Test 4 (drop all → symbol_table=NULL) passed\n" RESET);

  domain_analyzer_free(&da);
  printf(GREEN "test_push_drop_domain passed!\n" RESET);
}

// ============================================================
// TEST 3: NEW SYMBOL
// ============================================================

static void test_new_symbol(void) {
  printf("Running test_new_symbol...\n");

  Symbol *s = new_symbol("x", SYMBOL_KIND_VARIABLE);
  assert(s != NULL);
  assert(strcmp(s->name, "x") == 0);
  assert(s->kind == SYMBOL_KIND_VARIABLE);
  printf(GREEN "Test 1 (name and kind set) passed\n" RESET);

  assert(s->owner == NULL);
  printf(GREEN "Test 2 (owner is NULL) passed\n" RESET);

  assert(s->next == NULL);
  printf(GREEN "Test 3 (next is NULL) passed\n" RESET);

  assert(s->type.symbol == NULL);
  assert(s->type.array_dimension == 0);
  printf(GREEN "Test 4 (type is zero-initialized) passed\n" RESET);

  free_symbol(s);
  printf(GREEN "test_new_symbol passed!\n" RESET);
}

// ============================================================
// TEST 4: DUPLICATE SYMBOL
// ============================================================

static void test_duplicate_symbol(void) {
  printf("Running test_duplicate_symbol...\n");

  Symbol *orig      = new_symbol("p", SYMBOL_KIND_PARAMETER);
  orig->type        = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  orig->param_index = 7;

  Symbol *sibling = new_symbol("q", SYMBOL_KIND_PARAMETER);
  orig->next      = sibling;

  Symbol *dup = duplicate_symbol(orig);
  assert(strcmp(dup->name, "p") == 0);
  assert(dup->kind == SYMBOL_KIND_PARAMETER);
  assert(dup->type.type_base == TYPE_BASE_DOUBLE);
  assert(dup->type.array_dimension == -1);
  assert(dup->param_index == 7);
  printf(GREEN "Test 1 (fields are copied) passed\n" RESET);

  assert(dup->next == NULL);
  printf(GREEN "Test 2 (next is NULL in duplicate) passed\n" RESET);

  free_symbol(dup);
  free_symbol(sibling);
  orig->next = NULL;
  free_symbol(orig);
  printf(GREEN "test_duplicate_symbol passed!\n" RESET);
}

// ============================================================
// TEST 5: SYMBOL LIST OPERATIONS
// ============================================================

static void test_symbol_list(void) {
  printf("Running test_symbol_list...\n");

  assert(symbols_len(NULL) == 0);
  printf(GREEN "Test 1 (symbols_len(NULL) = 0) passed\n" RESET);

  Symbol *head = NULL;
  Symbol *s1   = new_symbol("a", SYMBOL_KIND_VARIABLE);
  Symbol *s2   = new_symbol("b", SYMBOL_KIND_VARIABLE);
  Symbol *s3   = new_symbol("c", SYMBOL_KIND_VARIABLE);

  add_symbol_to_list(&head, s1);
  add_symbol_to_list(&head, s2);
  add_symbol_to_list(&head, s3);

  assert(symbols_len(head) == 3);
  printf(GREEN "Test 2 (symbols_len counts correctly) passed\n" RESET);

  assert(head == s1);
  assert(s1->next == s2);
  assert(s2->next == s3);
  assert(s3->next == NULL);
  printf(GREEN "Test 3 (insertion order preserved) passed\n" RESET);

  assert(find_symbol_in_list(head, "b") == s2);
  printf(GREEN "Test 4 (find_symbol_in_list finds by name) passed\n" RESET);

  assert(find_symbol_in_list(head, "z") == NULL);
  printf(GREEN "Test 5 (find_symbol_in_list returns NULL for missing) passed\n" RESET);

  free_symbols(head);
  printf(GREEN "test_symbol_list passed!\n" RESET);
}

// ============================================================
// TEST 6: ADD / FIND SYMBOL IN DOMAIN
// ============================================================

static void test_domain_symbol_ops(void) {
  printf("Running test_domain_symbol_ops...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);

  Symbol *s = new_symbol("var1", SYMBOL_KIND_VARIABLE);
  s->type   = (Type){TYPE_BASE_INT, NULL, -1};
  add_symbol_to_domain(da.symbol_table, s);

  assert(find_symbol_in_domain(da.symbol_table, "var1") == s);
  printf(GREEN "Test 1 (added symbol is findable in its domain) passed\n" RESET);

  assert(find_symbol_in_domain(da.symbol_table, "missing") == NULL);
  printf(GREEN "Test 2 (find_symbol_in_domain returns NULL for missing) passed\n" RESET);

  push_domain(&da);
  assert(find_symbol_in_domain(da.symbol_table, "var1") == NULL);
  printf(GREEN "Test 3 (find_symbol_in_domain ignores parent) passed\n" RESET);

  drop_domain(&da);
  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_domain_symbol_ops passed!\n" RESET);
}

// ============================================================
// TEST 7: CROSS-SCOPE SYMBOL SEARCH (find_symbol)
// ============================================================

static void test_cross_scope_search(void) {
  printf("Running test_cross_scope_search...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);

  Symbol *global_x = new_symbol("x", SYMBOL_KIND_VARIABLE);
  global_x->type   = (Type){TYPE_BASE_INT, NULL, -1};
  add_symbol_to_domain(da.symbol_table, global_x);

  assert(find_symbol(&da, "x") == global_x);
  printf(GREEN "Test 1 (finds in current domain) passed\n" RESET);

  push_domain(&da);

  assert(find_symbol(&da, "x") == global_x);
  printf(GREEN "Test 2 (finds in parent domain) passed\n" RESET);

  Symbol *local_x = new_symbol("x", SYMBOL_KIND_VARIABLE);
  local_x->type   = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  add_symbol_to_domain(da.symbol_table, local_x);

  Symbol *found = find_symbol(&da, "x");
  assert(found == local_x);
  assert(found->type.type_base == TYPE_BASE_DOUBLE);
  printf(GREEN "Test 3 (inner domain shadows parent) passed\n" RESET);

  assert(find_symbol(&da, "undefined_symbol") == NULL);
  printf(GREEN "Test 4 (returns NULL when not found anywhere) passed\n" RESET);

  drop_domain(&da);
  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_cross_scope_search passed!\n" RESET);
}

// ============================================================
// TEST 8: TYPE SIZE CALCULATIONS
// ============================================================

static void test_type_size(void) {
  printf("Running test_type_size...\n");

  Type t;

  t = (Type){TYPE_BASE_INT, NULL, -1};
  assert(type_size(&t) == (int)sizeof(int));
  printf(GREEN "Test 1 (INT size = sizeof(int)) passed\n" RESET);

  t = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  assert(type_size(&t) == (int)sizeof(double));
  printf(GREEN "Test 2 (DOUBLE size = sizeof(double)) passed\n" RESET);

  t = (Type){TYPE_BASE_CHAR, NULL, -1};
  assert(type_size(&t) == 1);
  printf(GREEN "Test 3 (CHAR size = 1) passed\n" RESET);

  t = (Type){TYPE_BASE_VOID, NULL, -1};
  assert(type_size(&t) == 0);
  printf(GREEN "Test 4 (VOID size = 0) passed\n" RESET);

  t = (Type){TYPE_BASE_INT, NULL, 5};
  assert(type_size(&t) == 5 * (int)sizeof(int));
  printf(GREEN "Test 5 (INT[5] size = 5*sizeof(int)) passed\n" RESET);

  t = (Type){TYPE_BASE_INT, NULL, 0};
  assert(type_size(&t) == (int)sizeof(void *));
  printf(GREEN "Test 6 (INT[] size = sizeof(void*)) passed\n" RESET);

  // struct { int i; double d; }
  Symbol *s         = new_symbol("S", SYMBOL_KIND_STRUCT);
  s->type           = (Type){TYPE_BASE_STRUCT, s, -1};
  s->struct_members = NULL;

  Symbol *m_int    = new_symbol("i", SYMBOL_KIND_VARIABLE);
  m_int->type      = (Type){TYPE_BASE_INT, NULL, -1};
  m_int->owner     = s;
  m_int->var_index = type_size(&s->type);
  add_symbol_to_list(&s->struct_members, duplicate_symbol(m_int));

  Symbol *m_double    = new_symbol("d", SYMBOL_KIND_VARIABLE);
  m_double->type      = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  m_double->owner     = s;
  m_double->var_index = type_size(&s->type);
  add_symbol_to_list(&s->struct_members, duplicate_symbol(m_double));

  assert(type_size(&s->type) == (int)(sizeof(int) + sizeof(double)));
  printf(GREEN "Test 7 (STRUCT size = sum of member sizes) passed\n" RESET);

  free_symbol(m_int);
  free_symbol(m_double);
  free_symbol(s);
  printf(GREEN "test_type_size passed!\n" RESET);
}

// ============================================================
// TEST 9: STRUCT MEMBER BYTE OFFSETS
// ============================================================

static void test_struct_member_offsets(void) {
  printf("Running test_struct_member_offsets...\n");

  // Mirrors what the parser does for: struct S { int i; double d; char c; }
  // var_index = type_size(&owner->type) at moment of insertion = byte offset

  Symbol *s         = new_symbol("S", SYMBOL_KIND_STRUCT);
  s->type           = (Type){TYPE_BASE_STRUCT, s, -1};
  s->struct_members = NULL;

  Symbol *m1    = new_symbol("i", SYMBOL_KIND_VARIABLE);
  m1->type      = (Type){TYPE_BASE_INT, NULL, -1};
  m1->owner     = s;
  m1->var_index = type_size(&s->type);   // 0
  add_symbol_to_list(&s->struct_members, duplicate_symbol(m1));
  assert(m1->var_index == 0);

  Symbol *m2    = new_symbol("d", SYMBOL_KIND_VARIABLE);
  m2->type      = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  m2->owner     = s;
  m2->var_index = type_size(&s->type);   // sizeof(int)
  add_symbol_to_list(&s->struct_members, duplicate_symbol(m2));
  assert(m2->var_index == (int)sizeof(int));

  Symbol *m3    = new_symbol("c", SYMBOL_KIND_VARIABLE);
  m3->type      = (Type){TYPE_BASE_CHAR, NULL, -1};
  m3->owner     = s;
  m3->var_index = type_size(&s->type);   // sizeof(int) + sizeof(double)
  add_symbol_to_list(&s->struct_members, duplicate_symbol(m3));
  assert(m3->var_index == (int)(sizeof(int) + sizeof(double)));

  printf(GREEN "Test 1 (byte offsets: 0, sizeof(int), sizeof(int)+sizeof(double)) passed\n" RESET);

  assert(type_size(&s->type) == (int)(sizeof(int) + sizeof(double) + sizeof(char)));
  printf(GREEN "Test 2 (total struct size correct) passed\n" RESET);

  free_symbol(m1);
  free_symbol(m2);
  free_symbol(m3);
  free_symbol(s);
  printf(GREEN "test_struct_member_offsets passed!\n" RESET);
}

// ============================================================
// TEST 10: FUNCTION PARAMETER INDEXING
// ============================================================

static void test_function_parameter_indexing(void) {
  printf("Running test_function_parameter_indexing...\n");

  Symbol *fn                             = new_symbol("add", SYMBOL_KIND_FUNCTION);
  fn->type                               = (Type){TYPE_BASE_INT, NULL, -1};
  fn->function.parameters                = NULL;
  fn->function.locals                    = NULL;
  fn->function.external_function_pointer = NULL;

  Type int_t    = {TYPE_BASE_INT,    NULL, -1};
  Type double_t = {TYPE_BASE_DOUBLE, NULL, -1};

  Symbol *p0 = add_function_parameter(fn, "a", int_t);
  assert(p0->param_index == 0);
  printf(GREEN "Test 1 (first param index = 0) passed\n" RESET);

  Symbol *p1 = add_function_parameter(fn, "b", double_t);
  assert(p1->param_index == 1);
  printf(GREEN "Test 2 (second param index = 1) passed\n" RESET);

  Symbol *p2 = add_function_parameter(fn, "c", int_t);
  assert(p2->param_index == 2);
  printf(GREEN "Test 3 (third param index = 2) passed\n" RESET);

  assert(symbols_len(fn->function.parameters) == 3);
  printf(GREEN "Test 4 (function.parameters has 3 entries) passed\n" RESET);

  free_symbol(p0);
  free_symbol(p1);
  free_symbol(p2);
  free_symbol(fn);
  printf(GREEN "test_function_parameter_indexing passed!\n" RESET);
}

// ============================================================
// TEST 11: FUNCTION LOCAL VARIABLE INDEXING (slot index)
// ============================================================

static void test_function_local_indexing(void) {
  printf("Running test_function_local_indexing...\n");

  // Function locals use sequential slot index, NOT byte offset.
  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);

  Symbol *fn = new_symbol("f", SYMBOL_KIND_FUNCTION);
  fn->type   = (Type){TYPE_BASE_VOID, NULL, -1};
  fn->function.parameters                = NULL;
  fn->function.locals                    = NULL;
  fn->function.external_function_pointer = NULL;
  add_symbol_to_domain(da.symbol_table, fn);

  push_domain(&da);

  Symbol *x    = new_symbol("x", SYMBOL_KIND_VARIABLE);
  x->type      = (Type){TYPE_BASE_INT, NULL, -1};
  x->owner     = fn;
  x->var_index = symbols_len(fn->function.locals);   // 0
  add_symbol_to_domain(da.symbol_table, x);
  add_symbol_to_list(&fn->function.locals, duplicate_symbol(x));
  assert(x->var_index == 0);
  printf(GREEN "Test 1 (first local slot = 0) passed\n" RESET);

  Symbol *y    = new_symbol("y", SYMBOL_KIND_VARIABLE);
  y->type      = (Type){TYPE_BASE_DOUBLE, NULL, -1};
  y->owner     = fn;
  y->var_index = symbols_len(fn->function.locals);   // 1, not sizeof(int)
  add_symbol_to_domain(da.symbol_table, y);
  add_symbol_to_list(&fn->function.locals, duplicate_symbol(y));
  assert(y->var_index == 1);
  printf(GREEN "Test 2 (second local slot = 1, not sizeof(int)) passed\n" RESET);

  assert(symbols_len(fn->function.locals) == 2);
  printf(GREEN "Test 3 (function.locals has 2 entries) passed\n" RESET);

  drop_domain(&da);
  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_function_local_indexing passed!\n" RESET);
}

// ============================================================
// TEST 12: EXTERN FUNCTION REGISTRATION
// ============================================================

static void test_extern_function_registration(void) {
  printf("Running test_extern_function_registration...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);

  Type void_t = {TYPE_BASE_VOID, NULL, -1};
  Symbol *fn  = add_extern_function(&da, "my_extern", dummy_extern_fn, void_t);

  Symbol *found = find_symbol_in_domain(da.symbol_table, "my_extern");
  assert(found != NULL);
  assert(found == fn);
  printf(GREEN "Test 1 (extern symbol is in domain) passed\n" RESET);

  assert(fn->kind == SYMBOL_KIND_FUNCTION);
  printf(GREEN "Test 2 (kind is SYMBOL_KIND_FUNCTION) passed\n" RESET);

  assert(fn->function.external_function_pointer == dummy_extern_fn);
  printf(GREEN "Test 3 (extern function pointer is stored) passed\n" RESET);

  assert(fn->type.type_base == TYPE_BASE_VOID);
  printf(GREEN "Test 4 (return type is VOID) passed\n" RESET);

  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_extern_function_registration passed!\n" RESET);
}

// ============================================================
// TEST 13: INTEGRATION — valid programs via parse_src
// ============================================================

static void test_integration_global_variables(void) {
  printf("Running test_integration_global_variables...\n");

  parse_src("int x;");
  printf(GREEN "Test 1 (int x;) passed\n" RESET);

  parse_src("double y;");
  printf(GREEN "Test 2 (double y;) passed\n" RESET);

  parse_src("char c;");
  printf(GREEN "Test 3 (char c;) passed\n" RESET);

  parse_src("int arr[10];");
  printf(GREEN "Test 4 (int arr[10];) passed\n" RESET);

  parse_src("int x; double y; char z;");
  printf(GREEN "Test 5 (multiple global vars) passed\n" RESET);

  printf(GREEN "test_integration_global_variables passed!\n" RESET);
}

static void test_integration_struct_definition(void) {
  printf("Running test_integration_struct_definition...\n");

  parse_src("struct Empty {};");
  printf(GREEN "Test 1 (empty struct) passed\n" RESET);

  parse_src("struct Point { int x; int y; };");
  printf(GREEN "Test 2 (struct with int members) passed\n" RESET);

  parse_src("struct Mixed { int i; double d; char c; };");
  printf(GREEN "Test 3 (struct with mixed scalar types) passed\n" RESET);

  parse_src("struct S1 { int i; double d[2]; char x; };");
  printf(GREEN "Test 4 (struct with array member) passed\n" RESET);

  parse_src("struct Node { int val; }; struct Node n;");
  printf(GREEN "Test 5 (struct-type variable) passed\n" RESET);

  parse_src("struct Node { int val; }; struct Node arr[5];");
  printf(GREEN "Test 6 (struct-type array) passed\n" RESET);

  printf(GREEN "test_integration_struct_definition passed!\n" RESET);
}

static void test_integration_function_definition(void) {
  printf("Running test_integration_function_definition...\n");

  parse_src("void f() {}");
  printf(GREEN "Test 1 (void no-param function) passed\n" RESET);

  parse_src("int g() { return 0; }");
  printf(GREEN "Test 2 (int return function) passed\n" RESET);

  parse_src("double h(int a, double b) { return a; }");
  printf(GREEN "Test 3 (function with params) passed\n" RESET);

  parse_src("int add(int a, int b) { int result; return result; }");
  printf(GREEN "Test 4 (function with params and local vars) passed\n" RESET);

  parse_src("void arr_param(char s[], int n[10]) {}");
  printf(GREEN "Test 5 (array params) passed\n" RESET);

  printf(GREEN "test_integration_function_definition passed!\n" RESET);
}

static void test_integration_scope_shadowing(void) {
  printf("Running test_integration_scope_shadowing...\n");

  parse_src("int x; void f(int x) {}");
  printf(GREEN "Test 1 (param shadows global) passed\n" RESET);

  // Inner { } is stmCompound[true] — creates a new domain
  parse_src("void f() { int x; { int x; } }");
  printf(GREEN "Test 2 (block-local shadows function-local) passed\n" RESET);

  parse_src("void f() { int x; while (x) { int x; } }");
  printf(GREEN "Test 3 (shadow in while body) passed\n" RESET);

  parse_src("void f() { int x; if (x) { int x; } }");
  printf(GREEN "Test 4 (shadow in if body) passed\n" RESET);

  printf(GREEN "test_integration_scope_shadowing passed!\n" RESET);
}

static void test_integration_nested_scopes(void) {
  printf("Running test_integration_nested_scopes...\n");

  // Function body shares domain with params (stmCompound called with false)
  parse_src("void f(int a) { int b; }");
  printf(GREEN "Test 1 (params and body share scope) passed\n" RESET);

  parse_src("void f() { int x; if (x) { int y; } }");
  printf(GREEN "Test 2 (if body creates new scope) passed\n" RESET);

  parse_src("void f() { int x; while (x) { int y; } }");
  printf(GREEN "Test 3 (while body creates new scope) passed\n" RESET);

  parse_src("void f() { int a; if (a) { int b; if (b) { int c; } } }");
  printf(GREEN "Test 4 (deeply nested if scopes) passed\n" RESET);

  parse_src("struct S { int v; }; void f() { struct S s; }");
  printf(GREEN "Test 5 (struct type used in function) passed\n" RESET);

  printf(GREEN "test_integration_nested_scopes passed!\n" RESET);
}

// ============================================================
// TEST 14: ERROR CONDITIONS — invalid programs → exit failure
// ============================================================

static void test_error_conditions(void) {
  printf("Running test_error_conditions...\n");

  assert(exits_with_error("int x; int x;"));
  printf(GREEN "Test 1 (global var redefinition → error) passed\n" RESET);

  assert(exits_with_error("struct S {}; struct S {};"));
  printf(GREEN "Test 2 (struct redefinition → error) passed\n" RESET);

  assert(exits_with_error("void f() {} void f() {}"));
  printf(GREEN "Test 3 (function redefinition → error) passed\n" RESET);

  assert(exits_with_error("void f(int x, int x) {}"));
  printf(GREEN "Test 4 (parameter redefinition → error) passed\n" RESET);

  // Params and body share the function scope, so same name → redefinition
  assert(exits_with_error("void f(int x) { int x; }"));
  printf(GREEN "Test 5 (param/local same name → error) passed\n" RESET);

  assert(exits_with_error("void f() { int x; int x; }"));
  printf(GREEN "Test 6 (local var redefinition → error) passed\n" RESET);

  assert(exits_with_error("struct S { int x; int x; };"));
  printf(GREEN "Test 7 (struct member redefinition → error) passed\n" RESET);

  assert(exits_with_error("struct Unknown v;"));
  printf(GREEN "Test 8 (undefined struct → error) passed\n" RESET);

  // Variable array without specified dimension is rejected (int v[] is for params only)
  assert(exits_with_error("int v[];"));
  printf(GREEN "Test 9 (array var without dimension → error) passed\n" RESET);

  printf(GREEN "test_error_conditions passed!\n" RESET);
}

// ============================================================
// MAIN
// ============================================================

int main(void) {
  test_init_lifecycle();
  test_push_drop_domain();
  test_new_symbol();
  test_duplicate_symbol();
  test_symbol_list();
  test_domain_symbol_ops();
  test_cross_scope_search();
  test_type_size();
  test_struct_member_offsets();
  test_function_parameter_indexing();
  test_function_local_indexing();
  test_extern_function_registration();
  test_integration_global_variables();
  test_integration_struct_definition();
  test_integration_function_definition();
  test_integration_scope_shadowing();
  test_integration_nested_scopes();
  test_error_conditions();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
