#define NUM_CHARS (4)

typedef unsigned int wchar_t;
wchar_t utf_32_string[NUM_CHARS];

char iso_8859_1_string[NUM_CHARS] = {'a', 'b', 'c', 'd'};


int main() {
  int i;
  for (i = 0; i < NUM_CHARS; ++i)
    utf_32_string[i] = iso_8859_1_string[i] & 0xff;

  return 0;
}
