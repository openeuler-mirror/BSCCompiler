// Command line to get graphs: nodejs proto.js | ./viewdot.sh
class Vehicle {
  constructor (name) { this.name = name; }
}
class Car extends Vehicle {
  constructor (name) { super(name); }
}
class MyCar extends Car {
  constructor (name) { super(name); }
}

let car = new Car("A car");
let mycar = new MyCar("My car");
let arr = []

function* generator() {}

// Dump graphs with edges for prototype, __proto__ and constructor properties of each objects
for(let g = 0; g < 4; ++g) {
  let objs =  [], names = [];
  if (g < 2) {
    objs.push(  arr,   Array,   Vehicle,   Car,   MyCar,   car,   mycar,   Function,   Object);
    names.push("arr", "Array", "Vehicle", "Car", "MyCar", "car", "mycar", "Function", "Object");
  } else {
    let gen = generator.prototype.__proto__;
    objs.push(generator,     generator(),       generator.constructor,  gen,         gen.__proto__,    Function,   Object);
    names.push("generator", "generator_instance", "GeneratorFunction", "Generator", "GeneratorProto", "Function", "Object");
  }
  console.log("digraph JS" + g + " {\nranksep=0.6;\nnodesep=0.6;\n" + (g % 2 == 0 ? "newrank=true;\n" : ""));
  let num = objs.length;
  let k = num;
  // Add prototype objects and edges for them
  for(let i = 0; i < num; ++i) {
    let x = typeof objs[i].prototype;
    console.log("// " + names[i] + ": prototype: " + x + ", __proto__:" + typeof objs[i].__proto__);
    if(x === "function" || x === "object") {
      objs[k] = objs[i].prototype;
      names[k] = names[i] + "_prototype";
      console.log(g % 2 == 0 ? "subgraph cluster_" + names[i] + " {\nrank=same;\ncolor=white;\n"
        + names[i] + ";\n" + names[k] + "[label=\"" + names[i] + ".prototype\", shape=box];\n }"
        : names[k] + "[label=\"" + names[i] + ".prototype\", shape=box];");
      console.log(names[i] + " -> " + names[k] + " [label=\"prototype\", color=blue, fontcolor=blue];");
      k++;
    }
  }
  // Add edges for __proto__ and constructor properties of each objects
  num = objs.length;
  for(let i = 0; i < num; ++i) {
    for(let j = 0; j < num; ++j) {
      // Edges for constructor properties in the second graph only
      if(g % 2 == 1 && objs[i].constructor === objs[j])
        console.log(names[i] + " -> " + names[j] + " [label=\"ctor\", color=darkgreen, fontcolor=darkgreen];");
      // Edges for __proto__ properties
      if(objs[i].__proto__ === objs[j])
        console.log(names[i] + " -> " + names[j] + " [label=\"__proto__\", color=red, fontcolor=red];");
    }
  }
  console.log("}");
}
