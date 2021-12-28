class Klass {
  [key: string]: number;
};
 
type MyArray<T extends { type: string }> = Array<Klass[T['type']]>;
