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
let myCar = new MyCar("My car");
let arr = [1, 2, 3]
let regexpr = /ab+c/i

function* generator() { yield 1; }
const gpt = generator.prototype.__proto__;

function makeClosure(a) {
  return function (b) {
    return a + b;
  }
}
const closure = makeClosure(1);

let myMap = new Map();
let myMapIterator = myMap[Symbol.iterator]();
let MapIteratorPrototype = Object.getPrototypeOf(new Map()[Symbol.iterator]());

async function asyncFunction() {}
async function* asyncGenerator() {}
const agpt = asyncGenerator.prototype.__proto__;

// All data for generating graphs
let graphData = {
  Class    : ["Array", "arr", "myCar", "car"],
  Generator: ["generator", [generator(), "generator_instance"], [gpt, "GeneratorPrototype"],
              [gpt.__proto__, "IteratorPrototype"], [generator.__proto__, "Generator"]],
  Builtin  : ["Symbol", "Math", "JSON", "Promise", "RegExp", "regexpr"],
  Closure  : ["makeClosure", "closure"],
  Iterator : ["myMap", "myMapIterator", "MapIteratorPrototype", [gpt.__proto__, "IteratorPrototype"]],
  Async    : ["asyncFunction", "asyncGenerator", [asyncGenerator(), "asyncGenerator_instance"], [agpt, "AsyncGeneratorPrototype"],
              [agpt.__proto__, "AsyncIteratorPrototype"], [asyncGenerator.__proto__, "AsyncGenerator"]],
};

generateGraph(graphData);

function generateGraph(data) {
  // Gather all reachable objects from their prototype, __proto__ and constructor properties
  function insert(g, depth, ...args) {
    for (let arg of args) {
      let [o, name] = typeof arg === "string" ? [eval(arg), arg] : arg;
      if (typeof o !== "undefined" && o !== null)
        if (!g.has(o)) {
          g.set(o, name !== null || typeof o !== "function" ? name : o.toString().split(" ")[1].replace(/[^a-zA-Z0-9+]/g, ""));
          insert(g, depth + 1, [o.prototype, g.get(o) === null ? null : g.get(o) + "Prototype"], [o.__proto__, null], [o.constructor, null]);
        } else if (name !== null)
        g.set(o, name);
    }
    if (depth === 0) {
      let visited = new Set();
      for (let [index, [key, val]] of Array.from(g).entries()) {
        val = val === null || val === "" ? "Object_" + index : val.replace(/[^A-Za-z0-9]+/g, "_");
        if (visited.has(val)) val += "__" + index;
        visited.add(val);
        g.set(key, val);
      }
    }
  }

  // Dump graphs with edges for prototype, __proto__ and constructor properties of each object
  let nodejs = (typeof process !== 'undefined') && (process.release.name === 'node')
  for (let prop in data) {
    let graph = new Map();
    insert(graph, 0, "Function", "Object", ...data[prop]);
    for (let ctor of ["", "_with_Constructor_Edges"]) {
      console.log("digraph JS_" + prop + ctor + " {\nlabel=\"\\n" + prop + " Graph" + ctor.replace(/_/g, " ") + "\\n(Node in gray: function)"
        + "\";\nrankdir = TB;\nranksep=0.6;\nnodesep=0.6;\n" + (ctor != "" ? "" : "newrank=true;"));
      for (let [index, [key, val]] of Array.from(graph).entries()) {
        let func = typeof key === "function";
        // Add comments with detailed information of keys
        if (nodejs)
          console.log("\n/* key =", key, "\nObject.getOwnPropertyNames(" + val + "):\n", Object.getOwnPropertyNames(key),
            "\n" + val + ".toString(): " + (func ? key.toString().replace(/\s+/g, " ") : "-") + "\n*/");
        console.log(val + " [label=\"" + val + " " + (index < 4 ? 3 - index : index) + "\", shape="
          + (val.includes("Prototype") ? "box" : "oval") + (func ? ", style=filled" : "") + "];");
        // Add edges for prototype, constructor and __proto__ properties of objects
        for (let [f, c] of [["prototype", "blue"], ["constructor", "darkgreen"], ["__proto__", "red"]])
          if (typeof key[f] !== "undefined" && key[f] !== null && graph.has(key[f]) && (ctor != "" || f !== "constructor"))
            console.log((ctor != "" || f !== "prototype" ? "" : "subgraph cluster_" + val + " {\nlabel=\"\";rank=same;color=white;\n"
              + val + ";\n" + graph.get(key.prototype) + " [shape=box];\n}\n") + val + " -> " + graph.get(key[f])
              + " [label=\"" + (f === "constructor" ? "ctor" : f) + "\", color=" + c + ", fontcolor=" + c + "];");
      }
      console.log("} // digraph JS_" + prop + ctor);
    }
  }
} // generateGraph 
