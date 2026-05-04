#include "../../include/frontend/token.h"

#include <stdio.h>

void token_print_name(FILE *out, const Token *token) {
  switch (token->type) {
    case INT:
      fprintf(out, "INT:%d", token->integer_value);
      break;
    case DOUBLE:
      fprintf(out, "DOUBLE:%.1f", token->double_value);
      break;
    case CHAR:
      fprintf(out, "CHAR:%c", token->character_value);
      break;
    case STRING:
      fprintf(out, "STRING:%s", token->text);
      break;
    case ID:
      fprintf(out, "ID:%s", token->text);
      break;
    case IF:
      fprintf(out, "IF");
      break;
    case ELSE:
      fprintf(out, "ELSE");
      break;
    case RETURN:
      fprintf(out, "RETURN");
      break;
    case STRUCT:
      fprintf(out, "STRUCT");
      break;
    case VOID:
      fprintf(out, "VOID");
      break;
    case WHILE:
      fprintf(out, "WHILE");
      break;
    case LPAR:
      fprintf(out, "LPAR");
      break;
    case RPAR:
      fprintf(out, "RPAR");
      break;
    case LBRACKET:
      fprintf(out, "LBRACKET");
      break;
    case RBRACKET:
      fprintf(out, "RBRACKET");
      break;
    case LACC:
      fprintf(out, "LACC");
      break;
    case RACC:
      fprintf(out, "RACC");
      break;
    case COMMA:
      fprintf(out, "COMMA");
      break;
    case SEMICOLON:
      fprintf(out, "SEMICOLON");
      break;
    case DOT:
      fprintf(out, "DOT");
      break;
    case ADD:
      fprintf(out, "ADD");
      break;
    case SUB:
      fprintf(out, "SUB");
      break;
    case MUL:
      fprintf(out, "MUL");
      break;
    case DIV:
      fprintf(out, "DIV");
      break;
    case AND:
      fprintf(out, "AND");
      break;
    case OR:
      fprintf(out, "OR");
      break;
    case NOT:
      fprintf(out, "NOT");
      break;
    case NOTEQ:
      fprintf(out, "NOTEQ");
      break;
    case EQUAL:
      fprintf(out, "EQUAL");
      break;
    case ASSIGN:
      fprintf(out, "ASSIGN");
      break;
    case LESS:
      fprintf(out, "LESS");
      break;
    case LESSEQ:
      printf("LESSEQ");
      break;
    case GREATER:
      fprintf(out, "GREATER");
      break;
    case GREATEREQ:
      fprintf(out, "GREATEREQ");
      break;
    case END:
      fprintf(out, "END");
      break;
    case TYPE_INT:
      fprintf(out, "TYPE_INT");
      break;
    case TYPE_DOUBLE:
      fprintf(out, "TYPE_DOUBLE");
      break;
    case TYPE_CHAR:
      fprintf(out, "TYPE_CHAR");
      break;
    default:
      fprintf(out, "UNKNOWN");
      break;
  }
}

// Returns a string representation of the typeBase starting at `token`.
// For STRUCT, includes the following ID: "struct Foo".
// The returned pointer is valid for the lifetime of the token list.
const char *token_type_base_name(const Token *token, char *buffer, size_t buffer_size) {
  switch (token->type) {
    case TYPE_INT:
      return "int";
    case TYPE_DOUBLE:
      return "double";
    case TYPE_CHAR:
      return "char";
    case STRUCT:
      snprintf(buffer, buffer_size, "struct %s", token->next->text);
      return buffer;
    default:
      return "?";
  }
}
