#include "../../include/frontend/parser.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/frontend/token.h"
#include "../../include/utils/utils.h"

// CONTEXT

typedef struct {
  TokenStream    *stream;
  DomainAnalyzer *domain_analyzer;
  Symbol         *owner;
} ParserContext;

// FUNCTION DECLARATIONS

bool consume(ParserContext *ctx, TokenType type);
bool unit(ParserContext *ctx);
bool type_base(ParserContext *ctx, Type *t);
bool function_definition(ParserContext *ctx);
bool variable_definition(ParserContext *ctx);
bool struct_definition(ParserContext *ctx);
bool array_declaration(ParserContext *ctx, Type *t);
bool function_parameter_definition(ParserContext *ctx);
bool stm_definition(ParserContext *ctx);
bool stm_compound_definition(ParserContext *ctx, bool new_domain);
bool expression(ParserContext *ctx);
bool assignment_expression(ParserContext *ctx);
bool or_expression(ParserContext *ctx);
bool and_expression(ParserContext *ctx);
bool equal_expression(ParserContext *ctx);
bool relational_expression(ParserContext *ctx);
bool addition_expression(ParserContext *ctx);
bool multiplication_expression(ParserContext *ctx);
bool cast_expression(ParserContext *ctx);
bool unary_expression(ParserContext *ctx);
bool postfix_expression(ParserContext *ctx);
bool primary_expression(ParserContext *ctx);

bool consume(ParserContext *ctx, TokenType type) {
  if (ctx->stream->tokens.iterator->type == type) {
    ctx->stream->tokens.consumed = ctx->stream->tokens.iterator;
    ctx->stream->tokens.iterator = ctx->stream->tokens.iterator->next;
    return true;
  }
  return false;
}

void parse(TokenStream *stream, DomainAnalyzer *domain_analyzer) {
  ParserContext ctx = {
      .stream          = stream,
      .domain_analyzer = domain_analyzer,
      .owner           = NULL,
  };
  ctx.stream->tokens.iterator = ctx.stream->tokens.head;
  if (!unit(&ctx)) {
    token_stream_error(ctx.stream, "syntax error");
  }
}

// unit: ( struct_def | fn_def | var_def )* END
bool unit(ParserContext *ctx) {
  for (;;) {
    if (struct_definition(ctx)) {
    } else if (function_definition(ctx)) {
    } else if (variable_definition(ctx)) {
    } else {
      break;
    }
  }
  if (consume(ctx, END)) {
    return true;
  }
  return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool struct_definition(ParserContext *ctx) {
  Token *start = ctx->stream->tokens.iterator;
  if (consume(ctx, STRUCT)) {
    if (consume(ctx, ID)) {
      Token *tk_name = ctx->stream->tokens.consumed;
      if (consume(ctx, LACC)) {
        Symbol *s = find_symbol_in_domain(ctx->domain_analyzer->symbol_table, tk_name->text);
        if (s) {
          token_stream_error(ctx->stream, "symbol redefinition: %s", tk_name->text);
        }
        s = add_symbol_to_domain(ctx->domain_analyzer->symbol_table,
                                 new_symbol(tk_name->text, SYMBOL_KIND_STRUCT));
        s->type.type_base      = TYPE_BASE_STRUCT;
        s->type.symbol         = s;
        s->type.array_dimension = -1;

        push_domain(ctx->domain_analyzer);
        ctx->owner = s;

        // committed: LACC seen, must be a struct definition
        while (variable_definition(ctx)) {
        }
        if (!consume(ctx, RACC)) {
          token_stream_error(ctx->stream, "expected } to close struct body");
        }
        if (!consume(ctx, SEMICOLON)) {
          token_stream_error(ctx->stream, "expected ; after struct definition");
        }

        ctx->owner = NULL;
        drop_domain(ctx->domain_analyzer);

        return true;
      }
      // STRUCT ID without LACC: backtrack only if next token is an ID,
      // meaning this is a type usage (struct Foo myVar; or struct Foo myFn())
      if (ctx->stream->tokens.iterator->type != ID) {
        token_stream_error(ctx->stream, "expected { to start struct body");
      }
    }
    ctx->stream->tokens.iterator = start; // backtrack: STRUCT ID followed by ID is a varDef/fnDef
  }
  return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool variable_definition(ParserContext *ctx) {
  const Token *type_start = ctx->stream->tokens.iterator;
  Type t;
  if (!type_base(ctx, &t)) {
    return false;
  }
  if (!consume(ctx, ID)) {
    char buf[64];
    token_stream_error(ctx->stream, "expected identifier after '%s'",
                       token_type_base_name(type_start, buf, sizeof(buf)));
  }
  Token *tk_name = ctx->stream->tokens.consumed;
  if (array_declaration(ctx, &t)) {
    if (t.array_dimension == 0) {
      token_stream_error(ctx->stream, "a vector variable must have a specified dimension");
    }
  }
  if (!consume(ctx, SEMICOLON)) {
    token_stream_error(ctx->stream, "expected ; after variable definition");
  }

  Symbol *var = find_symbol_in_domain(ctx->domain_analyzer->symbol_table, tk_name->text);
  if (var) {
    token_stream_error(ctx->stream, "symbol redefinition: %s", tk_name->text);
  }
  var        = new_symbol(tk_name->text, SYMBOL_KIND_VARIABLE);
  var->type  = t;
  var->owner = ctx->owner;
  add_symbol_to_domain(ctx->domain_analyzer->symbol_table, var);

  if (ctx->owner) {
    switch (ctx->owner->kind) {
    case SYMBOL_KIND_FUNCTION:
      var->var_index = symbols_len(ctx->owner->function.locals);
      add_symbol_to_list(&ctx->owner->function.locals, duplicate_symbol(var));
      break;
    case SYMBOL_KIND_STRUCT:
      var->var_index = type_size(&ctx->owner->type);
      add_symbol_to_list(&ctx->owner->struct_members, duplicate_symbol(var));
      break;
    default:
      break;
    }
  } else {
    var->var_mem = safe_alloc(type_size(&t));
  }

  return true;
}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool type_base(ParserContext *ctx, Type *t) {
  t->array_dimension = -1;
  if (consume(ctx, TYPE_INT)) {
    t->type_base = TYPE_BASE_INT;
    return true;
  }
  if (consume(ctx, TYPE_DOUBLE)) {
    t->type_base = TYPE_BASE_DOUBLE;
    return true;
  }
  if (consume(ctx, TYPE_CHAR)) {
    t->type_base = TYPE_BASE_CHAR;
    return true;
  }
  if (consume(ctx, STRUCT)) {
    if (consume(ctx, ID)) {
      Token *tk_name = ctx->stream->tokens.consumed;
      t->type_base   = TYPE_BASE_STRUCT;
      t->symbol      = find_symbol(ctx->domain_analyzer, tk_name->text);
      if (!t->symbol) {
        token_stream_error(ctx->stream, "undefined struct: %s", tk_name->text);
      }
      return true;
    }
    token_stream_error(ctx->stream, "Need an identifier after declaring a struct");
  }
  return false;
}

// fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR
// stmCompound
bool function_definition(ParserContext *ctx) {
  Token *start = ctx->stream->tokens.iterator;
  Type t;
  if (consume(ctx, VOID)) {
    t.type_base = TYPE_BASE_VOID;
  } else if (!type_base(ctx, &t)) {
    return false;
  }
  if (!consume(ctx, ID)) {
    ctx->stream->tokens.iterator = start;
    return false;
  }
  Token *tk_name = ctx->stream->tokens.consumed;
  if (!consume(ctx, LPAR)) {
    ctx->stream->tokens.iterator = start; // backtrack: could be varDef
    return false;
  }

  Symbol *fn = find_symbol_in_domain(ctx->domain_analyzer->symbol_table, tk_name->text);
  if (fn) {
    token_stream_error(ctx->stream, "symbol redefinition: %s", tk_name->text);
  }
  fn             = new_symbol(tk_name->text, SYMBOL_KIND_FUNCTION);
  fn->type       = t;
  fn->owner      = NULL; // global function
  add_symbol_to_domain(ctx->domain_analyzer->symbol_table, fn);
  ctx->owner = fn;
  push_domain(ctx->domain_analyzer);

  // committed: LPAR found
  if (function_parameter_definition(ctx)) {
    while (consume(ctx, COMMA)) {
      if (!function_parameter_definition(ctx)) {
        token_stream_error(ctx->stream, "expected parameter after ,");
      }
    }
  }
  if (!consume(ctx, RPAR)) {
    if (ctx->stream->tokens.iterator->type == ID) {
      token_stream_error(ctx->stream, "missing type before parameter '%s'",
                         ctx->stream->tokens.iterator->text);
    }
    if (ctx->stream->tokens.iterator->type == COMMA) {
      token_stream_error(ctx->stream, "extra comma in parameter definition");
    }
    token_stream_error(ctx->stream, "expected ) after function parameters");
  }
  if (!stm_compound_definition(ctx, false)) {
    token_stream_error(ctx->stream, "expected function body");
  }

  drop_domain(ctx->domain_analyzer);
  ctx->owner = NULL;

  return true;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool array_declaration(ParserContext *ctx, Type *t) {
  if (consume(ctx, LBRACKET)) {
    if (consume(ctx, INT)) {
      Token *tk_size     = ctx->stream->tokens.consumed;
      t->array_dimension = tk_size->integer_value;
    } else {
      t->array_dimension = 0;
    }
    if (consume(ctx, RBRACKET)) {
      return true;
    }
    token_stream_error(ctx->stream, "expected ] in array declaration");
  }

  // Lookahead: if the next token looks like an array size or a lone ],
  // the opening [ was most likely forgotten — emit a precise diagnostic.
  const Token *peek = ctx->stream->tokens.iterator;
  if (peek->type == RBRACKET) {
    token_stream_error(ctx->stream, "missing [ before ]");
  }
  if (peek->type == INT && peek->next && peek->next->type == RBRACKET) {
    token_stream_error(ctx->stream, "missing [ before array size %d", peek->integer_value);
  }

  return false;
}

// fnParam: typeBase ID arrayDecl?
bool function_parameter_definition(ParserContext *ctx) {
  Type t;
  if (!type_base(ctx, &t)) {
    return false;
  }
  if (!consume(ctx, ID)) {
    token_stream_error(ctx->stream, "expected identifier in function parameter");
  }
  Token *tk_name = ctx->stream->tokens.consumed;
  if (array_declaration(ctx, &t)) {
    t.array_dimension = 0; // arrays as parameters are always int v[]
  }

  Symbol *param = find_symbol_in_domain(ctx->domain_analyzer->symbol_table, tk_name->text);
  if (param) {
    token_stream_error(ctx->stream, "symbol redefinition: %s", tk_name->text);
  }
  param              = new_symbol(tk_name->text, SYMBOL_KIND_PARAMETER);
  param->type        = t;
  param->owner       = ctx->owner;
  param->param_index = symbols_len(ctx->owner->function.parameters);
  add_symbol_to_domain(ctx->domain_analyzer->symbol_table, param);
  add_symbol_to_list(&ctx->owner->function.parameters, duplicate_symbol(param));

  return true;
}

// stmCompound: LACC ( varDef | stm )* RACC
bool stm_compound_definition(ParserContext *ctx, bool new_domain) {
  if (!consume(ctx, LACC)) {
    return false;
  }
  if (new_domain) {
    push_domain(ctx->domain_analyzer);
  }
  while (variable_definition(ctx) || stm_definition(ctx)) {
  }
  if (!consume(ctx, RACC)) {
    token_stream_error(ctx->stream, "expected } to close compound statement");
  }
  if (new_domain) {
    drop_domain(ctx->domain_analyzer);
  }
  return true;
}

// stm: stmCompound
//    | IF LPAR expr RPAR stm ( ELSE stm )?
//    | WHILE LPAR expr RPAR stm
//    | RETURN expr? SEMICOLON
//    | expr? SEMICOLON
bool stm_definition(ParserContext *ctx) {
  if (stm_compound_definition(ctx, true)) {
    return true;
  }
  if (consume(ctx, IF)) {
    if (!consume(ctx, LPAR)) {
      token_stream_error(ctx->stream, "expected ( after if");
    }
    if (!expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression in if condition");
    }
    if (!consume(ctx, RPAR)) {
      token_stream_error(ctx->stream, "expected ) after if condition");
    }
    if (!stm_definition(ctx)) {
      token_stream_error(ctx->stream, "expected statement after if");
    }
    if (consume(ctx, ELSE)) {
      if (!stm_definition(ctx)) {
        token_stream_error(ctx->stream, "expected statement after else");
      }
    }
    return true;
  }
  if (consume(ctx, WHILE)) {
    if (!consume(ctx, LPAR)) {
      token_stream_error(ctx->stream, "expected ( after while");
    }
    if (!expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression in while condition");
    }
    if (!consume(ctx, RPAR)) {
      token_stream_error(ctx->stream, "expected ) after while condition");
    }
    if (!stm_definition(ctx)) {
      token_stream_error(ctx->stream, "expected statement after while");
    }
    return true;
  }
  if (consume(ctx, RETURN)) {
    expression(ctx); // optional
    if (!consume(ctx, SEMICOLON)) {
      token_stream_error(ctx->stream, "expected ; after return");
    }
    return true;
  }
  // expr? SEMICOLON
  if (expression(ctx)) {
    if (!consume(ctx, SEMICOLON)) {
      token_stream_error(ctx->stream, "expected ; after expression");
    }
    return true;
  }
  if (consume(ctx, SEMICOLON)) {
    return true; // empty statement
  }
  return false;
}

bool expression(ParserContext *ctx) {
  if (assignment_expression(ctx)) {
    return true;
  }

  return false;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
// Note: parse exprOr first (covers casts, unary, etc.), then check for ASSIGN.
// Lvalue validation is left to semantic analysis.
bool assignment_expression(ParserContext *ctx) {
  if (!or_expression(ctx)) {
    return false;
  }
  if (consume(ctx, ASSIGN)) {
    if (!assignment_expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after =");
    }
  }
  return true;
}

// exprOr: exprOr OR exprAnd | exprAnd  =>  exprAnd ( OR exprAnd )*
bool or_expression(ParserContext *ctx) {
  if (!and_expression(ctx)) {
    return false;
  }
  while (consume(ctx, OR)) {
    if (!and_expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after ||");
    }
  }
  return true;
}

// exprAnd: exprAnd AND exprEq | exprEq  =>  exprEq ( AND exprEq )*
bool and_expression(ParserContext *ctx) {
  if (!equal_expression(ctx)) {
    return false;
  }
  while (consume(ctx, AND)) {
    if (!equal_expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after &&");
    }
  }
  return true;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel  =>  exprRel ( ( EQUAL |
// NOTEQ ) exprRel )*
bool equal_expression(ParserContext *ctx) {
  if (!relational_expression(ctx)) {
    return false;
  }
  TokenType op;
  while ((op = ctx->stream->tokens.iterator->type) == EQUAL || op == NOTEQ) {
    consume(ctx, op);
    if (!relational_expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after '%s'",
                         op == EQUAL ? "==" : "!=");
    }
  }
  return true;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
//       =>  exprAdd ( ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd )*
bool relational_expression(ParserContext *ctx) {
  if (!addition_expression(ctx)) {
    return false;
  }
  TokenType op;
  while ((op = ctx->stream->tokens.iterator->type) == LESS || op == LESSEQ ||
         op == GREATER || op == GREATEREQ) {
    consume(ctx, op);
    if (!addition_expression(ctx)) {
      const char *sym = op == LESS ? "<" : op == LESSEQ ? "<=" : op == GREATER ? ">" : ">=";
      token_stream_error(ctx->stream, "expected expression after '%s'", sym);
    }
  }
  return true;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul  =>  exprMul ( ( ADD | SUB )
// exprMul )*
bool addition_expression(ParserContext *ctx) {
  if (!multiplication_expression(ctx)) {
    return false;
  }
  while (consume(ctx, ADD) || consume(ctx, SUB)) {
    if (!multiplication_expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after + or -");
    }
  }
  return true;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast  =>  exprCast ( ( MUL |
// DIV ) exprCast )*
bool multiplication_expression(ParserContext *ctx) {
  if (!cast_expression(ctx)) {
    return false;
  }
  while (consume(ctx, MUL) || consume(ctx, DIV)) {
    if (!cast_expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after * or /");
    }
  }
  return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool cast_expression(ParserContext *ctx) {
  Token *start = ctx->stream->tokens.iterator;
  if (consume(ctx, LPAR)) {
    Type t;
    if (type_base(ctx, &t)) {
      array_declaration(ctx, &t); // optional
      if (!consume(ctx, RPAR)) {
        token_stream_error(ctx->stream, "expected ) in cast expression");
      }
      if (!cast_expression(ctx)) {
        token_stream_error(ctx->stream, "expected expression after cast");
      }
      return true;
    }
    ctx->stream->tokens.iterator = start; // backtrack: not a cast, try exprUnary
  }
  return unary_expression(ctx);
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool unary_expression(ParserContext *ctx) {
  if (consume(ctx, SUB) || consume(ctx, NOT)) {
    if (!unary_expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after unary operator");
    }
    return true;
  }
  return postfix_expression(ctx);
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID |
// exprPrimary
//           =>  exprPrimary ( LBRACKET expr RBRACKET | DOT ID )*
bool postfix_expression(ParserContext *ctx) {
  if (!primary_expression(ctx)) {
    return false;
  }
  for (;;) {
    if (consume(ctx, LBRACKET)) {
      if (!expression(ctx)) {
        token_stream_error(ctx->stream, "expected expression in array index");
      }
      if (!consume(ctx, RBRACKET)) {
        token_stream_error(ctx->stream, "expected ] after array index");
      }
    } else if (consume(ctx, DOT)) {
      if (!consume(ctx, ID)) {
        token_stream_error(ctx->stream, "expected identifier after .");
      }
    } else {
      break;
    }
  }
  return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE |
// CHAR | STRING | LPAR expr RPAR
bool primary_expression(ParserContext *ctx) {
  if (consume(ctx, ID)) {
    if (consume(ctx, LPAR)) {
      if (expression(ctx)) {
        while (consume(ctx, COMMA)) {
          if (!expression(ctx)) {
            token_stream_error(ctx->stream, "expected expression after ,");
          }
        }
      }
      if (!consume(ctx, RPAR)) {
        token_stream_error(ctx->stream, "expected ) after function call arguments");
      }
    }
    return true;
  }

  if (consume(ctx, INT) || consume(ctx, DOUBLE) || consume(ctx, CHAR) ||
      consume(ctx, STRING)) {
    return true;
  }

  if (consume(ctx, LPAR)) {
    if (!expression(ctx)) {
      token_stream_error(ctx->stream, "expected expression after (");
    }
    if (!consume(ctx, RPAR)) {
      token_stream_error(ctx->stream, "expected ) after expression");
    }
    return true;
  }
  return false;
}
