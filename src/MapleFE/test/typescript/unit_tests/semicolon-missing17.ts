function func(arg: number): number | undefined {
  for(let i = 0; i < arg; i++) {
    if(i % 2 > 0) continue
     console.log(i);
  }
  return arg * 10;
}
console.log(func(5));

