#include <stdio.h>

int main(){
    int array1[5];
    array1 = {1, 2, 3, 4, 5};
    printf("%d %d %d %d %d\n", array1[0], array1[1], array1[2], array1[3], array1[4]);

    float array2[5] = {1., 2., 3., 4., 5.};
    printf("%f %f %f %f %f\n", array2[0], array2[1], array2[2], array2[3], array2[4]);

    char array3[] = {'a', 'b', 'c', 'd', 'e'};
    printf("%c %c %c %c %c\n", array3[0], array3[1], array3[2], array3[3], array3[4]);

    double array4[5];
    array4[0] = 0.0;
    array4[1] = 0.1;
    array4[2] = 0.2;
    array4[3] = 0.3;
    array4[4] = 0.4;
    printf("%lf %lf %lf %lf %lf\n", array4[0], array4[1], array4[2], array4[3], array4[4]);

    int array5[2][3];
    array5 = {{1, 2, 3}, {4, 5, 6}};
    printf("%d %d %d %d %d %d\n", array5[0][0], array5[0][1], array5[0][2], array5[1][0], array5[1][1], array5[1][2]);

    float array6[2][3] = {{1., 2., 3.}, {4., 5., 6.}};
    printf("%f %f %f %f %f %f\n", array6[0][0], array6[0][1], array6[0][2], array6[1][0], array6[1][1], array6[1][2]);

    char array7[2][3] = {{'a', 'b', 'c'}, {'d', 'e', 'f'}};
    return 0;
}
