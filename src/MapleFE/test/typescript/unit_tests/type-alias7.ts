class Klass {
  value: number = 0;
  key: string = "";
}

type MyType<T extends Klass>= Array<Array<T>>;
