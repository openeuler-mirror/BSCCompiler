function func(arg: number): number | undefined {
  if(arg < 1) return
  for(let i = 0; i < arg; i++)
     console.log(i);
  return arg * 10;
}
console.log(func(3));

