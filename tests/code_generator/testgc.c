// Code-generation acceptance test (AtomC source).
// Each `put_*` call's expected output is in the trailing comment so a human
// (and the test harness) can verify by inspection.
//
// Coverage:
//   - locals and parameters (int + double)
//   - while loop with comparison and i<int> increment
//   - if/else
//   - implicit int→double promotion on arg passing
//   - return values and the OP_RET frame teardown

void count_ints(int n) {
  int i;
  i = 0;
  while (i < n) {
    put_int(i); // => 0, => 1, => 2
    i = i + 1;
  }
}

void count_doubles(double n) {
  double i;
  i = 0.0;
  while (i < n) {
    put_double(i); // => 0, => 0.5, => 1, => 1.5
    i = i + 0.5;
  }
}

int triple(int x) {
  return x * 3;
}

void main() {
  count_ints(3);
  // mixed int/double: 4 promotes to double at the call boundary
  count_doubles(2.0);
  put_int(triple(7)); // => 21

  int a;
  a = 10;
  if (a < 20) {
    put_int(1); // => 1   (then-branch taken)
  } else {
    put_int(0);
  }
}
