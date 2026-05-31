struct S {
  int v;
};

// ERROR: index not convertible to int — double
void err_double_index() {
  int arr[5];
  int d;
  int x;
  x = arr[d];
}

// ERROR: index not convertible to int — array
void err_array_index() {
  int arr[5];
  int idx;
  int x;
  x = arr[idx];
}

// ERROR: index not convertible to int — struct
void err_struct_index() {
  int      arr[5];
  struct S s;
  int      x;
  x = arr[s];
}
