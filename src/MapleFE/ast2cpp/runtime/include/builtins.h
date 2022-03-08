/*
 * Copyright (C) [2021-2022] Futurewei Technologies, Inc. All rights reverved.
 *
 * OpenArkFE is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *  http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include <sstream>

namespace t2crt {

template <typename T1, typename T2>
class Record : public Object {
  public:
    std::unordered_map<T1, T2> records;
    Record() {}
    ~Record() {}
    Record(Function* ctor, Object* proto) : Object(ctor, proto) {}
    Record(Function* ctor, Object* proto, std::vector<ObjectProp> props) : Object(ctor, proto, props) {}
};

template <typename T>
class Array : public Object {
  public:
    std::vector<T> elements;
    Array(Function* ctor, Object* proto): Object(ctor, proto) {}
    Array(Function* ctor, Object* proto, std::initializer_list<T> l): Object(ctor, proto), elements(l) {}

    T& operator[](int i) {return elements[i];}
    void operator = (const std::vector<T> &v) { elements = v; }
    long size() { return elements.size(); }

    // Output array to string (recurses if multi-dim array via ostream output operator overload in t2cpp.cpp)
    std::string Dump (void) override {
      std::stringstream ss;
      std::streambuf* old = std::cout.rdbuf(ss.rdbuf());
      if (elements.empty())
        std::cout << "[]";
      else {
        std::cout << "[ ";
        auto i = elements.begin(), e = elements.end();
        std::cout << *i++;
        for (; i != e; ++i)
          std::cout << ", " << *i;
        std::cout << " ]";
      }
      std::cout.rdbuf(old);
      return ss.str();
    }

    // Put JS Array.prototype props as static fields and methods in this class
    // and add to proplist of Array_ctor.prototype object on system init.

    class Ctor: public Function {
    public:
      Ctor(Function* ctor, Object* proto, Object* prototype_proto) : Function(ctor, proto, prototype_proto) {}
      Array<T>* _new() {
        return new Array<T>(this, this->prototype);
      }
      Array<T>* _new(std::initializer_list<T>l) {
        return new Array<T>(this, this->prototype, l);
      }
    };
    static Ctor ctor;
};

// Create ctor func for 1,2,3 dimension array of given type
// note: must be in sync with format generated by ArrayCtorName in helper.h
#define ARR_CTOR_DEF(type) \
  template <> \
  Array<type>::Ctor                 Array<type>::ctor = Array<type>::Ctor(&Function::ctor, Function::ctor.prototype, Object::ctor.prototype); \
  template <> \
  Array<Array<type>*>::Ctor         Array<Array<type>*>::ctor = Array<Array<type>*>::Ctor(&Function::ctor, Function::ctor.prototype, Object::ctor.prototype); \
  template <> \
  Array<Array<Array<type>*>*>::Ctor Array<Array<Array<type>*>*>::ctor = Array<Array<Array<type>*>*>::Ctor(&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);

class JSON : public Object {
  // TODO
};

class RegExp : public Object {
  // TODO
public:
  RegExp(Function* ctor, Object* proto): Object(ctor, proto) { }
  RegExp(Function* ctor, Object* proto, std::string src): Object(ctor, proto) {  source = src; }
  ~RegExp(){}
  std::string source;               // text of the pattern
  std::string Dump(void) override { return source; }

  class Ctor : public Function {
  public:
    Ctor(Function* ctor, Object* proto, Object* prototype_proto) : Function(ctor, proto, prototype_proto) { }
    RegExp* _new(std::string src) {return new RegExp(this, this->prototype, src);}
    virtual const char* __GetClassName() const {return "RegExp ";}
  };
  static Ctor ctor;
};

class Number : public Object {
public:
  // TODO
  class Ctor : public Function {
  public:
    Ctor(Function* ctor, Object* proto, Object* prototype_proto) : Function(ctor, proto, prototype_proto) { }
    virtual const char* __GetClassName() const {return "Number ";}
  };
  static Ctor ctor;
};


// 20.5 Error objects for execptions
class Error : public Object {
  // TODO
};

// JavaScript generators and generator functions
// - The builtin GeneratorFunction is the constructor for all generator functions.
// - Generator functions are called directly to return generators (with closure).
// - Generators are iterators that calls corresponding generator function with
//   data captured in closure to iterate for results.

// ecma-262 section references are based on ecma-262 edition 12.0

// ecma262 27.1.1.5 IteratorResult interface:
struct IteratorResult : public Object {
  bool   done;  // status of iterator next() call
  JS_Val value; // done=false: current iteration element value
                // done=true:  return value of the iterator, undefined if none returned
  IteratorResult() : done(true), value(undefined) { }
  IteratorResult(bool done, JS_Val val) : done(done), value(val) { }
  ~IteratorResult() { }
};

// ecma262 27.1.1.1 Iterable interface:
// To be iterable, an object or one of the objects up its prototype chain must
// have a property with a @@iterator key (<obj>[Symbol.iterator], the value of
// which is a function that returns iterators (i.e objects with Iterator interace
// methods next/return/throw).
//
// Note: For iterable objects such as arrays and strings, <obj>[Symbol.iterator]()
// returns a new iteraor object. But for the intrinsic object %IteratorPrototype%
// (27.1.2.1) it returns the current iterator instance, which
// means for all iterators, <obj>[Sumbol.iterator]() returns itself.

// ecma262 27.1.2.1 %IteratorPrototype%:
// 1) All objects that implement iterator interface also inherit from %IteratorPrototype%
// 2) %IteratorPrototype% provides shared props for all iterator objects
// 3) %IteratorPrototype%[Symbol.iterator]() = this (current iterator instance) - used in for loops
class IteratorProto : public Object {
public:
  IteratorResult _res;
  IteratorProto(Function* ctor, Object* proto) : Object(ctor, proto) { }
  ~IteratorProto() { }
  // note: the arg on an iterator's 1st next() call is ignored per spec 27.5.1.2
  virtual IteratorResult* next  (JS_Val* arg = nullptr) { return &_res; }
  virtual IteratorResult* _return(JS_Val* val = nullptr) { return &_res; }
  virtual IteratorResult* _throw(Error exception)  { return &_res; }

  // TODO: %IteratorPrototype%[Symbol.iterator]() = this (current iterator instance)
};

// 27.5.1 Generator Prototype Object
// - in ecma edition 11: named %GeneratorPrototype% (25.4.1)
// - in ecma edition 12: named %GeneratorFunction.prototype.prototype% (27.5.1) but
//                       labelled as %GeneratoPrototype% in 27.3 Figure 5.
//                       Label corrected in version at tc39.
class GeneratorProto : public IteratorProto {
public:
  IteratorResult _res;
  GeneratorProto(Function* ctor, Object* proto) : IteratorProto(ctor, proto) { }
  ~GeneratorProto() { }
  void*  _yield     = nullptr; // pointer to yield label to resume execution
  bool   _finished  = false;   // flag if generator is in finished state
  bool   _firstNext = true;    // flag if first next has been called on iterator (27.5.1.2)
  
  IteratorResult* _return(JS_Val* arg = nullptr) override {
    _finished = true;
    if (arg != nullptr) {
      _res.value = *arg;
    }
    return &_res;
  }
};

// 27.3.1 GeneratorFunction Constructor
class GeneratorFunc : public Function::Ctor {
public:
  GeneratorFunc(Function* ctor, Object* proto, Object* prototype_proto, Function* prototype_obj) : Function::Ctor(ctor, proto, prototype_proto, prototype_obj) { }
  ~GeneratorFunc() {}
};

// 27.3.3 GeneratorFunction Prototype Obejct
// - in ecma edition 11: named %Generator% (25.2.3)
// - in ecma edition 12: named %GeneratorFunction.prorotype% (27.3.3) but
//                       labelled as %Generator% in 27.3 Figure 5.
//                       Label corrected in tc39 version.
class GeneratorFuncPrototype : public Function {
public:
  GeneratorFuncPrototype(Function* ctor, Object* proto, Object* prototype_proto) : Function(ctor, proto, prototype_proto) { }
};

// Generator related intrinsic objects. (ecma 27.3)
// IteratorPrototype: It is not a prototype object of any constructor func, but holds shared properties for iterators
// GeneratorFunction: A builtin function used as the constructor for generator functions.
// Generator:         (GeneratorFuncion.prototype in edition 12.0) is the prototype object of GeneratorFunction,
//                    It is a special object used as both prototype object and constructor - as prototype for sharing
//                    properties between generator functions, and as constructor whose prototype object (GeneratorPrototype
//                    in edition 11) holds shared properties for generators (i.e. instances returned by generator functions.
extern IteratorProto              IteratorPrototype;
extern GeneratorFunc              GeneratorFunction;
extern GeneratorFuncPrototype     Generator;
extern Object*                    GeneratorPrototype;

} // namespace t2crt


using t2crt::Record;
using t2crt::JSON;
using t2crt::RegExp;
#endif // __BUILTINS_H__
