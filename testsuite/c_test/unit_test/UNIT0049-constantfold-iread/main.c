#include <stdio.h>

typedef struct {
  double val;
} Header;

typedef struct {
  int x;
  int y;
  int indexId;
} Key;

typedef struct {
  Header header;
  Key *key;
} Proto;


void foo(int id) {
  printf("%d\n", id);
}

void bar(const Header *protoObj)
{
  const Proto *proto = (const Proto *)(void *)protoObj;
  foo(proto->key->indexId);
}

int main() {
  Proto proto;
  Key key;
  key.indexId = 42;
  proto.key = &key;
  bar(&(proto.header));
}

