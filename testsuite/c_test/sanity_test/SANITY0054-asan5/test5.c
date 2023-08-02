// ./run_asan.sh test5.c
int array[100] = {0}; 
int main (int argc, char** argv) {     
  array[100] = 1;
  return 0;
}
