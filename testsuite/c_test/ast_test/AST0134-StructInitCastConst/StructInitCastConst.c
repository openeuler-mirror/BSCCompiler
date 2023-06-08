// CHECK: type $S <struct {
// CHECK-NEXT:  @x <* i32>
// CHECK: type $A <struct {
// CHECK-NEXT:  @y <* i32> const

// CHECK: var $s <$S> = [1= addrof ptr $cle.2]
// CHECK-NEXT: $cle.2 <[3] i32> const = [1, 2, 3]
struct S { const int *x; } s = { (const int[]){1, 2, 3} };

// CHECK: var $a <$A> = [1= addrof ptr $cle.4]
// CHECK-NEXT: var $cle.4 <[3] i32> const = [1, 2, 3]
struct A { const int *const y; } a = { (const int[]){1, 2, 3} };

// CHECK: var $b <[3] i32> = [1, 2, 3]
int b[] = { 1, 2, 3 };

// CHECK: var $y <* i32> = addrof ptr $cle.6
// CHECK-NEXT: var $cle.6 <[3] i32> const = [1, 2, 3]
const int *y = (const int[]){1, 2, 3};

// CHECK: var $z <* <* i32>> = addrof ptr $cle.9
// CHECK-NEXT: var $cle.9 <[1] <* i32>> = [addrof ptr $cle.11]
// CHECK-NEXT: var $cle.11 <[3] i32> const = [1, 2, 3]
const int **z = (const int *[]){(const int[]) {1, 2, 3}};

// CHECK: var $arr <[1] <* i32>> = [addrof ptr $cle.13]
// CHECK-NEXT: var $cle.13 <[3] i32> const = [1, 2, 3]
const int *arr[] = { (const int[]){1, 2, 3} };

// CHECK: var $q i32 const = 8
const int q = sizeof((struct S){ (const int[]){1, 2, 3} });

// CHECK: var $u <* i32> = addrof ptr $cle.16
// CHECK-NEXT: var $cle.16 <[3] i32> = [1, 2, 3]
const int *u = (typeof((const int[]){1, 2, 3})){1, 2, 3};

// CHECK: var $v <$S> = [1= addrof ptr $cle.20]
// CHECK-NEXT: var $cle.20 <[3] i32> const = [1, 2, 3]
struct S v = (typeof((struct S){(const int[]){1, 2, 3}})){(const int[]){1, 2, 3}};

// CHECK: var $z1 <* <* i32>> const = addrof ptr $cle.23
// CHECK-NEXT: var $cle.23 <[1] <* i32>> = [addrof ptr $cle.25]
// CHECK-NEXT: var $cle.25 <[3] i32> const = [1, 2, 3]
const int ** const z1 = (const int *[]){(const int[]) {1, 2, 3}};

// CHECK: var $z2 <* <* i32>> const = addrof ptr $cle.30
// CHECK-NEXT: var $cle.30 <[1] <* i32>> = [addrof ptr $cle.32]
// CHECK-NEXT: var $cle.32 <[3] i32> const = [1, 2, 3]
const int *const * const z2 = (const int *[]){(const int[]) {1, 2, 3}};

// CHECK: var $z3 <* <* i32>> const = addrof ptr $cle.37
// CHECK-NEXT: var $cle.37 <[1] <* i32>> const = [addrof ptr $cle.39]
// CHECK-NEXT: var $cle.39 <[3] i32> const = [1, 2, 3]
const int ** const z3 = (const int *const[]){(const int[]) {1, 2, 3}};

int main() {
  return 0;
}
