void f_check_struct_size(void* ps, void* pf, int ofst)
{
    if ((((char*)ps) + ofst) != ((char*)pf)) {
                printf("error\n");
        } else {
                printf("ok\n");
    }
}
 
 
struct __attribute__ ((aligned(2))) A {
   char a ;
   int b;
   short c;;
};
 
struct B {
  short s __attribute__ ((aligned(2), packed));
  double d __attribute__ ((aligned(2), packed));
};
 
struct __attribute__ ((aligned(8))) C {
   char a;     // offset 0
   long long b; // offset 4
   short c;; // offset 12
};// size  16; align 8
struct __attribute__ ((aligned(2))) D {
   char a;     // offset 0
   long long b; // offset 4
   short c;; // offset 12
};// size  16; align 4
 
struct __attribute__ ((aligned(8), __packed__)) E {
   char a;     // offset 0
   long long b; // offset 1
   short c; // offset 9
};// size  16; align 8
struct __attribute__ ((aligned(2), __packed__)) F {
   char a;     // offset 0
   long long b; // offset 1
   short c;; // offset 9
}; // size  12; align 2
struct __attribute__ ((aligned(1), __packed__)) G {
   char a;     // offset 0
   long long b; // offset 1
   short c;; // offset 9
};
 
int main() {
   struct A a[2];
   struct B b[2];
   struct C c[2];
   struct D d[2];
   struct E e[2];
   struct F f[2];
   struct G g[2];
 
   f_check_struct_size(&a[0], &a[1], 12);
   f_check_struct_size(&b[0], &b[1], 10);
   f_check_struct_size(&c[0], &c[1], 24);
   f_check_struct_size(&d[0], &d[1], 24);
   f_check_struct_size(&e[0], &e[1], 16);
   f_check_struct_size(&f[0], &f[1], 12);
   f_check_struct_size(&g[0], &g[1], 11);
}