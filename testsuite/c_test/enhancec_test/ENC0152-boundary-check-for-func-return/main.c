// CHECK: [[# FILENUM:]] "{{.*}}/main.c"

int q[10] = {0};
// CHECK: dassign %_boundary.return.size 0 (dread i32 %m)
__attribute__((returns_byte_count_index(2)))
int *func2(int n, int m) {
  if (n == 0) {
    // CHECK: LOC [[# FILENUM]] 12
    // CHECK-NEXT: returnassertle <&func2> (
    // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
    // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
    return q;
  } else {
    if (n == 1) {
      // CHECK: LOC [[# FILENUM]] 19
      // CHECK-NEXT: returnassertle <&func2> (
      // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
      // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
      return q;
    } else {
      // CHECK: LOC [[# FILENUM]] 25
      // CHECK-NEXT: returnassertle <&func2> (
      // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
      // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
      return q;
    }
  }
  for (int i = 0; i < n; i++) {
    // CHECK: LOC [[# FILENUM]] 33
    // CHECK-NEXT: returnassertle <&func2> (
    // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
    // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
    return q;
  }
  switch(n) {
    case 1:{
      if (n == 2) {
        // CHECK: LOC [[# FILENUM]] 42
        // CHECK-NEXT: returnassertle <&func2> (
        // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
        // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
        return q;
      }
      // CHECK: LOC [[# FILENUM]] 48
      // CHECK-NEXT: returnassertle <&func2> (
      // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
      // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
      return q;
      break;
   }
   default:
     // CHECK: LOC [[# FILENUM]] 56
     // CHECK-NEXT: returnassertle <&func2> (
     // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
     // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
     return q;
  }
  while (n) {
    for (;;) {
      if (n == 1) {
        // CHECK: LOC [[# FILENUM]] 65
        // CHECK-NEXT: returnassertle <&func2> (
        // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
        // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
        return q;
      }
      // CHECK: LOC [[# FILENUM]] 71
      // CHECK-NEXT: returnassertle <&func2> (
      // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
      // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
      return q;
    }
    // CHECK: LOC [[# FILENUM]] 77
    // CHECK-NEXT: returnassertle <&func2> (
    // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
    // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
    return q;
  }
  do {
    switch(n) {
      case 1:{
        if (n == 2) {
          // CHECK: LOC [[# FILENUM]] 87
          // CHECK-NEXT: returnassertle <&func2> (
          // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
          // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
          return q;
        }
        // CHECK: LOC [[# FILENUM]] 93
        // CHECK-NEXT: returnassertle <&func2> (
        // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
        // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
        return q;
        break;
      }
      default:
        // CHECK: LOC [[# FILENUM]] 101
        // CHECK-NEXT: returnassertle <&func2> (
        // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
        // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
        return q;
     }
    // CHECK: LOC [[# FILENUM]] 107
    // CHECK-NEXT: returnassertle <&func2> (
    // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
    // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
    return q;
  } while(n);
  // CHECK: LOC [[# FILENUM]] 113
  // CHECK-NEXT: returnassertle <&func2> (
  // CHECK-NEXT: add ptr (addrof ptr $q, dread i32 %_boundary.return.size),
  // CHECK-NEXT: add ptr (addrof ptr $q, constval ptr 40))
  return q;
}

int main() {
  return 0;
}
