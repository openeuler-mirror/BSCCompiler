 #include <stdio.h>
 struct sd {
     int a;
     int b;
     int c;
 };
 int te1(int a, int b, int c, int d, struct sd ml){
     int m1 = ml.a + ml.b + ml.c;
     ml.b = b;
     ml.c = 3;
     printf("%d\n", ml.c);
     return m1;
 }

 struct sc {
     int a;
     int b;
     int c;
     int d;
     int e;
 };
 int te(struct sc ml, int a, int b, int c, int d, int e, int f, int g, int h, int i){
     int m = ml.e;
     ml.b = b;
     ml.c = 3;
     ml.d = 4;
     ml.e = 5;
     printf("%d\n", ml.d);
     return m;
 }
 int main() {
     struct sd mn;
     mn.a = 10;
     mn.b = 9;
     mn.c = 8;
     struct sc mm;
     mm.a = 7;
     mm.b = 8;
     mm.c = 9;
     mm.d = 10;
     mm.e = 11;
     int m  = te(mm, 1, 2, 3, 4, 5, 6, 7, 8, 9);
     printf("%d\n", m);
     int n = te1(1, 2, 3, 4, mn);
     printf("%d\n", n);
     return 0;
 }
