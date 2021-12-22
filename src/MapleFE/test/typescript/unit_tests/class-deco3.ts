function class_deco(name: string): Function {
  function deco(ctor: Function): void {
    console.log("Class constructor is :", ctor, ", Name is: ", name);
  }
  return deco;
}

@class_deco('Klass')
class Klass {
  data: any = null;
  public setData(value: any) {
    this.data= [
      {
        n: value,
      },
    ];
  }

  public dump (value: number) {
    switch (value) {
      case 1:
        console.log(value, this.data);
    }
  }
}

let obj: Klass = new Klass();
obj.setData(123);
obj.dump(1);
