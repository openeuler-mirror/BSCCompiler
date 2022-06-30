#include <stdio.h>


int main() {
  int n1=5, n2=6, n3=7;
  int (*A)[n1][n2] = malloc(sizeof(int[n1][n2]));
  int (*B)[n2][n3] = malloc(sizeof(int[n2][n3]));
  int (*C)[n1][n3] = malloc(sizeof(int[n1][n3]));
  for (size_t i1 = 0; i1 < n1; i1++) {
    for (size_t i2 = 0; i2 < n2; i2++) {
      (*A)[i1][i2] = 1;
    }
  }
  for (size_t i2 = 0; i2 < n2; i2++) {
    for (size_t i3 = 0; i3 < n3; i3++) {
       (*B)[i2][i3] = 2;
    }
  }  
  for (size_t i1 = 0; i1 < n1; i1++) {
    for (size_t i3 = 0; i3 < n3; i3++) {
      (*C)[i1][i3] = 0.0;
      for (size_t i2 = 0; i2 < n2; i2++) {
	(*C)[i1][i3] += (*A)[i1][i2] * (*B)[i2][i3];
      }
    }
  }
  for (size_t i1 = 0; i1 < n1; i1++) {
    for (size_t i3 = 0; i3 < n3; i3++) {
      if ((*C)[i1][i3] != 12) {
	abort();
      }
    }
  }
  free(A); free(B); free(C);
  return 0;
}
