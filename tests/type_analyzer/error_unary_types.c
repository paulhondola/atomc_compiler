struct S {
  int v;
};

// ERROR: unary - requires scalar — array
void err_unary_minus_array() {
  int arr[3];
  int x;
  x = -arr;
}

// ERROR: unary ! requires scalar — array
void err_unary_not_array() {
  int arr[3];
  int x;
  x = !arr;
}

// ERROR: unary - requires scalar — struct
void err_unary_minus_struct() {
  struct S s;
  int      x;
  x = -s;
}

// ERROR: unary ! requires scalar — struct
void err_unary_not_struct() {
  struct S s;
  int      x;
  x = !s;
}
