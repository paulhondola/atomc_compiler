#pragma once

#include <stdint.h>
#include <stdio.h>

typedef enum : uint8_t {
  // identifiers
  ID,
  // keywords
  ELSE,
  IF,
  RETURN,
  STRUCT,
  VOID,
  WHILE,
  TYPE_INT,
  TYPE_DOUBLE,
  TYPE_CHAR,
  // data types
  INT,
  DOUBLE,
  CHAR,
  STRING,
  // delimiters
  COMMA,
  END,
  SEMICOLON,
  LPAR,
  RPAR,
  LBRACKET,
  RBRACKET,
  LACC,
  RACC,
  // operators
  ADD,
  SUB,
  MUL,
  DIV,
  DOT,
  AND,
  OR,
  NOT,
  ASSIGN,
  EQUAL,
  NOTEQ,
  LESS,
  LESSEQ,
  GREATER,
  GREATEREQ,
  // whitespace
  SPACE,
  LINECOMMENT,
  BLOCKCOMMENT,
} TokenType;

typedef struct Token {
  TokenType type;           // ID, TYPE_CHAR, ...
  int       line;           // the line from the input file
  union {
    char  *text;            // the chars for ID, STRING (dynamically allocated)
    int    integer_value;   // the value for INT
    char   character_value; // the value for CHAR
    double double_value;    // the value for DOUBLE
  };
  struct Token *next;       // next token in a simple linked list
} Token;

// returns the type's name from a token
const char *token_type_base_name(const Token *token, char *buffer, size_t buffer_size);

// prints the token's name to a given output file
void token_print_name(FILE *output_file, const Token *token);
