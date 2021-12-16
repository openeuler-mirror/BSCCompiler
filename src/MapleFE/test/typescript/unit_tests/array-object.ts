function func(arg: any) {
  console.log(arg);
}
func([ [`Number`, { type: Number, count: 1, }],
       [`String`, { type: String, count: 2, }],
     ] as Array<[ name: string, value: any, ]>);
