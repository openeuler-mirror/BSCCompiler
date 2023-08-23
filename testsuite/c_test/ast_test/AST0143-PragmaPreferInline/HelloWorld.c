// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"

// CHECK: LOC {{.*}} 147 5
// CHECK-NEXT: func &main public () i32
// CHECK-NEXT: pragma !1 prefer_inline "ON"
// CHECK-NEXT: pragma !2 prefer_inline "OFF"
struct A {
  int a;
  int b;
};

int outScope(int a) {
  return a - 1;
}

int inScope(int a) {
  return a + 1;
}

int funcA() {
  int d = 0;
  #pragma prefer_inline ON
  // CHECK: LOC {{.*}} 27 7
  // CHECK-NEXT:   callassigned &inScope (constval i32 1) { dassign %retVar_3 0 } !1
  // CHECK: LOC {{.*}} 27 19
  // CHECK-NEXT:   callassigned &outScope (constval i32 1) { dassign %retVar_4 0 }
  d = inScope(1); outScope(1);
  #pragma prefer_inline OFF
  {
    #pragma prefer_inline ON
    // CHECK: LOC {{.*}} 33 13
    // CHECK-NEXT:  callassigned &inScope (constval i32 2) { dassign %retVar_5 0 } !1 
    int e = inScope(2);
    // CHECK: LOC {{.*}} 36 5
    // CHECK-NEXT:   callassigned &outScope (constval i32 2) { dassign %retVar_6 0 }
    outScope(2);
  }
  #pragma prefer_inline ON
  // CHECK: LOC {{.*}} 41 9
  // CHECK-NEXT:   callassigned &inScope (constval i32 3) { dassign %retVar_7 0 } !1
  (long)inScope(3);
  #pragma prefer_inline OFF
  // CHECK: LOC {{.*}} 47 19
  // CHECK-NEXT:   callassigned &inScope (constval i32 4) { dassign %retVar_9 0 } !2
  // CHECK: LOC {{.*}} 47 11
  // CHECK-NEXT:   callassigned &inScope (dread i32 %retVar_9) { dassign %retVar_8 0 } !2
  int c = inScope(inScope(4));
  #pragma prefer_inline ON
  {
    // CHECK: LOC {{.*}} 52 13
    // CHECK-NEXT:   callassigned &outScope (constval i32 5) { dassign %retVar_10 0 }
    int f = outScope(5);
    // CHECK: LOC {{.*}} 55 5
    // CHECK-NEXT:   callassigned &outScope (constval i32 5) { dassign %retVar_11 0 }
    outScope(5);
  }
  #pragma prefer_inline OFF
  // CHECK: LOC {{.*}} 60 9
  // CHECK-NEXT:   callassigned &inScope (constval i32 6) { dassign %retVar_12 0 } !2
  while(inScope(6)) {
    // CHECK: LOC {{.*}} 63 5
    // CHECK-NEXT:     callassigned &outScope (constval i32 6) { dassign %retVar_13 0 }
    outScope(6);
  }
  #pragma prefer_inline ON
  // CHECK: LOC {{.*}} 70 15
  // CHECK-NEXT:   callassigned &inScope (constval i32 7) { dassign %retVar_14 0 } !1
  // CHECK: LOC {{.*}} 70 27
  // CHECK-NEXT:   callassigned &inScope (constval i32 8) { dassign %retVar_15 0 } !1
  int b[2] = {inScope(7), inScope(8)};
  #pragma prefer_inline OFF
  struct A Aa = {
    // CHECK: LOC {{.*}} 77 13
    // CHECK-NEXT:   callassigned &inScope (constval i32 8) { dassign %retVar_17 0 } !2
    // CHECK: LOC {{.*}} 78 5
    // CHECK-NEXT:   callassigned &inScope (constval i32 9) { dassign %retVar_18 0 } !2
    inScope(inScope(8)),
    inScope(9)
  };
  #pragma prefer_inline ON
  // CHECK: LOC {{.*}} 83 10
  // CHECK-NEXT:   callassigned &inScope (constval i32 10) { dassign %retVar_19 0 } !1
  switch(inScope(10)) {
    case 1:
      // CHECK: LOC {{.*}} 87 7
      // CHECK:   callassigned &outScope (constval i32 10) { dassign %retVar_20 0 }
      outScope(10);
      break;
    case 2:
      break;
    default:
      break;
  }
  #pragma prefer_inline OFF
  do {
    // CHECK: LOC {{.*}} 98 5
    // CHECK-NEXT:     callassigned &outScope (constval i32 11) { dassign %retVar_22 0 }
    outScope(11);
    // CHECK: LOC {{.*}} 101 11
    // CHECK-NEXT:     callassigned &inScope (constval i32 11) { dassign %retVar_21 0 } !2
  } while(inScope(11));
  #pragma prefer_inline ON
  do {
    #pragma prefer_inline OFF
    // CHECK: LOC {{.*}} 107 5
    // CHECK-NEXT:     callassigned &inScope (constval i32 12) { dassign %retVar_24 0 } !2
    inScope(12);
    // CHECK: LOC {{.*}} 110 11
    // CHECK-NEXT:     callassigned &inScope (constval i32 12) { dassign %retVar_23 0 } !1
  } while(inScope(12));
  #pragma prefer_inline ON
  // CHECK: LOC {{.*}} 114 7
  // CHECK-NEXT:   callassigned &inScope (constval i32 13) { dassign %retVar_25 0 } !1
  if (inScope(13)) {
    // CHECK: LOC {{.*}} 117 5
    // CHECK-NEXT:     callassigned &outScope (constval i32 13) { dassign %retVar_26 0 }
    outScope(13);
  }
  #pragma prefer_inline OFF
  // CHECK: LOC {{.*}} 122 7
  // CHECK-NEXT:   callassigned &inScope (constval i32 14) { dassign %retVar_27 0 } !2
  if (inScope(14)) {
    // CHECK: LOC {{.*}} 125 5
    // CHECK-NEXT:     callassigned &outScope (constval i32 14) { dassign %retVar_28 0 }
    outScope(14);
    // CHECK: LOC {{.*}} 128 14
    // CHECK-NEXT:     callassigned &outScope (constval i32 15) { dassign %retVar_29 0 }
  } else if (outScope(15)) {
    #pragma prefer_inline ON
    // CHECK: LOC {{.*}} 132 5
    // CHECK-NEXT:       callassigned &inScope (constval i32 15) { dassign %retVar_30 0 } !1
    inScope(15);
  }
  #pragma prefer_inline OFF
  // CHECK: LOC {{.*}} 139 7
  // CHECK-NEXT:   callassigned &inScope (constval i32 16) { dassign %retVar_33 0 } !2
  // CHECK: LOC {{.*}} 139 19
  // CHECK:   callassigned &inScope (constval i32 17) { dassign %retVar_32 0 } !2
  if (inScope(16) || inScope(17)) {
    // CHECK: LOC {{.*}} 142 5
    // CHECK-NEXT:     callassigned &outScope (constval i32 16) { dassign %retVar_34 0 }
    outScope(16);
  }
  return 0;
}

int main() {
  #pragma prefer_inline OFF
  // CHECK: LOC {{.*}} 151 10
  // CHECK-NEXT:   callassigned &funcA () { dassign %retVar_35 0 } !2
  return funcA();
}