class Klass {
  items: number[] = [];
  break() {
    for (const i of this.items)
      console.log(i);
  }
}

var obj: Klass = new Klass();
obj.items.push(6, 2, 1, 4, 5, 3);
obj.break();
