struct S {
  int v;
};

void g() {
}

// ERROR: invalid operand for + — array as left operand
void err_array_plus_int() {
  int arr[3];
  int x;
  x = arr + 1;
}

// ERROR: invalid operand for + — array as right operand
void err_int_plus_array() {
  int arr[3];
  int x;
  x = 1 + arr;
}

// ERROR: invalid operand for + — struct as left operand
void err_struct_plus_int() {
  struct S s;
  int      x;
  x = s + 1;
}

// ERROR: invalid operand for - — struct as right operand
void err_int_minus_struct() {
  struct S s;
  int      x;
  x = 1 - s;
}

// ERROR: invalid operand for * — struct operands
void err_struct_mul_struct() {
  struct S a;
  struct S b;
  int      x;
  x = a * b;
}

// ERROR: invalid operand for < — array in relational
void err_array_relational() {
  int arr[3];
  int x;
  x = arr < 1;
}

// ERROR: invalid operand for == — struct in equality
void err_struct_equality() {
  struct S s;
  int      x;
  x = s == 1;
}

// ERROR: invalid operand for + — void function result
void err_void_plus_int() {
  int x;
  x = g() + 1;
}
