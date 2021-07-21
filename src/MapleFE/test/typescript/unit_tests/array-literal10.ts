class Foo {
  public f1: number = 0;
  private f2: number = 0;
  constructor(a: number, b: number) {this.f1 = a; this.f2 = b;}
}
var funcs = [
  function func() : Foo {
    console.log("Returning a new object");
    return new Foo(789, 0);
  }
];
var obj : Foo;
var i : number = 1;
var res : number = (obj || funcs[0]())[`f${i}`];
console.log(res);
obj = new Foo(123, 456);
++i;
res = (obj || funcs[0]())[`f${i}`];
console.log(res);
