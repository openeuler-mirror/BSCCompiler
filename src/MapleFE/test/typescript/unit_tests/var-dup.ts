var name:string = "abc";
function foo(a: number) {
  var sum = 0;
  for (var i = 0; i < a; i++) {
    var i = 4;
    for (var i = 0; i < a; i++) {
      let i = 8;
      sum += i;
    }
  }

  return sum;
}
var name:string = "xyz";
console.log(foo(10));
console.log(name);
