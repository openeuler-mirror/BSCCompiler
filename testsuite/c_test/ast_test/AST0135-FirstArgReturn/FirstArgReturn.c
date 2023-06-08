struct S{
  long a[4];
};
typedef struct S S;

struct S1 {
  int a;
  // CHECK: @fp1 <* <func firstarg_return (<* <$S>>,u8) void>>
  // CHECK-NEXT: @fp2 <[2] <* <func firstarg_return (<* <$S>>,u8) void>>>
  S(*fp1)(char);
  S(*fp2[2])(char);
  int b;
};

union S2 {
  int a;
  // CHECK: @fp1 <* <func firstarg_return (<* <$S>>,u8) void>>
  // CHECK-NEXT: @fp2 <[2] <* <func firstarg_return (<* <$S>>,u8) void>>>
  S(*fp1)(char);
  S(*fp2[2])(char);
  int b;
};

// CHECK: func &f1 public firstarg_return (var %first_arg_return <* <$S>>, var %a u8) void
S f1(char a) {
  S s;
  return s;
}

// CHECK: var $fp1 <* <func firstarg_return (<* <$S>>,u8) void>>
S(*fp1)(char);
// CEHCK: var $fp2 <[2] <* <func firstarg_return (<* <$S>>,u8) void>>>
S(*fp2[2])(char);

int main() {
  // CHECK: var %fp3_{{[0-9_]+}} <* <func firstarg_return (<* <$S>>,u8) void>>
  S(*fp3)(char);
  // CHECK: var %fp4_{{[0-9_]+}} <[2] <* <func firstarg_return (<* <$S>>,u8) void>>>
  S(*fp4[2])(char);
  return 0;
}
