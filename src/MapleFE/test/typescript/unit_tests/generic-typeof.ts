class Klass<T> {
  f: T;
}

const t = new Klass<number>(); 
t.f = 123;
console.log(typeof t);

export type MyType = Klass<typeof t>;
