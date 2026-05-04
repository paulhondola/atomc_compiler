#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../include/frontend/lexer.h"
#include "../../include/utils/utils.h"

#define EPS 1e-9

static TokenStream tokenize_src(const char *src) {
  TokenStream stream;
  token_stream_init(&stream);
  tokenize(&stream, src);
  return stream;
}

// ---------------------------------------------------------------------------
// Keywords
// ---------------------------------------------------------------------------

static void test_lexer_keywords(void) {
  printf("Running test_lexer_keywords...\n");
  TokenStream stream = tokenize_src("int double char void struct if else while return");

  Token *t = stream.tokens.head;

  assert(t->type == TYPE_INT);
  t = t->next;
  assert(t->type == TYPE_DOUBLE);
  t = t->next;
  assert(t->type == TYPE_CHAR);
  t = t->next;
  assert(t->type == VOID);
  t = t->next;
  assert(t->type == STRUCT);
  t = t->next;
  assert(t->type == IF);
  t = t->next;
  assert(t->type == ELSE);
  t = t->next;
  assert(t->type == WHILE);
  t = t->next;
  assert(t->type == RETURN);
  t = t->next;
  assert(t->type == END);

  printf(GREEN "test_lexer_keywords passed!\n" RESET);

  token_stream_free(&stream);
}

// ---------------------------------------------------------------------------
// Identifiers
// ---------------------------------------------------------------------------

static void test_lexer_ids(void) {
  printf("Running test_lexer_ids...\n");
  TokenStream stream = tokenize_src("main my_var_123");

  Token *t = stream.tokens.head;

  assert(t->type == ID);
  assert(strcmp(t->text, "main") == 0);
  t = t->next;
  assert(t->type == ID);
  assert(strcmp(t->text, "my_var_123") == 0);
  t = t->next;
  assert(t->type == END);

  printf(GREEN "test_lexer_ids passed!\n" RESET);

  token_stream_free(&stream);
}

// ---------------------------------------------------------------------------
// Numerals
// ---------------------------------------------------------------------------

static void test_lexer_numerals(void) {
  printf("Running test_lexer_numerals...\n");
  TokenStream stream = tokenize_src("123 45.67 1e-2 1E+2");

  Token *t = stream.tokens.head;

  assert(t->type == INT);
  assert(t->integer_value == 123);

  t = t->next;

  assert(t->type == DOUBLE);
  assert(fabs(t->double_value - 45.67) < EPS);

  t = t->next;

  assert(t->type == DOUBLE);
  assert(fabs(t->double_value - 0.01) < EPS);

  t = t->next;

  assert(t->type == DOUBLE);
  assert(fabs(t->double_value - 100) < EPS);

  t = t->next;
  assert(t->type == END);

  printf(GREEN "test_lexer_numerals passed!\n" RESET);

  token_stream_free(&stream);
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------

static void test_lexer_strings(void) {
  printf("Running test_lexer_strings...\n");
  TokenStream stream = tokenize_src("\"hello world\" \"quote \\\" \"");

  Token *t = stream.tokens.head;

  assert(t->type == STRING);
  assert(strcmp(t->text, "hello world") == 0);
  t = t->next;
  assert(t->type == STRING);
  assert(strcmp(t->text, "quote \" ") == 0);
  t = t->next;
  assert(t->type == END);

  printf(GREEN "test_lexer_strings passed!\n" RESET);

  token_stream_free(&stream);
}

// ---------------------------------------------------------------------------
// Characters
// ---------------------------------------------------------------------------

static void test_lexer_chars(void) {
  printf("Running test_lexer_chars...\n");
  TokenStream stream = tokenize_src("'a' '\\'' '\\\\' '\\n' '\\b' '\\t' '\\0'");

  Token *t = stream.tokens.head;

  assert(t->type == CHAR);
  assert(t->character_value == 'a');
  t = t->next;
  assert(t->type == CHAR);
  assert(t->character_value == '\'');
  t = t->next;
  assert(t->type == CHAR);
  assert(t->character_value == '\\');
  t = t->next;
  assert(t->type == CHAR);
  assert(t->character_value == '\n');
  t = t->next;
  assert(t->type == CHAR);
  assert(t->character_value == '\b');
  t = t->next;
  assert(t->type == CHAR);
  assert(t->character_value == '\t');
  t = t->next;
  assert(t->type == CHAR);
  assert(t->character_value == '\0');
  t = t->next;
  assert(t->type == END);

  printf(GREEN "test_lexer_chars passed!\n" RESET);

  token_stream_free(&stream);
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

static void test_lexer_operators(void) {
  printf("Running test_lexer_operators...\n");
  TokenStream stream = tokenize_src("+ - * / . && || ! != == = < <= > >=");

  Token *t = stream.tokens.head;

  assert(t->type == ADD);
  t = t->next;
  assert(t->type == SUB);
  t = t->next;
  assert(t->type == MUL);
  t = t->next;
  assert(t->type == DIV);
  t = t->next;
  assert(t->type == DOT);
  t = t->next;
  assert(t->type == AND);
  t = t->next;
  assert(t->type == OR);
  t = t->next;
  assert(t->type == NOT);
  t = t->next;
  assert(t->type == NOTEQ);
  t = t->next;
  assert(t->type == EQUAL);
  t = t->next;
  assert(t->type == ASSIGN);
  t = t->next;
  assert(t->type == LESS);
  t = t->next;
  assert(t->type == LESSEQ);
  t = t->next;
  assert(t->type == GREATER);
  t = t->next;
  assert(t->type == GREATEREQ);
  t = t->next;

  assert(t->type == END);
  printf(GREEN "test_lexer_operators passed!\n" RESET);

  token_stream_free(&stream);
}

// ---------------------------------------------------------------------------

int main(void) {
  test_lexer_keywords();
  test_lexer_ids();
  test_lexer_numerals();
  test_lexer_strings();
  test_lexer_chars();
  test_lexer_operators();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
