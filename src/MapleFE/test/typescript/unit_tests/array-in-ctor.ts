class Klass {
  data: any;
  constructor() {
    {
      this.data = [
        new Array(123, 456)
      ];
    }
  }
  public dump (value: number) {
    switch (value) {
      case 1:
        console.log(value, this.data);
    }
  }
}

let obj: Klass = new Klass();
obj.dump(1);
