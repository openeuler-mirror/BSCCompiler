#include <stdint.h>
__attribute__((noinline)) int foo(uint64_t a){
  return (int)((a << 32) & 0xffffffffful) == 0;
}

int main(){
  if(!foo(0xffffffffful)){
    __builtin_abort();
  }
}
