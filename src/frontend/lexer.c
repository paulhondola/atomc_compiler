#include "../../include/frontend/lexer.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/frontend/stream.h"
#include "../../include/frontend/token.h"
#include "../../include/utils/utils.h"

// adds a token to the end of the tokens list and returns it
// sets its code and line
static Token *add_token(TokenStream *stream, TokenType type) {
  Token *token = safe_alloc(sizeof(Token));
  token->type  = type;
  token->line  = stream->line;
  token->next  = NULL;
  if (stream->tokens.tail) {
    stream->tokens.tail->next = token;
  } else {
    stream->tokens.head = token;
  }
  stream->tokens.tail = token;
  return token;
}

const char *consume_char_literal(TokenStream *stream, const char *source_code) {
  source_code++; // skip opening quote
  char current_character;
  if (*source_code == '\\') {
    source_code++;
    switch (*source_code) {
      // newline
      case 'n':
        current_character = '\n';
        break;
      // tab
      case 't':
        current_character = '\t';
        break;
      // carriage return
      case 'r':
        current_character = '\r';
        break;
      // backspace
      case 'b':
        current_character = '\b';
        break;
      // null
      case '0':
        current_character = '\0';
        break;
      // backslash
      case '\\':
        current_character = '\\';
        break;
      // single quote
      case '\'':
        current_character = '\'';
        break;
      default:
        err("line %d: unknown escape sequence: \\%c", stream->line, *source_code);
        current_character = *source_code;
    }
  } else if (*source_code && *source_code != '\'') {
    // normal character
    current_character = *source_code;
  } else {
    err("line %d: empty or malformed char literal", stream->line);
    current_character = 0;
  }
  source_code++;
  if (*source_code != '\'') {
    err("line %d: unterminated char literal (expected ' at the end)", stream->line);
  }
  source_code++;
  Token *token           = add_token(stream, CHAR);
  token->character_value = current_character;
  return source_code;
}

const char *consume_string_literal(TokenStream *stream, const char *source_code) {
  source_code++; // skip opening quote
  const char *start = source_code;
  // first pass: find the end of the string literal
  while (*source_code && *source_code != '"') {
    if (*source_code == '\\') {
      source_code++; // skip over the escape backslash
      if (*source_code == '\0') {
        break;
      }
    }
    if (*source_code == '\n') {
      stream->line++;
    }
    source_code++;
  }
  if (*source_code == '\0') {
    err("line %d: unterminated string literal (missing closing \")", stream->line);
  }

  // second pass: copy the string literal to a new heap string
  const size_t len  = (size_t)(source_code - start);
  char        *text = safe_alloc(len + 1);

  const char *iterator = start;
  size_t      index    = 0;
  while (iterator < source_code) {
    // handle escape sequences
    if (*iterator == '\\') {
      iterator++;
      switch (*iterator) {
        case 'n':
          text[index++] = '\n';
          break;
        case 't':
          text[index++] = '\t';
          break;
        case 'r':
          text[index++] = '\r';
          break;
        case '0':
          text[index++] = '\0';
          break;
        case '\\':
          text[index++] = '\\';
          break;
        case '\'':
          text[index++] = '\'';
          break;
        case 'b':
          text[index++] = '\b';
          break;
        case '"':
          text[index++] = '"';
          break;
        default:
          err("line %d: unknown escape sequence: \\%c", stream->line, *iterator);
          text[index++] = *iterator;
      }
    } else {
      text[index++] = *iterator;
    }
    iterator++;
  }
  // null terminate the string
  text[index] = '\0';

  // add the string token
  Token *token = add_token(stream, STRING);
  token->text  = text;
  source_code++; // skip closing quote
  return source_code;
}

const char *consume_identifier_or_keyword(TokenStream *stream, const char *source_code) {
  const char *start = source_code++;
  // consume all alphanumeric characters and underscores
  while (isalnum(*source_code) || *source_code == '_') {
    source_code++;
  }
  // extract the identifier or keyword
  char *text = extract(start, source_code);
  // check if the identifier is a keyword
  if (strcmp(text, "else") == 0) {
    free(text);
    add_token(stream, ELSE);
  } else if (strcmp(text, "if") == 0) {
    free(text);
    add_token(stream, IF);
  } else if (strcmp(text, "return") == 0) {
    free(text);
    add_token(stream, RETURN);
  } else if (strcmp(text, "struct") == 0) {
    free(text);
    add_token(stream, STRUCT);
  } else if (strcmp(text, "void") == 0) {
    free(text);
    add_token(stream, VOID);
  } else if (strcmp(text, "while") == 0) {
    free(text);
    add_token(stream, WHILE);
  } else if (strcmp(text, "int") == 0) {
    free(text);
    add_token(stream, TYPE_INT);
  } else if (strcmp(text, "double") == 0) {
    free(text);
    add_token(stream, TYPE_DOUBLE);
  } else if (strcmp(text, "char") == 0) {
    free(text);
    add_token(stream, TYPE_CHAR);
  } else {
    // add the identifier token
    Token *token = add_token(stream, ID);
    token->text  = text;
  }
  return source_code;
}

const char *consume_numeric_literal(TokenStream *stream, const char *source_code) {
  const char *start = source_code;
  // consume all digits
  while (isdigit(*source_code)) {
    source_code++;
  }

  int is_double = 0;
  // check for decimal point
  if (*source_code == '.') {
    is_double = 1;
    source_code++;
    // consume all digits after decimal point (necessary)
    if (isdigit(*source_code)) {
      while (isdigit(*source_code)) {
        source_code++;
      }
    } else {
      err("line %d: missing exponent digits after '.'", stream->line);
    }
  }

  // check for scientific notation
  if (*source_code == 'e' || *source_code == 'E') {
    const char *e_start = source_code;
    source_code++;
    // check for exponent sign
    if (*source_code == '+' || *source_code == '-') {
      source_code++;
    }
    // check for exponent digits
    if (isdigit(*source_code)) {
      is_double = 1;
      while (isdigit(*source_code)) {
        source_code++;
      }
    } else {
      const char *e_sign = "";
      if (source_code > e_start + 1) {
        e_sign = source_code[-1] == '+' ? "+" : "-";
      }
      err("line %d: missing exponent digits after '%c%s'", stream->line, *e_start, e_sign);
    }
  }

  // extract the numeric literal
  char *num_str = extract(start, source_code);
  char *endptr;
  // check if the numeric literal is a double
  if (is_double) {
    Token *token        = add_token(stream, DOUBLE);
    errno               = 0;
    token->double_value = strtod(num_str, &endptr);
    if (errno == ERANGE) {
      err("line %d: numeric literal out of range for double: %s", stream->line, num_str);
    }
    if (*endptr != '\0') {
      err("line %d: invalid numeric literal: %s", stream->line, num_str);
    }
  } else {
    // add the integer token
    Token *token = add_token(stream, INT);
    errno        = 0;
    long val     = strtol(num_str, &endptr, 10);
    if (errno == ERANGE) {
      err("line %d: numeric literal out of range for int: %s", stream->line, num_str);
    }
    if (*endptr != '\0') {
      err("line %d: invalid numeric literal: %s", stream->line, num_str);
    }
    token->integer_value = (int)val;
  }
  free(num_str);
  return source_code;
}

const char *consume_comment_or_div(TokenStream *stream, const char *source_code) {
  if (source_code[1] == '/') {
    // single-line comment — consume until end of line
    source_code += 2;
    while (*source_code && *source_code != '\n' && *source_code != '\r') {
      source_code++;
    }
  } else if (source_code[1] == '*') {
    // block comment — consume until */
    source_code += 2;
    for (;;) {
      if (*source_code == '\0') {
        err("line %d: unterminated block comment (missing */ at the end)", stream->line);
      }
      if (*source_code == '\n' || (*source_code == '\r' && source_code[1] != '\n')) {
        stream->line++;
      }
      if (*source_code == '*' && source_code[1] == '/') {
        source_code += 2;
        break;
      }
      source_code++;
    }
  } else {
    // add the division token
    add_token(stream, DIV);
    source_code++;
  }
  return source_code;
}

void tokenize(TokenStream *stream, const char *source_code) {
  for (;;) {
    switch (*source_code) {

      // ── whitespace
      // ────────────────────────────────────────────────────────────
      case ' ':
      case '\t':
        source_code++;
        break;
      case '\r': // handles Windows (\r\n), classic Mac (\r) newlines
        if (source_code[1] == '\n') {
          source_code++;
        }
        // fallthrough to \n
      case '\n':
        stream->line++;
        source_code++;
        break;

      // ── end of input
      // ──────────────────────────────────────────────────────────
      case '\0':
        add_token(stream, END);
        return;

      // ── delimiters
      // ────────────────────────────────────────────────────────────
      case ',':
        add_token(stream, COMMA);
        source_code++;
        break;
      case ';':
        add_token(stream, SEMICOLON);
        source_code++;
        break;
      case '(':
        add_token(stream, LPAR);
        source_code++;
        break;
      case ')':
        add_token(stream, RPAR);
        source_code++;
        break;
      case '[':
        add_token(stream, LBRACKET);
        source_code++;
        break;
      case ']':
        add_token(stream, RBRACKET);
        source_code++;
        break;
      case '{':
        add_token(stream, LACC);
        source_code++;
        break;
      case '}':
        add_token(stream, RACC);
        source_code++;
        break;

      // ── operators
      // ─────────────────────────────────────────────────────────────
      case '+':
        add_token(stream, ADD);
        source_code++;
        break;
      case '-':
        add_token(stream, SUB);
        source_code++;
        break;
      case '*':
        add_token(stream, MUL);
        source_code++;
        break;
      case '/':
        source_code = consume_comment_or_div(stream, source_code);
        break;
      case '%':
        add_token(stream, MOD);
        source_code++;
        break;
      case '.':
        add_token(stream, DOT);
        source_code++;
        break;
      case '&':
        if (source_code[1] == '&') {
          add_token(stream, AND);
          source_code += 2;
        } else {
          err("line %d: invalid char: %c (%d); maybe you meant &&?", stream->line, *source_code,
              *source_code);
        }
        break;
      case '|':
        if (source_code[1] == '|') {
          add_token(stream, OR);
          source_code += 2;
        } else {
          err("line %d: invalid char: %c (%d); maybe you meant ||?", stream->line, *source_code,
              *source_code);
        }
        break;
      case '!':
        if (source_code[1] == '=') {
          add_token(stream, NOTEQ);
          source_code += 2;
        } else {
          add_token(stream, NOT);
          source_code++;
        }
        break;
      case '=':
        if (source_code[1] == '=') {
          add_token(stream, EQUAL);
          source_code += 2;
        } else {
          add_token(stream, ASSIGN);
          source_code++;
        }
        break;
      case '<':
        if (source_code[1] == '=') {
          add_token(stream, LESSEQ);
          source_code += 2;
        } else {
          add_token(stream, LESS);
          source_code++;
        }
        break;
      case '>':
        if (source_code[1] == '=') {
          add_token(stream, GREATEREQ);
          source_code += 2;
        } else {
          add_token(stream, GREATER);
          source_code++;
        }
        break;

      // ── char literal
      // ──────────────────────────────────────────────────────────
      case '\'':
        source_code = consume_char_literal(stream, source_code);
        break;

      // ── string literal
      // ────────────────────────────────────────────────────────
      case '"':
        source_code = consume_string_literal(stream, source_code);
        break;

      default:
        // ── identifiers & keywords ────────────────────────────────────────────
        if (isalpha(*source_code) || *source_code == '_') {
          source_code = consume_identifier_or_keyword(stream, source_code);
        }
        // ── numeric literals (int / double) ───────────────────────────────────
        else if (isdigit(*source_code)) {
          source_code = consume_numeric_literal(stream, source_code);
        } else {
          err("line %d: invalid char: %c (%d)", stream->line, *source_code, *source_code);
        }
    }
  }
}
