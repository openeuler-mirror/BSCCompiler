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
const gen = generator.prototype.__proto__;

function makeClosure(a) {
  return function (b) {
    return a + b;
  }
}
const closure = makeClosure(1);

// All data for generating graphs
let data = [
  ["Classes",   ["Array", "arr", "mycar", "car"]],
  ["Generator", ["generator", [generator(), "generator_instance"], [generator.__proto__, "Generator"],
                 [generator.constructor, "GeneratorFunction"], [gen, "GeneratorPrototype"], [gen.__proto__, "IteratorPrototype"]]],
  ["Builtins",  ["Symbol", "Math", "JSON", "Promise"]],
  ["Closure",   ["makeClosure", "closure"]],
];

// Gether all reachable objects from their prototype, __proto__ and constructor properties
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

// Dump graphs with edges for prototype, __proto__ and constructor properties of each object
let nodejs = (typeof process !== 'undefined') && (process.release.name === 'node')
for (let entry of data) {
  let [title, objs] = entry;
  let graph = new Map();
  insert(graph, "Function", "Object", ...objs);
  for (let ctor of ["", "_with_ctors"]) {
    console.log("digraph JS_" + title + ctor + " {\nrankdir = TB;\nranksep=0.6;\nnodesep=0.6;\n" + (ctor != "" ? "" : "newrank=true;"));
    for (let [key, value] of graph) {
      // Add comments with detailed information of keys
      if (nodejs)
        console.log("\n/* key =", key, "\nObject.getOwnPropertyNames(" + value + "):\n", Object.getOwnPropertyNames(key),
          "\n" + value + ".toString(): " + (typeof key !== "function" ? "-" : key.toString().replace(/\s+/g, " ")) + "\n*/");
      console.log(value + (value.includes("Prototype") ? "[shape=box];" : "[shape=oval];"));
      // Add edges for prototype properties of objects
      if (typeof key.prototype !== "undefined" && key.prototype !== null)
        console.log((ctor != "" ? "" : "subgraph cluster_" + value
          + " {\nrank=same;\ncolor=white;\n" + value + ";\n" + graph.get(key.prototype) + "[shape=box];\n }")
          + value + " -> " + graph.get(key.prototype) + " [label=\"prototype\", color=blue, fontcolor=blue];");
      // Add edges for constructor properties of objects
      if (ctor != "" && key.constructor !== "undefined" && key.constructor !== null)
        console.log(value + " -> " + graph.get(key.constructor) + " [label=\"ctor\", color=darkgreen, fontcolor=darkgreen];");
      // Add edges for __proto__ properties of objects
      if (key.__proto__ !== "undefined" && key.__proto__ !== null)
        console.log(value + " -> " + graph.get(key.__proto__) + " [label=\"__proto__\", color=red, fontcolor=red];");
    }
    console.log("} // digraph JS_" + title + ctor);
  }
}
