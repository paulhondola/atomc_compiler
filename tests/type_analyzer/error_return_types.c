struct S {
  int v;
};

// ERROR: return value must be scalar — array
int err_return_array() {
  int arr[3];
  return arr;
}

// ERROR: return value must be scalar — struct
int err_return_struct() {
  struct S s;
  return s;
}
