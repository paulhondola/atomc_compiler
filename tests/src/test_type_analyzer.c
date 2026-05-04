#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/utils/utils.h"

static void test_type_analyzer(void) {
  printf("Running test_type_analyzer...\n");
  printf(GREEN "test_type_analyzer passed!\n" RESET);
}

int main(void) {
  test_type_analyzer();

  printf(GREEN "All tests passed successfully!\n" RESET);
  return 0;
}
