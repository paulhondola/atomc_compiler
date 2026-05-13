#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argtable3.h>

#include "../include/analyzer/domain.h"
#include "../include/analyzer/domain_analyzer.h"
#include "../include/analyzer/symbol.h"
#include "../include/frontend/lexer.h"
#include "../include/frontend/parser.h"
#include "../include/utils/utils.h"
#include "../include/vm/instruction.h"
#include "../include/vm/vm.h"

int main(int argc, char *argv[]) {
  struct arg_file *source_code_file;
  struct arg_file *tokens_file;
  struct arg_file *domain_file;
  struct arg_file *vm_file;
  struct arg_lit  *help;
  struct arg_end  *end;

  void *argtable[] = {
      help             = arg_lit0(NULL, "help", "print this help and exit"),
      tokens_file      = arg_file0("t", "tokens", "<file>", "Output token stream to <file>"),
      domain_file      = arg_file0("d", "domain", "<file>", "Output domain analyzer info to <file>"),
      vm_file          = arg_file0("v", "vm", "<file>", "Run VM and redirect output to <file>"),
      source_code_file = arg_file1(NULL, NULL, "<file>", "Source code file"),
      end              = arg_end(20),
  };

  const char *progname = "atomcc";

  if (arg_nullcheck(argtable) != 0) {
    fprintf(stderr, "%s: insufficient memory\n", progname);
    return 1;
  }

  int nerrors = arg_parse(argc, argv, argtable);

  if (help->count > 0) {
    printf("Usage: %s", progname);
    arg_print_syntax(stdout, argtable, "\n");
    printf("AtomC-Compiler -- A compiler for the AtomC language.\n\n");
    arg_print_glossary(stdout, argtable, "  %-25s %s\n");
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 0;
  }

  if (nerrors > 0) {
    arg_print_errors(stderr, end, progname);
    fprintf(stderr, "Try '%s --help' for more information.\n", progname);
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 1;
  }

  const char *source_code_path  = source_code_file->filename[0];
  const char *token_output_path = tokens_file->count > 0 ? tokens_file->filename[0] : NULL;
  const char *domain_output_path = domain_file->count > 0 ? domain_file->filename[0] : NULL;
  const char *vm_output_path     = vm_file->count > 0 ? vm_file->filename[0] : NULL;

  FILE *token_out = stdout;
  if (token_output_path) {
    token_out = fopen(token_output_path, "w");
    if (token_out == NULL) {
      err("cannot open tokens output file %s", token_output_path);
    }
  }

  FILE *domain_out = stdout;
  if (domain_output_path) {
    domain_out = fopen(domain_output_path, "w");
    if (domain_out == NULL) {
      err("cannot open domain output file %s", domain_output_path);
    }
  }

  FILE *vm_out = stdout;
  if (vm_output_path) {
    vm_out = fopen(vm_output_path, "w");
    if (vm_out == NULL) {
      err("cannot open VM output file %s", vm_output_path);
    }
  }

  const char *src_string = load_file(source_code_path);

  TokenStream    token_stream;
  DomainAnalyzer domain_analyzer;
  token_stream_init(&token_stream);
  domain_analyzer_init(&domain_analyzer);

  token_stream.output    = token_out;
  domain_analyzer.output = domain_out;

  tokenize(&token_stream, src_string);
  token_stream_show(&token_stream);

  push_domain(&domain_analyzer);
  // Register host-side externs (put_int/put_double) in the global scope BEFORE
  // parsing so AtomC source can call them as ordinary functions. The parser's
  // primary-expression action emits OP_CALL_EXT when a symbol carries a
  // non-null external_function_pointer.
  vm_init(&domain_analyzer);
  parse(&token_stream, &domain_analyzer);
  show_domain(&domain_analyzer, "global");

  if (vm_file->count > 0) {
    Symbol *sym_main =
        find_symbol_in_domain(domain_analyzer.symbol_table, "main");
    if (!sym_main || sym_main->kind != SYMBOL_KIND_FUNCTION) {
      err("missing main function");
    }
    // Build the entry stub: CALL main; HALT. This is the only code path that
    // gets executed at start-up — the spec mandates programs begin at main().
    Instruction *entry_code = NULL;
    add_instruction(&entry_code, OP_CALL)->argument.instruction_pointer =
        sym_main->function.instruction;
    add_instruction(&entry_code, OP_HALT);

    VirtualMachine vm;
    vm_create(&vm);
    vm.output = vm_out;
    run(&vm, entry_code);

    delete_instruction(entry_code);
  }

  drop_domain(&domain_analyzer);

  if (token_out != stdout) {
    fclose(token_out);
  }

  if (domain_out != stdout) {
    fclose(domain_out);
  }

  if (vm_out != stdout) {
    fclose(vm_out);
  }

  free((void *)src_string);
  token_stream_free(&token_stream);
  domain_analyzer_free(&domain_analyzer);

  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

  printf(GREEN "Parsing finished successfully!" RESET "\n");

  return 0;
}
