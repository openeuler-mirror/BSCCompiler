#include <stdio.h>
#define bool _Bool
#define true 1
#define false 0
// lower and upper of value ranges(var a and var b) are constant
// primitive type of two vrs are equal
// can opt the condition
bool func1(int a, int b) { // test 9, 25
  if (a > 5 && a < 10 && b > 20 && b < 30) {
    // CHECK-NOT: cmp
    if (a > b) {
      return true;
    }
  }
  return false;
}
// lower and upper of value ranges(var a and var b) are constant
// primitive type of two vrs are not equal
// can not opt the condition
bool func2(int a, long b) { // test: 9, 0x100000000 
  if (a > 5 && a < 10 && b > 0xffffffff) {
    // CHECK: cmp
    if (a > b) {
      return true;
    }
  }
  return false;
}

// the value range of a is constant, and the value range of b is not constant
// can not opt the condition
bool func3(int a, int b, int c) { //test: 0x7fffffff, 100
  if (a == 100 && b > c) {
    // CHECK: cmp
    if (a > b) {
      return true;
    }
  }
  return false;
}

// the value ranges of a and b are not constant, can not opt the condition
bool func4(int a, int b, int c) { // test: 4, 5, 10
  if (a < c && b < c) {
    // CHECK: cmp
    if (a < b) {
      return true; 
    }
  }
  return false;
}


// the value ranges of a and b are not constant, and the value ranges are equal, can opt the condtition
bool func5(int a, int b, int c) {
  if (a == c+10 && b == c+10) {
    // CHECK-NOT: cmp
    if (a == b) {
      return true;
    } 
  }
  return false;
}

// the primtive types of a and b are not equal, can not opt the condtition
bool func6(int a, long b, int c) {
  if (a == c+10 && b == c+10) {
    // CHECK: cmp
    if (a == b) {
      return true;
    }
  }
  return false;
}

// the value range of a is min, and the value range of b is not constant, can opt the condition
bool func7(int a, int b, int c) {
  if (a == 0xffffffff && b < c) {
    // CHECK-NOT: cmp
    if (a < b) {
      return true;
    }
  }
  return false;
}

// the value range of a is min, and the value range of b is not constant,
// the primtive types of a and b are not equal, can not opt the condition
bool func8(int a, long b, int c) {
  if (a == 0xffffffff && b < c) {
    // CHECK: cmp
    if (a < b) {
      return true;
    }
  }
  return false;
}

// the value range of a is max, and the value range of b is not constant,
// the primtive types of a and b are not equal, can not opt the condition
bool func9(int a, int b, int c) {
  if (a == 0x7fffffff && b < c) {
    // CHECK-NOT: cmp
    if (b < a) {
      return true;
    }
  }
  return false;
}

// the value range of a is max, and the value range of b is not constant,
// the primtive types of a and b are not equal, can not opt the condition
bool func10(int a, long b, int c) {
  if (a == 0x7fffffff && b < c) {
    // CHECK: cmp
    if (b < a) {
      return true;
    }
  }
  return false;
}

int main() {
  return 0;
}
