__attribute__((noinline)) int foo(__attribute__((unused)) int a){
  return 1;
}

int main(){
  // CHECK: {{^ mov w0, #3}}
  int a = 3;
  return foo(a);
}
