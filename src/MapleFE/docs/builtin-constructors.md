
## JavaScript built-in objects

JavaScript built-in object info is available at:

### 1. ECMA-262 standard
https://262.ecma-international.org/12.0/#sec-ecmascript-standard-built-in-objects
```
Under sections:
  19 The Global Object
  20 Fundamental Objects
  21 Numbers and Dates
  22 Text Processing
  23 Indexed Collections
  24 Keyed Collections
  25 Structured Data
```

### 2. Mozilla Developer docs
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference under Built-in objects

## JavaScript built-in object constructors

Not all built-in objects work as object constructors. The following is a list of 
JavaScript built-in objects that works as object constructors to create objects
of corresponding built-in type:

### 1. List of JavaScript built-in object constructors
```
  1 AggregateError
  2 Array
  3 ArrayBuffer
  4 AsyncFunction
  5 BigInt64Array
  6 BigUint64Array
  7 Boolean
  8 DataView
  9 Date
 10 Error
 11 EvalError
 12 FinalizationRegistry
 13 Float32Array
 14 Float64Array
 15 Function
 16 Generator
 17 GeneratorFunction
 18 Int16Array
 19 Int32Array
 20 Int8Array
 21 InternalError (Mozilla only)
 22 Map
 23 Math
 24 Number
 25 Object
 26 Promise
 27 Proxy
 28 RangeError
 29 ReferenceError
 30 RegExp
 31 Set
 32 SharedArrayBuffer
 33 String
 34 Symbol
 35 SyntaxError
 36 TypeError
 37 Uint16Array
 38 Uint32Array
 39 Uint8Array
 40 Uint8ClampedArray
 41 URIError
 42 WeakMap
 43 WeakRef
 44 WeakSet
```

### 2. JavaScript builtin String/Number/Boolean object constructor and string/number/boolean primitive
https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String#string_primitives_and_string_objects

"Note that JavaScript distinguishes between String objects and primitive string values. (The same is true of Boolean and Numbers.)

String literals (denoted by double or single quotes) and strings returned from String calls in a non-constructor context (that is, called without using the new keyword) are primitive strings. JavaScript automatically converts primitives to String objects, so that it's possible to use String object methods for primitive strings. In contexts where a method is to be invoked on a primitive string or a property lookup occurs, JavaScript will automatically wrap the string primitive and call the method or perform the property lookup."
```
  1 var s1 : string = "test";             // string literal
  2 var s2 : String = "test";             // string literal
  3 var s3 : string = String("test");     // string literal
  4 var s4 : String = String("test");     // string literal
  5 var s5 : String = new String("test"); // String object
  6 console.log(typeof(s1));     // string
  7 console.log(s1.slice(1,2));  // string literal s1 wrapped/converted to String object for call
  8 console.log(typeof(s2));     // string
  9 console.log(typeof(s3));     // string
 10 console.log(typeof(s4));     // string
 11 console.log(typeof(s5));     // object
```

For TypeScript to C++ mapping, string primitive maps to std::string, and String objects maples to builtin String object t2crt::String (same for Booelan and Numbers).

The type returned by JavaScript/TypeScript String/Number/Boolean builtin/constructor function depends on the usage:
- when used as a function, it is a type converter (convert between literal typeis), returns primitve/literal type string/number/boolean
- when used with new op(), it is a constructor and returns an object
- A variable declared as primitve type string/number/boolean will be wrapped/converted to a String/Number/Boolean object if any object property/method is referenced
  For TypeScript to C++, this conversion can be done by the runtime, but there is opportunity for optimization if it can be determined at compile time whether a primitive will be used as an object, in which case the primitve literal can be generated as object intead.


## TypeScript types

Additionally these TypeScript types will be treated as built-in object types too:
- Record
- Tuple
- Iterable
- Iterator 

### 1. Record and Tuple types
Record and Tuple currently are TypeScript only types. They are
not ECMA-262 standard yet, but has been proposed and undergoing standardization.

- https://tc39.es/proposal-record-tuple
- https://www.typescriptlang.org/docs/handbook/2/objects.html#tuple-types
- https://www.typescriptlang.org/docs/handbook/utility-types.html#recordkeys-type

### 2. Iterable and Iterator types
These are TypeScript types
- https://www.typescriptlang.org/docs/handbook/release-notes/typescript-3-6.html#stricter-generators
- https://www.typescriptlang.org/docs/handbook/iterators-and-generators.html#iterable-interface
