 #include <stdio.h>
 struct sc {
     int a;
     int b;
     int c;
 };

 struct sc teSmall(int a, int b, int c, int d, int e, int f, int g, struct sc ml){
     int m1 = ml.c;
     ml.a = f;
     ml.b = b;
     ml.c = 3;
     printf("%d\n", ml.c);
     return ml;
 }

 struct sd {
     int a;
     int b;
     int c;
     int d;
     int e;
 };
 struct sd teLarge(int a, int b){
     struct sd ml;
     ml.a = a;
     ml.b = b;
     ml.c = 3;
     ml.d = 4;
     ml.e = 5;
     return ml;
 }

 int main() {
     struct sd larg  = teLarge(1, 2);
     printf("%d\n", larg.a + larg.b + larg.c + larg.d + larg.e);
     struct sc teSmal;
     teSmal.a = 7;
     teSmal.b = 8;
     teSmal.c = 9;
     struct sc smal = teSmall(1, 2, 3, 4, 5, 6, 7, teSmal);
     printf("%d\n", smal.a + smal.b + smal.c);
     return 0;
  }
