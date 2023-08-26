#include "stdio.h"
#include "stdlib.h"

typedef struct {
  long long *vec;
  unsigned int word_num;
  unsigned long long word;
} ira_allocno_set_iterator;

__attribute__((noinline))
_Bool ira_allocno_set_iter_cond (ira_allocno_set_iterator *i, int *n)
{
  // can not opt i->word == 0 in phase vrp
  for (; i->word == 0; i->word = i->vec[i->word_num])
  {
    printf("%d, %d, %d\n", i->word, i->word_num, i->vec[i->word_num]);
    i->word_num++;
  }
  return 1;
}

int main() {
  ira_allocno_set_iterator *i = malloc(sizeof(ira_allocno_set_iterator));
  i->word = 0;
  i->word_num = 0;
  i->vec = malloc(sizeof(long long)*4);
  i->vec[0] = 0;
  i->vec[1] = 0;
  i->vec[2] = 0;
  i->vec[3] = 1;
  int *n = malloc(sizeof(int));
  ira_allocno_set_iter_cond(i, n);
  return 0;
}
