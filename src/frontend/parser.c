#include "../../include/frontend/parser.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/frontend/token.h"

// FUNCTION DECLARATIONS

bool consume(TokenStream *stream, TokenType type);
bool type_base(TokenStream *stream);
bool function_definition(TokenStream *stream);
bool variable_definition(TokenStream *stream);
bool struct_definition(TokenStream *stream);
bool array_declaration(TokenStream *stream);
bool function_parameter_definition(TokenStream *stream);
bool stm_definition(TokenStream *stream);
bool stm_compound_definition(TokenStream *stream);
bool expression(TokenStream *stream);
bool assignment_expression(TokenStream *stream);
bool or_expression(TokenStream *stream);
bool and_expression(TokenStream *stream);
bool equal_expression(TokenStream *stream);
bool relational_expression(TokenStream *stream);
bool addition_expression(TokenStream *stream);
bool multiplication_expression(TokenStream *stream);
bool cast_expression(TokenStream *stream);
bool unary_expression(TokenStream *stream);
bool postfix_expression(TokenStream *stream);
bool primary_expression(TokenStream *stream);

bool consume(TokenStream *stream, TokenType type) {
  if (stream->tokens.iterator->type == type) {
    stream->tokens.iterator = stream->tokens.iterator->next;
    return true;
  }
  return false;
}

// unit: ( struct_def | fn_def | var_def )* END
bool unit(TokenStream *stream) {
  for (;;) {
    if (struct_definition(stream)) {
    } else if (function_definition(stream)) {
    } else if (variable_definition(stream)) {
    } else {
      break;
    }
  }
  if (consume(stream, END)) {
    return true;
  }
  return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool struct_definition(TokenStream *stream) {
  Token *start = stream->tokens.iterator;
  if (consume(stream, STRUCT)) {
    if (consume(stream, ID)) {
      if (consume(stream, LACC)) {
        // committed: LACC seen, must be a struct definition
        while (variable_definition(stream)) {
        }
        if (!consume(stream, RACC)) {
          token_stream_error(stream, "expected } to close struct body");
        }
        if (!consume(stream, SEMICOLON)) {
          token_stream_error(stream, "expected ; after struct definition");
        }
        return true;
      }
      // STRUCT ID without LACC: backtrack only if next token is an ID,
      // meaning this is a type usage (struct Foo myVar; or struct Foo myFn())
      if (stream->tokens.iterator->type != ID) {
        token_stream_error(stream, "expected { to start struct body");
      }
    }
    stream->tokens.iterator = start; // backtrack: STRUCT ID followed by ID is a varDef/fnDef
  }
  return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool variable_definition(TokenStream *stream) {
  const Token *type_start = stream->tokens.iterator;
  if (!type_base(stream)) {
    return false;
  }
  if (!consume(stream, ID)) {
    char buf[64];
    token_stream_error(stream, "expected identifier after '%s'",
                       token_type_base_name(type_start, buf, sizeof(buf)));
  }
  array_declaration(stream); // optional
  if (!consume(stream, SEMICOLON)) {
    token_stream_error(stream, "expected ; after variable definition");
  }
  return true;
}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool type_base(TokenStream *stream) {
  if (consume(stream, TYPE_INT)) {
    return true;
  }
  if (consume(stream, TYPE_DOUBLE)) {
    return true;
  }
  if (consume(stream, TYPE_CHAR)) {
    return true;
  }
  if (consume(stream, STRUCT)) {
    if (consume(stream, ID)) {
      return true;
    }
    token_stream_error(stream, "Need an identifier after declaring a struct");
  }
  return false;
}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR
// stmCompound
bool function_definition(TokenStream *stream) {
  Token *start = stream->tokens.iterator;
  if (!consume(stream, VOID) && !type_base(stream)) {
    return false;
  }
  if (!consume(stream, ID)) {
    stream->tokens.iterator = start;
    return false;
  }
  if (!consume(stream, LPAR)) {
    stream->tokens.iterator = start; // backtrack: could be varDef
    return false;
  }
  // committed: LPAR foundl
  if (function_parameter_definition(stream)) {
    while (consume(stream, COMMA)) {
      if (!function_parameter_definition(stream)) {
        token_stream_error(stream, "expected parameter after ,");
      }
    }
  }
  if (!consume(stream, RPAR)) {
    if (stream->tokens.iterator->type == ID) {
      token_stream_error(stream, "missing type before parameter '%s'",
                         stream->tokens.iterator->text);
    }
    if (stream->tokens.iterator->type == COMMA) {
      token_stream_error(stream, "extra comma in parameter definition");
    }
    token_stream_error(stream, "expected ) after function parameters");
  }
  if (!stm_compound_definition(stream)) {
    token_stream_error(stream, "expected function body");
  }
  return true;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool array_declaration(TokenStream *stream) {
  if (consume(stream, LBRACKET)) {
    consume(stream, INT); // optional size
    if (consume(stream, RBRACKET)) {
      return true;
    }
    token_stream_error(stream, "expected ] in array declaration");
  }

  // Lookahead: if the next token looks like an array size or a lone ],
  // the opening [ was most likely forgotten — emit a precise diagnostic.
  const Token *peek = stream->tokens.iterator;
  if (peek->type == RBRACKET) {
    token_stream_error(stream, "missing [ before ]");
  }
  if (peek->type == INT && peek->next && peek->next->type == RBRACKET) {
    token_stream_error(stream, "missing [ before array size %d", peek->integer_value);
  }

  return false;
}

// fnParam: typeBase ID arrayDecl?
bool function_parameter_definition(TokenStream *stream) {
  if (!type_base(stream)) {
    return false;
  }
  if (!consume(stream, ID)) {
    token_stream_error(stream, "expected identifier in function parameter");
  }
  array_declaration(stream); // optional
  return true;
}

// stmCompound: LACC ( varDef | stm )* RACC
bool stm_compound_definition(TokenStream *stream) {
  if (!consume(stream, LACC)) {
    return false;
  }
  while (variable_definition(stream) || stm_definition(stream)) {
  }
  if (!consume(stream, RACC)) {
    token_stream_error(stream, "expected } to close compound statement");
  }
  return true;
}

// stm: stmCompound
//    | IF LPAR expr RPAR stm ( ELSE stm )?
//    | WHILE LPAR expr RPAR stm
//    | RETURN expr? SEMICOLON
//    | expr? SEMICOLON
bool stm_definition(TokenStream *stream) {
  if (stm_compound_definition(stream)) {
    return true;
  }
  if (consume(stream, IF)) {
    if (!consume(stream, LPAR)) {
      token_stream_error(stream, "expected ( after if");
    }
    if (!expression(stream)) {
      token_stream_error(stream, "expected expression in if condition");
    }
    if (!consume(stream, RPAR)) {
      token_stream_error(stream, "expected ) after if condition");
    }
    if (!stm_definition(stream)) {
      token_stream_error(stream, "expected statement after if");
    }
    if (consume(stream, ELSE)) {
      if (!stm_definition(stream)) {
        token_stream_error(stream, "expected statement after else");
      }
    }
    return true;
  }
  if (consume(stream, WHILE)) {
    if (!consume(stream, LPAR)) {
      token_stream_error(stream, "expected ( after while");
    }
    if (!expression(stream)) {
      token_stream_error(stream, "expected expression in while condition");
    }
    if (!consume(stream, RPAR)) {
      token_stream_error(stream, "expected ) after while condition");
    }
    if (!stm_definition(stream)) {
      token_stream_error(stream, "expected statement after while");
    }
    return true;
  }
  if (consume(stream, RETURN)) {
    expression(stream); // optional
    if (!consume(stream, SEMICOLON)) {
      token_stream_error(stream, "expected ; after return");
    }
    return true;
  }
  // expr? SEMICOLON
  if (expression(stream)) {
    if (!consume(stream, SEMICOLON)) {
      token_stream_error(stream, "expected ; after expression");
    }
    return true;
  }
  if (consume(stream, SEMICOLON)) {
    return true; // empty statement
  }
  return false;
}

bool expression(TokenStream *stream) {
  if (assignment_expression(stream)) {
    return true;
  }

  return false;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
// Note: parse exprOr first (covers casts, unary, etc.), then check for ASSIGN.
// Lvalue validation is left to semantic analysis.
bool assignment_expression(TokenStream *stream) {
  if (!or_expression(stream)) {
    return false;
  }
  if (consume(stream, ASSIGN)) {
    if (!assignment_expression(stream)) {
      token_stream_error(stream, "expected expression after =");
    }
  }
  return true;
}

// exprOr: exprOr OR exprAnd | exprAnd  =>  exprAnd ( OR exprAnd )*
bool or_expression(TokenStream *stream) {
  if (!and_expression(stream)) {
    return false;
  }
  while (consume(stream, OR)) {
    if (!and_expression(stream)) {
      token_stream_error(stream, "expected expression after ||");
    }
  }
  return true;
}

// exprAnd: exprAnd AND exprEq | exprEq  =>  exprEq ( AND exprEq )*
bool and_expression(TokenStream *stream) {
  if (!equal_expression(stream)) {
    return false;
  }
  while (consume(stream, AND)) {
    if (!equal_expression(stream)) {
      token_stream_error(stream, "expected expression after &&");
    }
  }
  return true;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel  =>  exprRel ( ( EQUAL |
// NOTEQ ) exprRel )*
bool equal_expression(TokenStream *stream) {
  if (!relational_expression(stream)) {
    return false;
  }
  TokenType op;
  while ((op = stream->tokens.iterator->type) == EQUAL || op == NOTEQ) {
    consume(stream, op);
    if (!relational_expression(stream)) {
      token_stream_error(stream, "expected expression after '%s'",
                         op == EQUAL ? "==" : "!=");
    }
  }
  return true;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
//       =>  exprAdd ( ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd )*
bool relational_expression(TokenStream *stream) {
  if (!addition_expression(stream)) {
    return false;
  }
  TokenType op;
  while ((op = stream->tokens.iterator->type) == LESS || op == LESSEQ ||
         op == GREATER || op == GREATEREQ) {
    consume(stream, op);
    if (!addition_expression(stream)) {
      const char *sym = op == LESS ? "<" : op == LESSEQ ? "<=" : op == GREATER ? ">" : ">=";
      token_stream_error(stream, "expected expression after '%s'", sym);
    }
  }
  return true;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul  =>  exprMul ( ( ADD | SUB )
// exprMul )*
bool addition_expression(TokenStream *stream) {
  if (!multiplication_expression(stream)) {
    return false;
  }
  while (consume(stream, ADD) || consume(stream, SUB)) {
    if (!multiplication_expression(stream)) {
      token_stream_error(stream, "expected expression after + or -");
    }
  }
  return true;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast  =>  exprCast ( ( MUL |
// DIV ) exprCast )*
bool multiplication_expression(TokenStream *stream) {
  if (!cast_expression(stream)) {
    return false;
  }
  while (consume(stream, MUL) || consume(stream, DIV)) {
    if (!cast_expression(stream)) {
      token_stream_error(stream, "expected expression after * or /");
    }
  }
  return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool cast_expression(TokenStream *stream) {
  Token *start = stream->tokens.iterator;
  if (consume(stream, LPAR)) {
    if (type_base(stream)) {
      array_declaration(stream); // optional
      if (!consume(stream, RPAR)) {
        token_stream_error(stream, "expected ) in cast expression");
      }
      if (!cast_expression(stream)) {
        token_stream_error(stream, "expected expression after cast");
      }
      return true;
    }
    stream->tokens.iterator = start; // backtrack: not a cast, try exprUnary
  }
  return unary_expression(stream);
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool unary_expression(TokenStream *stream) {
  if (consume(stream, SUB) || consume(stream, NOT)) {
    if (!unary_expression(stream)) {
      token_stream_error(stream, "expected expression after unary operator");
    }
    return true;
  }
  return postfix_expression(stream);
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID |
// exprPrimary
//           =>  exprPrimary ( LBRACKET expr RBRACKET | DOT ID )*
bool postfix_expression(TokenStream *stream) {
  if (!primary_expression(stream)) {
    return false;
  }
  for (;;) {
    if (consume(stream, LBRACKET)) {
      if (!expression(stream)) {
        token_stream_error(stream, "expected expression in array index");
      }
      if (!consume(stream, RBRACKET)) {
        token_stream_error(stream, "expected ] after array index");
      }
    } else if (consume(stream, DOT)) {
      if (!consume(stream, ID)) {
        token_stream_error(stream, "expected identifier after .");
      }
    } else {
      break;
    }
  }
  return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE |
// CHAR | STRING | LPAR expr RPAR
bool primary_expression(TokenStream *stream) {
  if (consume(stream, ID)) {
    if (consume(stream, LPAR)) {
      if (expression(stream)) {
        while (consume(stream, COMMA)) {
          if (!expression(stream)) {
            token_stream_error(stream, "expected expression after ,");
          }
        }
      }
      if (!consume(stream, RPAR)) {
        token_stream_error(stream, "expected ) after function call arguments");
      }
    }
    return true;
  }

  if (consume(stream, INT) || consume(stream, DOUBLE) || consume(stream, CHAR) ||
      consume(stream, STRING)) {
    return true;
  }

  if (consume(stream, LPAR)) {
    if (!expression(stream)) {
      token_stream_error(stream, "expected expression after (");
    }
    if (!consume(stream, RPAR)) {
      token_stream_error(stream, "expected ) after expression");
    }
    return true;
  }
  return false;
}

void parse(TokenStream *stream) {
  stream->tokens.iterator = stream->tokens.head;
  if (!unit(stream)) {
    token_stream_error(stream, "syntax error");
  }
}
