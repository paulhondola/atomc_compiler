#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/frontend/lexer.h"
#include "../../include/utils/utils.h"

#define EPS 1e-9

static TokenStream tokenize_src(const char *src) {
  TokenStream stream;
  token_stream_init(&stream);
  tokenize(&stream, src);
  return stream;
}

// Runs tokenize in a child process; returns true when the child exits with
// non-zero status (i.e. err() / exit(EXIT_FAILURE) was triggered).
static bool exits_with_lexer_error(const char *src) {
  pid_t pid = fork();
  if (pid == 0) {
    freopen("/dev/null", "w", stderr);
    TokenStream stream;
    token_stream_init(&stream);
    tokenize(&stream, src);
    token_stream_free(&stream);
    exit(EXIT_SUCCESS);
  }
  int status;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS;
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
// Delimiters
// ---------------------------------------------------------------------------

static void test_lexer_delimiters(void) {
  printf("Running test_lexer_delimiters...\n");
  TokenStream stream = tokenize_src(", ; ( ) [ ] { }");

  Token *t = stream.tokens.head;

  assert(t->type == COMMA);
  printf(GREEN "Test 1 (COMMA) passed\n" RESET);
  t = t->next;

  assert(t->type == SEMICOLON);
  printf(GREEN "Test 2 (SEMICOLON) passed\n" RESET);
  t = t->next;

  assert(t->type == LPAR);
  printf(GREEN "Test 3 (LPAR) passed\n" RESET);
  t = t->next;

  assert(t->type == RPAR);
  printf(GREEN "Test 4 (RPAR) passed\n" RESET);
  t = t->next;

  assert(t->type == LBRACKET);
  printf(GREEN "Test 5 (LBRACKET) passed\n" RESET);
  t = t->next;

  assert(t->type == RBRACKET);
  printf(GREEN "Test 6 (RBRACKET) passed\n" RESET);
  t = t->next;

  assert(t->type == LACC);
  printf(GREEN "Test 7 (LACC) passed\n" RESET);
  t = t->next;

  assert(t->type == RACC);
  printf(GREEN "Test 8 (RACC) passed\n" RESET);
  t = t->next;

  assert(t->type == END);
  printf(GREEN "Test 9 (END follows RACC) passed\n" RESET);

  token_stream_free(&stream);
  printf(GREEN "test_lexer_delimiters passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Line number tracking
// ---------------------------------------------------------------------------

static void test_lexer_line_numbers(void) {
  printf("Running test_lexer_line_numbers...\n");

  // All tokens on a single line start at line 1
  {
    TokenStream stream = tokenize_src("int x");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    assert(t->line == 1);
    t = t->next;
    assert(t->type == ID);
    assert(t->line == 1);
    token_stream_free(&stream);
    printf(GREEN "Test 1 (single line tokens start at line 1) passed\n" RESET);
  }

  // Token after a newline is on line 2
  {
    TokenStream stream = tokenize_src("int\nx");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    assert(t->line == 1);
    t = t->next;
    assert(t->type == ID);
    assert(t->line == 2);
    token_stream_free(&stream);
    printf(GREEN "Test 2 (newline increments line counter) passed\n" RESET);
  }

  // Two blank lines → token on line 3
  {
    TokenStream stream = tokenize_src("int\n\nx");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    assert(t->line == 1);
    t = t->next;
    assert(t->type == ID);
    assert(t->line == 3);
    token_stream_free(&stream);
    printf(GREEN "Test 3 (two newlines → line 3) passed\n" RESET);
  }

  // Windows-style CRLF line endings
  {
    TokenStream stream = tokenize_src("int\r\nx");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    assert(t->line == 1);
    t = t->next;
    assert(t->type == ID);
    assert(t->line == 2);
    token_stream_free(&stream);
    printf(GREEN "Test 4 (CRLF line ending increments line counter once) passed\n" RESET);
  }

  // Block comment spanning lines increments the counter
  {
    TokenStream stream = tokenize_src("int /* line1\nline2 */ x");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    assert(t->line == 1);
    t = t->next;
    assert(t->type == ID);
    assert(t->line == 2);
    token_stream_free(&stream);
    printf(GREEN "Test 5 (block comment newline increments counter) passed\n" RESET);
  }

  printf(GREEN "test_lexer_line_numbers passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Comment handling
// ---------------------------------------------------------------------------

static void test_lexer_comments(void) {
  printf("Running test_lexer_comments...\n");

  // Single-line comment: text after // until end-of-line is ignored
  {
    TokenStream stream = tokenize_src("int // this is ignored\nx");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    t = t->next;
    assert(t->type == ID);
    assert(strcmp(t->text, "x") == 0);
    t = t->next;
    assert(t->type == END);
    token_stream_free(&stream);
    printf(GREEN "Test 1 (single-line comment skipped) passed\n" RESET);
  }

  // Single-line comment at end of input (no trailing newline)
  {
    TokenStream stream = tokenize_src("int // comment");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    t = t->next;
    assert(t->type == END);
    token_stream_free(&stream);
    printf(GREEN "Test 2 (single-line comment at EOF) passed\n" RESET);
  }

  // Block comment on one line
  {
    TokenStream stream = tokenize_src("int /* ignored */ x");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    t = t->next;
    assert(t->type == ID);
    assert(strcmp(t->text, "x") == 0);
    t = t->next;
    assert(t->type == END);
    token_stream_free(&stream);
    printf(GREEN "Test 3 (block comment on one line skipped) passed\n" RESET);
  }

  // Block comment spanning multiple lines
  {
    TokenStream stream = tokenize_src("int /* line1\nline2\nline3 */ x");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    t = t->next;
    assert(t->type == ID);
    assert(strcmp(t->text, "x") == 0);
    t = t->next;
    assert(t->type == END);
    token_stream_free(&stream);
    printf(GREEN "Test 4 (multi-line block comment skipped) passed\n" RESET);
  }

  // Multiple comments interleaved with tokens
  {
    TokenStream stream = tokenize_src("/* a */ int /* b */ x /* c */");
    Token *t = stream.tokens.head;
    assert(t->type == TYPE_INT);
    t = t->next;
    assert(t->type == ID);
    t = t->next;
    assert(t->type == END);
    token_stream_free(&stream);
    printf(GREEN "Test 5 (interleaved block comments) passed\n" RESET);
  }

  printf(GREEN "test_lexer_comments passed!\n" RESET);
}

// ---------------------------------------------------------------------------
// Error conditions (fork-based)
// ---------------------------------------------------------------------------

static void test_lexer_errors(void) {
  printf("Running test_lexer_errors...\n");

  // Empty char literal: ''
  assert(exits_with_lexer_error("''"));
  printf(GREEN "Test 1 (empty char literal '' → error) passed\n" RESET);

  // Unknown escape sequence in char: '\q'
  assert(exits_with_lexer_error("'\\q'"));
  printf(GREEN "Test 2 (unknown char escape '\\q' → error) passed\n" RESET);

  // Unterminated char literal: 'ab' — 'a' is read, then 'b' is not a closing quote
  assert(exits_with_lexer_error("'ab'"));
  printf(GREEN "Test 3 (unterminated char literal 'ab' → error) passed\n" RESET);

  // Unterminated string literal: no closing quote
  assert(exits_with_lexer_error("\"hello"));
  printf(GREEN "Test 4 (unterminated string literal → error) passed\n" RESET);

  // Unknown escape sequence in string: "\q"
  assert(exits_with_lexer_error("\"\\q\""));
  printf(GREEN "Test 5 (unknown string escape \"\\q\" → error) passed\n" RESET);

  // Missing digits after decimal point: 1. (nothing after the dot)
  assert(exits_with_lexer_error("1."));
  printf(GREEN "Test 6 (missing decimal digits '1.' → error) passed\n" RESET);

  // Missing exponent digits: 1e with no digits following
  assert(exits_with_lexer_error("1e"));
  printf(GREEN "Test 7 (missing exponent digits '1e' → error) passed\n" RESET);

  // Missing exponent digits after sign: 1e+
  assert(exits_with_lexer_error("1e+"));
  printf(GREEN "Test 8 (missing exponent digits '1e+' → error) passed\n" RESET);

  // Integer out of range
  assert(exits_with_lexer_error("99999999999999999999999999"));
  printf(GREEN "Test 9 (integer out of range → error) passed\n" RESET);

  // Double out of range
  assert(exits_with_lexer_error("1e99999"));
  printf(GREEN "Test 10 (double out of range → error) passed\n" RESET);

  // Single ampersand — not a valid token, suggest &&
  assert(exits_with_lexer_error("&"));
  printf(GREEN "Test 11 (single '&' → error) passed\n" RESET);

  // Single pipe — not a valid token, suggest ||
  assert(exits_with_lexer_error("|"));
  printf(GREEN "Test 12 (single '|' → error) passed\n" RESET);

  // Unknown character
  assert(exits_with_lexer_error("@"));
  printf(GREEN "Test 13 (unknown char '@' → error) passed\n" RESET);

  // Unterminated block comment
  assert(exits_with_lexer_error("/* hello"));
  printf(GREEN "Test 14 (unterminated block comment → error) passed\n" RESET);

  printf(GREEN "test_lexer_errors passed!\n" RESET);
}

// ---------------------------------------------------------------------------

int main(void) {
  test_lexer_keywords();
  test_lexer_ids();
  test_lexer_numerals();
  test_lexer_strings();
  test_lexer_chars();
  test_lexer_operators();
  test_lexer_delimiters();
  test_lexer_line_numbers();
  test_lexer_comments();
  test_lexer_errors();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
