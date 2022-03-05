class Klass {
  public num: number = 1;
  
  if(n: number): boolean {
    return this.num == n;
  }

  try(n: number): void {
    if(n == this.num)
      console.log("EQ");
    else
      console.log("NE");
  }
}

var obj: Klass = new Klass();
console.log(obj.if(0));
console.log(obj.if(1));
obj.try(0);
obj.try(1);
