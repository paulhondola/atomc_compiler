struct A {
  int v;
};
struct B {
  int v;
};

void g_scalar(int x) {
}
void g_struct_a(struct A a) {
}

// ERROR: cannot convert argument — array to scalar param
void err_array_to_scalar_param() {
  int arr[3];
  g_scalar(arr);
}

// ERROR: cannot convert argument — struct B passed to struct A param
void err_wrong_struct_type() {
  struct B b;
  g_struct_a(b);
}
