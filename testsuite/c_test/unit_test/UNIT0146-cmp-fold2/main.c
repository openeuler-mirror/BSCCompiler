#define ABORT __builtin_abort
__attribute((noinline)) int foo(int a){
  return (a < 8) || (a > 0x1ffffff) ? 0 : 1;
}

unsigned int b = 0;
signed char e = 0;

static long fn2(long p1, unsigned int p2) {
  return (p1 < 0) || (p1 > 0x1ffffffffffffff) ? 0 : p1 << p2;
}

int main(){
  if(!foo(10)){
    ABORT();
  }
  if(foo(-1)){
    ABORT();
  }
  if(fn2((1 == e) != (b || 3), 6) != 64){
    ABORT();
  }
}
