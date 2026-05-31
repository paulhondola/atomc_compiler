struct S {
  int v;
};

// ERROR: if condition must be scalar — array
void err_if_array() {
  int arr[3];
  if (arr) {
  }
}

// ERROR: if condition must be scalar — struct
void err_if_struct() {
  struct S s;
  if (s) {
  }
}

// ERROR: while condition must be scalar — array
void err_while_array() {
  int arr[3];
  while (arr) {
  }
}

// ERROR: while condition must be scalar — struct
void err_while_struct() {
  struct S s;
  while (s) {
  }
}
