/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reverved.
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

class Ctor_Function;
class Ctor_Object;
//class Ctor_Array;

extern Ctor_Function Function_ctor;
extern Ctor_Object   Object_ctor;
//extern Ctor_Array    Array_ctor;

class Ctor_Object   : public Function {
  public:
    Ctor_Object(Function* ctor, Object* proto, Object* prototype_proto) : Function(ctor, proto, prototype_proto) {}

    Object* _new() {
      return new Object(this, this->prototype);
    }

    Object* _new(std::vector<ObjectProp> props) {
      return new Object(this, this->prototype, props);
    }
};

class Ctor_Function : public Function {
  public:
    Ctor_Function(Function* ctor, Object* proto, Object* prototype_proto) : Function(ctor, proto, prototype_proto) {}

    Function* _new() {
      return new Function(this, this->prototype, Object_ctor.prototype);
    }

};

template <typename T>
class Array : public Object {
  public:
    std::vector<T> elements;
    Array(Function* ctor, Object* proto): Object(ctor, proto) {}
    Array(Function* ctor, Object* proto, std::initializer_list<T> l): Object(ctor, proto), elements(l) {}

    T& operator[](int i) {return elements[i];}
    void operator = (const std::vector<T> &v) { elements = v; }

    // Put JS Array.prototype props as static fields and methods in this class
    // and add to proplist of Array_ctor.prototype object on system init.
};

template <typename T>
class Ctor_Array: public Function {
  public:
    Ctor_Array(Function* ctor, Object* proto, Object* prototype_proto) : Function(ctor, proto, prototype_proto) {}

    Array<T>* _new() {
      return new Array<T>(this, this->prototype);
    }

    Array<T>* _new(std::initializer_list<T>l) {
      return new Array<T>(this, this->prototype, l);
    }

};

// For creating array and array constructor instances
#define ARR_OBJ(NM,CTOR) NM(&CTOR, CTOR.prototype)
#define ARR_CTOR(NM)     NM(&Function_ctor, Function_ctor.prototype, Object_ctor.prototype)

#endif // __BUILTINS_H__
