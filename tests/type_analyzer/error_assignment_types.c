struct A {
  int v;
};
struct B {
  int v;
};

// ERROR: assign to constant
void err_assign_constant() {
  int a[3];
  int b[3];
  b = a;
}

// ERROR: assign destination must be scalar — struct on left
void err_assign_struct_scalar() {
  struct A s;
  int      i;
  s = i;
}

// ERROR: incompatible struct types
void err_assign_different_structs() {
  struct A a;
  struct B b;
  a = b;
}

// ERROR: array variable cannot be assign destination
void err_assign_to_array() {
  int arr[5];
  int x;
  arr = x;
}
