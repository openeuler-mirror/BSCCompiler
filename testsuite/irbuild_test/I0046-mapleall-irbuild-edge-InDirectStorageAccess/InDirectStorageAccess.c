#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct{
  char *name;
  int *number;
}People;

int addition(int a, int b)
{
  return a+b;
}
int main()
{
  //pointers
  //reference 
  char *cPtr = NULL;
  char name[] = "Huawei";
  cPtr = name;
  
  //Pointer arithematic
  char cPtr1 = *cPtr++;
  char *cPtr2 = ++cPtr;
    
  //dereference
  int *iPtr1, *iPtr2;
  *iPtr1 = 10;
  *iPtr2 = *iPtr1;

  People *sPtr;
  sPtr = new People();
  strcpy(sPtr->name, "huawei");
  *(sPtr->number) = 5;

  //arrays
  int i, j;
  int aPtr1[10];
  int aPtr2[10][10];
  int aPtr3[10][10][10];

  //1D array
  for(i=0;i<10;i++){
    aPtr1[i] = 10; //constant
    aPtr1[i] = i;  //varaible
    aPtr1[i] = *iPtr1; //pointer dereference
  }  

  //2D array
  for(i=0;i<10;i++){
    for(j=0;j<10;j++){
      aPtr2[i][j] = 10;      //constant value
      aPtr2[i][j] = i*j;     //variable arithmatic
      aPtr2[i][j] = aPtr1[(i*j)%10]; //pointer access
    }
  }
  
  //3D array
  int k;
  for(i=0;i<10;i++){
    for(j=0;j<10;j++){
      for(k=0;k<10;k++){
	aPtr3[i][j][k] = 10;
	aPtr3[i][j][k] = i*j*k;
	aPtr3[i][j][k] = aPtr2[i][j]*10;
      }
    }
  }
  
  //function pointer
  int (*fPtr1)(int, int);
  int (*fPtr2)(int, int);
  fPtr1 = addition;
  fPtr2 = &addition;
  int result = fPtr1(2,3);
      
  return result;
}
