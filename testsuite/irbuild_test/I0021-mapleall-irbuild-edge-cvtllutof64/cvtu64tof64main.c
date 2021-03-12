extern double cvtu64tof64(unsigned long long int x);

int main(){
  //printf("%f\n", cvtu64tof32(0xffffffff));
  //printf("%f\n", cvtu64tof32(0xfffffffffffff));
  //printf("%f\n", cvtu64tof32(1));
  printf("%f\n", cvtu64tof64(-1));
  printf("%f\n", cvtu64tof64(-111111111111111111));
  printf("%f\n", cvtu64tof64(1343423432));
  printf("%f\n", cvtu64tof64(44444444444444444444));
  printf("%f\n", cvtu64tof64(0));
  printf("%f\n", cvtu64tof64(0xffffffffffffffff));
  printf("%f\n", cvtu64tof64(1));
}
