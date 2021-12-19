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
let arr = [1, 2, 3]

function* generator() { yield 1; }

function insert(graph, ...args) {
  for (let arg of args) {
    let [obj, name]  = typeof arg === "string" ? [eval(arg), arg] : arg;
    if (typeof obj !== "undefined" && obj !== null)
      if (!graph.has(obj)) {
        graph.set(obj, name !== null || typeof obj !== "function" ? name : obj.toString().split(" ")[1].replace(/[^a-zA-Z0-9+]/g, ""));
        insert(graph, [obj.prototype, graph.get(obj) + "Prototype"], [obj.__proto__, null], [obj.constructor, null]);
      } else if (name !== null)
        graph.set(obj, name);
  }
}

// Dump graphs with edges for prototype, __proto__ and constructor properties of each objects
const gen = generator.prototype.__proto__;
for(let g = 0; g < 4; ++g) {
  let graph = new Map();
  if (g < 2)
    insert(graph, "Function", "Object", "Array", "arr", "mycar", "car");
  else
    insert(graph, "Function", "Object", "generator", [generator(), "generator_instance"], [generator.__proto__, "Generator"],
      [generator.constructor, "GeneratorFunction"], [gen, "GeneratorPrototype"], [gen.__proto__, "IteratorPrototype"]);
  console.log("digraph JS" + g + " {\nrankdir = TB;\nranksep=0.6;\nnodesep=0.6;\n" + (g % 2 == 1 ? "" : "newrank=true;"));
  for (let [key, value] of graph) {
    console.log("\n/* key =", key, "\nObject.getOwnPropertyNames(" + value + "):\n", Object.getOwnPropertyNames(key),
      "\n" + value + ".toString(): " + (typeof key !== "function" ? "-" : key.toString().replace(/\s+/g, " ")) + "\n*/");
    console.log(value + (value.includes("Prototype") ? "[shape=box];" : "[shape=oval];"));
    // Add edges for prototype properties of objects
    if (typeof key.prototype !== "undefined" && key.prototype !== null)
      console.log((g % 2 == 1 ? "" : "subgraph cluster_" + value
        + " {\nrank=same;\ncolor=white;\n" + value + ";\n" + graph.get(key.prototype) + "[shape=box];\n }")
        + value + " -> " + graph.get(key.prototype) + " [label=\"prototype\", color=blue, fontcolor=blue];");
    // Add edges for constructor properties of objects
    if (g % 2 == 1 && key.constructor !== "undefined" && key.constructor !== null)
        console.log(value + " -> " + graph.get(key.constructor) + " [label=\"ctor\", color=darkgreen, fontcolor=darkgreen];");
    // Add edges for __proto__ properties of objects
    if (key.__proto__ !== "undefined" && key.__proto__ !== null)
        console.log(value + " -> " + graph.get(key.__proto__) + " [label=\"__proto__\", color=red, fontcolor=red];");
  }
  console.log("}");
}
