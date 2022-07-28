unsigned  __attribute__((noinline))
start_sequence (unsigned * x, unsigned * y)
{
  return (unsigned )0xdeadbeef;
};

unsigned __attribute__((noinline))
gen_movsd (unsigned * operand0, unsigned * operand1)
{
  return start_sequence(operand0, operand1);
}

int main()
{
  unsigned x, y;

  x = 13;
  y = 14;
  (int)gen_movsd (&x, &y);
  return 0;
}

