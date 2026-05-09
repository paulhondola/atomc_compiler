#include "../../include/frontend/parser.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/frontend/token.h"
#include "../../include/utils/utils.h"
#include "../../include/analyzer/domain_analyzer.h"
#include "../../include/analyzer/domain.h"
#include "../../include/analyzer/symbol.h"
#include "../../include/analyzer/type_analyzer.h"

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
bool expression(ParserContext *ctx, ReturnValue *r);
bool assignment_expression(ParserContext *ctx, ReturnValue *r);
bool or_expression(ParserContext *ctx, ReturnValue *r);
bool and_expression(ParserContext *ctx, ReturnValue *r);
bool equal_expression(ParserContext *ctx, ReturnValue *r);
bool relational_expression(ParserContext *ctx, ReturnValue *r);
bool addition_expression(ParserContext *ctx, ReturnValue *r);
bool multiplication_expression(ParserContext *ctx, ReturnValue *r);
bool cast_expression(ParserContext *ctx, ReturnValue *r);
bool unary_expression(ParserContext *ctx, ReturnValue *r);
bool postfix_expression(ParserContext *ctx, ReturnValue *r);
bool primary_expression(ParserContext *ctx, ReturnValue *r);

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
    ReturnValue rCond;
    if (!expression(ctx, &rCond)) {
      token_stream_error(ctx->stream, "expected expression in if condition");
    }
    if (!can_be_scalar(&rCond)) {
      token_stream_error(ctx->stream, "the if condition must be a scalar value");
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
    ReturnValue rCond;
    if (!expression(ctx, &rCond)) {
      token_stream_error(ctx->stream, "expected expression in while condition");
    }
    if (!can_be_scalar(&rCond)) {
      token_stream_error(ctx->stream, "the while condition must be a scalar value");
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
    ReturnValue rExpr;
    if (expression(ctx, &rExpr)) {
      if (ctx->owner->type.type_base == TYPE_BASE_VOID) {
        token_stream_error(ctx->stream, "a void function cannot return a value");
      }
      if (!can_be_scalar(&rExpr)) {
        token_stream_error(ctx->stream, "the return value must be a scalar value");
      }
      if (!convert_to(&rExpr.type, &ctx->owner->type)) {
        token_stream_error(ctx->stream,
                           "cannot convert the return expression type to the function return type");
      }
    } else {
      if (ctx->owner->type.type_base != TYPE_BASE_VOID) {
        token_stream_error(ctx->stream, "a non-void function must return a value");
      }
    }
    if (!consume(ctx, SEMICOLON)) {
      token_stream_error(ctx->stream, "expected ; after return");
    }
    return true;
  }
  // expr? SEMICOLON
  ReturnValue r;
  if (expression(ctx, &r)) {
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

bool expression(ParserContext *ctx, ReturnValue *r) {
  if (assignment_expression(ctx, r)) {
    return true;
  }
  return false;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool assignment_expression(ParserContext *ctx, ReturnValue *r) {
  if (!or_expression(ctx, r)) {
    return false;
  }
  if (consume(ctx, ASSIGN)) {
    ReturnValue rDst = *r;
    if (!assignment_expression(ctx, r)) {
      token_stream_error(ctx->stream, "expected expression after =");
    }
    if (!rDst.is_left_value) {
      token_stream_error(ctx->stream, "the assign destination must be a left-value");
    }
    if (rDst.is_constant) {
      token_stream_error(ctx->stream, "the assign destination cannot be constant");
    }
    if (!can_be_scalar(&rDst)) {
      token_stream_error(ctx->stream, "the assign destination must be scalar");
    }
    if (!can_be_scalar(r)) {
      token_stream_error(ctx->stream, "the assign source must be scalar");
    }
    if (!convert_to(&r->type, &rDst.type)) {
      token_stream_error(ctx->stream, "the assign source cannot be converted to destination");
    }
    r->is_left_value = false;
    r->is_constant   = true;
  }
  return true;
}

// exprOr: exprOr OR exprAnd | exprAnd  =>  exprAnd ( OR exprAnd )*
bool or_expression(ParserContext *ctx, ReturnValue *r) {
  if (!and_expression(ctx, r)) {
    return false;
  }
  while (consume(ctx, OR)) {
    ReturnValue right;
    if (!and_expression(ctx, &right)) {
      token_stream_error(ctx->stream, "expected expression after ||");
    }
    Type tDst;
    if (!arithmetic_type_to(&r->type, &right.type, &tDst)) {
      token_stream_error(ctx->stream, "invalid operand type for ||");
    }
    r->type          = (Type){TYPE_BASE_INT, NULL, -1};
    r->is_left_value = false;
    r->is_constant   = true;
  }
  return true;
}

// exprAnd: exprAnd AND exprEq | exprEq  =>  exprEq ( AND exprEq )*
bool and_expression(ParserContext *ctx, ReturnValue *r) {
  if (!equal_expression(ctx, r)) {
    return false;
  }
  while (consume(ctx, AND)) {
    ReturnValue right;
    if (!equal_expression(ctx, &right)) {
      token_stream_error(ctx->stream, "expected expression after &&");
    }
    Type tDst;
    if (!arithmetic_type_to(&r->type, &right.type, &tDst)) {
      token_stream_error(ctx->stream, "invalid operand type for &&");
    }
    r->type          = (Type){TYPE_BASE_INT, NULL, -1};
    r->is_left_value = false;
    r->is_constant   = true;
  }
  return true;
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel  =>  exprRel ( ( EQUAL | NOTEQ ) exprRel )*
bool equal_expression(ParserContext *ctx, ReturnValue *r) {
  if (!relational_expression(ctx, r)) {
    return false;
  }
  TokenType op;
  while ((op = ctx->stream->tokens.iterator->type) == EQUAL || op == NOTEQ) {
    consume(ctx, op);
    ReturnValue right;
    if (!relational_expression(ctx, &right)) {
      token_stream_error(ctx->stream, "expected expression after '%s'", op == EQUAL ? "==" : "!=");
    }
    Type tDst;
    if (!arithmetic_type_to(&r->type, &right.type, &tDst)) {
      token_stream_error(ctx->stream, "invalid operand type for == or !=");
    }
    r->type          = (Type){TYPE_BASE_INT, NULL, -1};
    r->is_left_value = false;
    r->is_constant   = true;
  }
  return true;
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
//       =>  exprAdd ( ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd )*
bool relational_expression(ParserContext *ctx, ReturnValue *r) {
  if (!addition_expression(ctx, r)) {
    return false;
  }
  TokenType op;
  while ((op = ctx->stream->tokens.iterator->type) == LESS || op == LESSEQ ||
         op == GREATER || op == GREATEREQ) {
    consume(ctx, op);
    ReturnValue right;
    if (!addition_expression(ctx, &right)) {
      const char *sym =
          op == LESS ? "<" : op == LESSEQ ? "<=" : op == GREATER ? ">" : ">=";
      token_stream_error(ctx->stream, "expected expression after '%s'", sym);
    }
    Type tDst;
    if (!arithmetic_type_to(&r->type, &right.type, &tDst)) {
      token_stream_error(ctx->stream, "invalid operand type for <, <=, >, >=");
    }
    r->type          = (Type){TYPE_BASE_INT, NULL, -1};
    r->is_left_value = false;
    r->is_constant   = true;
  }
  return true;
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul  =>  exprMul ( ( ADD | SUB ) exprMul )*
bool addition_expression(ParserContext *ctx, ReturnValue *r) {
  if (!multiplication_expression(ctx, r)) {
    return false;
  }
  TokenType op;
  while ((op = ctx->stream->tokens.iterator->type) == ADD || op == SUB) {
    consume(ctx, op);
    ReturnValue right;
    if (!multiplication_expression(ctx, &right)) {
      token_stream_error(ctx->stream, "expected expression after + or -");
    }
    Type tDst;
    if (!arithmetic_type_to(&r->type, &right.type, &tDst)) {
      token_stream_error(ctx->stream, "invalid operand type for + or -");
    }
    r->type          = tDst;
    r->is_left_value = false;
    r->is_constant   = true;
  }
  return true;
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast  =>  exprCast ( ( MUL | DIV ) exprCast )*
bool multiplication_expression(ParserContext *ctx, ReturnValue *r) {
  if (!cast_expression(ctx, r)) {
    return false;
  }
  TokenType op;
  while ((op = ctx->stream->tokens.iterator->type) == MUL || op == DIV) {
    consume(ctx, op);
    ReturnValue right;
    if (!cast_expression(ctx, &right)) {
      token_stream_error(ctx->stream, "expected expression after * or /");
    }
    Type tDst;
    if (!arithmetic_type_to(&r->type, &right.type, &tDst)) {
      token_stream_error(ctx->stream, "invalid operand type for * or /");
    }
    r->type          = tDst;
    r->is_left_value = false;
    r->is_constant   = true;
  }
  return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool cast_expression(ParserContext *ctx, ReturnValue *r) {
  Token *start = ctx->stream->tokens.iterator;
  if (consume(ctx, LPAR)) {
    Type t;
    if (type_base(ctx, &t)) {
      array_declaration(ctx, &t); // optional
      if (!consume(ctx, RPAR)) {
        token_stream_error(ctx->stream, "expected ) in cast expression");
      }
      ReturnValue op;
      if (!cast_expression(ctx, &op)) {
        token_stream_error(ctx->stream, "expected expression after cast");
      }
      if (t.type_base == TYPE_BASE_STRUCT) {
        token_stream_error(ctx->stream, "cannot convert to a struct type");
      }
      if (op.type.type_base == TYPE_BASE_STRUCT) {
        token_stream_error(ctx->stream, "cannot convert a struct");
      }
      if (op.type.array_dimension >= 0 && t.array_dimension < 0) {
        token_stream_error(ctx->stream, "an array can be converted only to another array");
      }
      if (op.type.array_dimension < 0 && t.array_dimension >= 0) {
        token_stream_error(ctx->stream, "a scalar can be converted only to another scalar");
      }
      r->type          = t;
      r->is_left_value = false;
      r->is_constant   = true;
      return true;
    }
    ctx->stream->tokens.iterator = start; // backtrack: not a cast, try exprUnary
  }
  return unary_expression(ctx, r);
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool unary_expression(ParserContext *ctx, ReturnValue *r) {
  if (consume(ctx, SUB) || consume(ctx, NOT)) {
    if (!unary_expression(ctx, r)) {
      token_stream_error(ctx->stream, "expected expression after unary operator");
    }
    if (!can_be_scalar(r)) {
      token_stream_error(ctx->stream, "unary - or ! must have a scalar operand");
    }
    r->is_left_value = false;
    r->is_constant   = true;
    return true;
  }
  return postfix_expression(ctx, r);
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID |
// exprPrimary
//           =>  exprPrimary ( LBRACKET expr RBRACKET | DOT ID )*
bool postfix_expression(ParserContext *ctx, ReturnValue *r) {
  if (!primary_expression(ctx, r)) {
    return false;
  }
  for (;;) {
    if (consume(ctx, LBRACKET)) {
      ReturnValue idx;
      if (!expression(ctx, &idx)) {
        token_stream_error(ctx->stream, "expected expression in array index");
      }
      if (r->type.array_dimension < 0) {
        token_stream_error(ctx->stream, "only an array can be indexed");
      }
      if (idx.type.array_dimension >= 0 ||
          (idx.type.type_base != TYPE_BASE_INT && idx.type.type_base != TYPE_BASE_CHAR)) {
        token_stream_error(ctx->stream, "the index is not convertible to int");
      }
      if (!consume(ctx, RBRACKET)) {
        token_stream_error(ctx->stream, "expected ] after array index");
      }
      r->type.array_dimension = -1;
      r->is_left_value        = true;
      r->is_constant          = false;
    } else if (consume(ctx, DOT)) {
      if (!consume(ctx, ID)) {
        token_stream_error(ctx->stream, "expected identifier after .");
      }
      Token *tk_name = ctx->stream->tokens.consumed;
      if (r->type.type_base != TYPE_BASE_STRUCT) {
        token_stream_error(ctx->stream, "a field can only be selected from a struct");
      }
      Symbol *s = find_symbol_in_list(r->type.symbol->struct_members, tk_name->text);
      if (!s) {
        token_stream_error(ctx->stream, "the structure %s does not have a field %s",
                           r->type.symbol->name, tk_name->text);
      }
      r->type          = s->type;
      r->is_left_value = true;
      r->is_constant   = (r->type.array_dimension >= 0);
    } else {
      break;
    }
  }
  return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE |
// CHAR | STRING | LPAR expr RPAR
bool primary_expression(ParserContext *ctx, ReturnValue *r) {
  if (consume(ctx, ID)) {
    Token *tk_name = ctx->stream->tokens.consumed;
    Symbol *s      = find_symbol(ctx->domain_analyzer, tk_name->text);
    if (!s) {
      token_stream_error(ctx->stream, "undefined id: %s", tk_name->text);
    }
    if (consume(ctx, LPAR)) {
      if (s->kind != SYMBOL_KIND_FUNCTION) {
        token_stream_error(ctx->stream, "only a function can be called");
      }
      Symbol     *param = s->function.parameters;
      ReturnValue rArg;
      if (expression(ctx, &rArg)) {
        if (!param) {
          token_stream_error(ctx->stream, "too many arguments in function call");
        }
        if (!convert_to(&rArg.type, &param->type)) {
          token_stream_error(
              ctx->stream,
              "in call, cannot convert the argument type to the parameter type");
        }
        param = param->next;
        while (consume(ctx, COMMA)) {
          if (!expression(ctx, &rArg)) {
            token_stream_error(ctx->stream, "expected expression after ,");
          }
          if (!param) {
            token_stream_error(ctx->stream, "too many arguments in function call");
          }
          if (!convert_to(&rArg.type, &param->type)) {
            token_stream_error(
                ctx->stream,
                "in call, cannot convert the argument type to the parameter type");
          }
          param = param->next;
        }
      }
      if (!consume(ctx, RPAR)) {
        token_stream_error(ctx->stream, "expected ) after function call arguments");
      }
      if (param) {
        token_stream_error(ctx->stream, "too few arguments in function call");
      }
      r->type          = s->type;
      r->is_left_value = false;
      r->is_constant   = true;
    } else {
      if (s->kind == SYMBOL_KIND_FUNCTION) {
        token_stream_error(ctx->stream, "a function can only be called");
      }
      r->type          = s->type;
      r->is_left_value = true;
      r->is_constant   = (s->type.array_dimension >= 0);
    }
    return true;
  }

  if (consume(ctx, INT)) {
    r->type          = (Type){TYPE_BASE_INT, NULL, -1};
    r->is_left_value = false;
    r->is_constant   = true;
    return true;
  }
  if (consume(ctx, DOUBLE)) {
    r->type          = (Type){TYPE_BASE_DOUBLE, NULL, -1};
    r->is_left_value = false;
    r->is_constant   = true;
    return true;
  }
  if (consume(ctx, CHAR)) {
    r->type          = (Type){TYPE_BASE_CHAR, NULL, -1};
    r->is_left_value = false;
    r->is_constant   = true;
    return true;
  }
  if (consume(ctx, STRING)) {
    r->type          = (Type){TYPE_BASE_CHAR, NULL, 0};
    r->is_left_value = false;
    r->is_constant   = true;
    return true;
  }

  if (consume(ctx, LPAR)) {
    if (!expression(ctx, r)) {
      token_stream_error(ctx->stream, "expected expression after (");
    }
    if (!consume(ctx, RPAR)) {
      token_stream_error(ctx->stream, "expected ) after expression");
    }
    return true;
  }
  return false;
}
