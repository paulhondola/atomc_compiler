#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/utils/utils.h"

static void test_code_generator(void) {
  printf("Running test_code_generator...\n");
  printf(GREEN "test_code_generator passed!\n" RESET);
}

int main(void) {
  test_code_generator();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
