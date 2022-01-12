function class_deco(name: string): Function {
  function deco(ctor: Function): void {
    console.log("Class constructor is :", ctor, ", Name is: ", name);
  }
  return deco;
}

@class_deco('Klass')
class Klass {
  data:
    {n: number} = {n : 123};

  public dump (value: number) {
    switch (value) {
      case 1:
        console.log(value, this.data);
    }
  }
}

let obj: Klass = new Klass();
obj.dump(1);
