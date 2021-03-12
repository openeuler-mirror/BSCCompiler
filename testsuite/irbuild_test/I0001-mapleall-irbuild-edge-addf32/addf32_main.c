extern float addf32r(float, float);
extern float addf32I(float);

int main(){
  printf("%f vs %f\n", addf32r(1.2345, -1.234f), 1.2345 - 1.234);
  printf("%f vs %f\n", addf32I(-2.2222), -2.2222 + 1.234f);
}

