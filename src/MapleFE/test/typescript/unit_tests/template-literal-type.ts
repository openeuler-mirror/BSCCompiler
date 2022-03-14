class Num {
  neg: boolean = false;
  val: number = 0;
}

function func(v: Num): `${string}n` | `-${string}n` {
  return `${v.neg ? '-' : ''}${v.val}n`;
}

var obj : Num = {neg: true, val: 123};
console.log(func(obj));
console.log(typeof func(obj));
