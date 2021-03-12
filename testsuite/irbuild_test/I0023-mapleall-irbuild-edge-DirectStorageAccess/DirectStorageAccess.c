#include <iostream>
#include <string.h>
//Define a struct
typedef struct{
  char *address;
  char name[10];
  int age;
}People;

typedef struct{
  int number;
  People employee;
}Info;

int main()
{
  //direct read and assign
  //direct read from scalar
  char a = 'x';
  char b = a;

  int i = 5;
  int ii = i;
  
  float j = 12.9;
  float jj = j;

  double k = 20.5;
  double kk = k;
  
  //direct read/write to fields inside a structure
  //Structure field assign
  People p, pp;
  p.age = 25;
  p.address = NULL;
  strcpy(p.name,"Huawei");

  //structure field read
  pp.age = p.age;
  pp.address = p.address;
  strcpy(pp.name, p.name);

  Info info, info2;
  info.employee = p;
  info2.employee = info.employee;
  return 1;
}
