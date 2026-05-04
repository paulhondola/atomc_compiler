#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/utils/utils.h"

static void test_vm(void) {
  printf("Running test_vm...\n");
  printf(GREEN "test_vm passed!\n" RESET);
}

int main(void) {
  test_vm();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
