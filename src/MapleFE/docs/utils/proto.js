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
  let objs =  [Function, Object], names = ["Function", "Object"], gen = undefined;
  if (g < 2) {
    objs.unshift ( arr,   Array,   Vehicle,   Car,   MyCar,   car,   mycar);
    names.unshift("arr", "Array", "Vehicle", "Car", "MyCar", "car", "mycar");
  } else {
    gen = generator.prototype.__proto__;
    objs.unshift ( generator,   generator(),          generator.constructor,  gen,                  gen.__proto__);
    names.unshift("generator", "generator_instance", "GeneratorFunction",    "GeneratorPrototype", "IteratorPrototype");
  }
  console.log("digraph JS" + g + " {\nranksep=0.6;\nnodesep=0.6;\n" + (g % 2 == 0 ? "newrank=true;\n" : ""));
  let num = objs.length;
  let k = num, suffix = "Prototype";
  // Add prototype objects and edges for them
  for(let i = 0; i < num; ++i) {
    console.log(names[i].includes(suffix) ? names[i] + "[shape=box];" : "");
    if(typeof objs[i].prototype !== "undefined") {
      objs[k]  = objs[i].prototype;
      let special = names[i] === "GeneratorFunction" && objs[k].prototype === gen;
      names[k] = special ? "Generator" : names[i] + suffix;
      console.log(special ? "GeneratorPrototype -> " + names[k] + " [label=\"prototype\", color=blue, fontcolor=blue, dir=back];" : "");
      console.log(g % 2 == 0 ? "subgraph cluster_" + names[i] + " {\nrank=same;\ncolor=white;\n" + names[i] + ";\n"
        + names[k] + "[shape=box];\n }" : names[k] + "[shape=box];");
      console.log(names[i] + " -> " + names[k] + " [label=\"prototype\", color=blue, fontcolor=blue];");
      k++;
    }
  }
  // Add edges for __proto__ and constructor properties of each objects
  num = objs.length;
  for(let i = 0; i < num; ++i) {
    console.log("/* Object.getOwnPropertyNames(" + names[i] + "):\n", Object.getOwnPropertyNames(objs[i]), "*/");
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
