#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/analyzer/domain_analyzer.h"
#include "../../include/analyzer/symbol.h"
#include "../../include/analyzer/type.h"
#include "../../include/utils/utils.h"
#include "../../include/vm/instruction.h"
#include "../../include/vm/vm.h"

// Read a tmpfile back into a heap-allocated NUL-terminated string.
// Caller frees. Returns NULL on read failure.
static char *slurp_tmpfile(FILE *f) {
  fflush(f);
  if (fseek(f, 0L, SEEK_END) != 0)
    return NULL;
  long size = ftell(f);
  if (size < 0)
    return NULL;
  rewind(f);
  char *buf = (char *)malloc((size_t)size + 1);
  if (!buf)
    return NULL;
  size_t read = fread(buf, 1, (size_t)size, f);
  buf[read]   = '\0';
  return buf;
}

int main() {
  printf("Running test_run_double_program...\n");

  DomainAnalyzer da;
  domain_analyzer_init(&da);
  push_domain(&da);
  vm_init(&da);

  Instruction *code = gen_test_program_double(&da);

  FILE *tmp = tmpfile();

  VirtualMachine vm;
  vm_create(&vm);
  vm.output = tmp;

  run(&vm, code);

  char *trace = slurp_tmpfile(tmp);
  assert(trace != NULL);
  fputs(trace, stdout);

  // Loop iterates for i = 0.0, 0.5, 1.0, 1.5 (since n = 2.0, step = 0.5)
  assert(strstr(trace, "=> 0") != NULL);
  printf(GREEN "Test 1 (trace contains \"=> 0\") passed\n" RESET);

  assert(strstr(trace, "=> 0.5") != NULL);
  printf(GREEN "Test 2 (trace contains \"=> 0.5\") passed\n" RESET);

  assert(strstr(trace, "=> 1") != NULL);
  printf(GREEN "Test 3 (trace contains \"=> 1\") passed\n" RESET);

  assert(strstr(trace, "=> 1.5") != NULL);
  printf(GREEN "Test 4 (trace contains \"=> 1.5\") passed\n" RESET);

  // No "=> 2" — loop exits when i == n
  // Use \n boundary so this doesn't match a substring of "=> 2.0..." etc.
  assert(strstr(trace, "=> 2\n") == NULL);
  printf(GREEN "Test 5 (no \"=> 2\" — loop exits at i==n) passed\n" RESET);

  // Float opcodes must appear in the trace (they're newly implemented)
  assert(strstr(trace, "PUSH.f") != NULL);
  printf(GREEN "Test 6 (PUSH.f appears in trace) passed\n" RESET);

  assert(strstr(trace, "ADD.f") != NULL);
  printf(GREEN "Test 7 (ADD.f appears in trace) passed\n" RESET);

  assert(strstr(trace, "LESS.f") != NULL);
  printf(GREEN "Test 8 (LESS.f appears in trace) passed\n" RESET);

  assert(strstr(trace, "HALT") != NULL);
  printf(GREEN "Test 9 (trace ends with HALT) passed\n" RESET);

  free(trace);
  fclose(tmp);
  drop_domain(&da);
  domain_analyzer_free(&da);
  printf(GREEN "test_run_double_program passed!\n" RESET);
}