#include "../include/ts2cpp.h"

namespace t2crt {

Object::Ctor   Object::ctor  (&Function::ctor, Function::ctor.prototype);
Function::Ctor Function::ctor(&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);
Number::Ctor   Number::ctor  (&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);
RegExp::Ctor   RegExp::ctor  (&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);

IteratorProto                IteratorPrototype(&Object::ctor, Object::ctor.prototype);
GeneratorFuncPrototype       Generator(&GeneratorFunction, Function::ctor.prototype, &IteratorPrototype);
GeneratorFunc                GeneratorFunction(&Function::ctor, &Function::ctor, Function::ctor.prototype, &Generator);
Object* GeneratorPrototype = Generator.prototype;

ARR_CTOR_DEF(int)
ARR_CTOR_DEF(long)
ARR_CTOR_DEF(double)
ARR_CTOR_DEF(JS_Val)
ARR_CTOR_DEF(Object)
ARR_CTOR_DEF(Object*)

} // namepsace t2crt

