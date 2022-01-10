#include "../include/ts2cpp.h"

namespace t2crt {

Object::Ctor   Object::ctor = Object::Ctor    (&Function::ctor, Function::ctor.prototype);
Function::Ctor Function::ctor = Function::Ctor(&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);
Number::Ctor   Number::ctor = Number::Ctor    (&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);

ARR_CTOR_DEF(int)
ARR_CTOR_DEF(long)
ARR_CTOR_DEF(double)
ARR_CTOR_DEF(JS_Val)
ARR_CTOR_DEF(Object)
ARR_CTOR_DEF(Object*)

} // namepsace t2crt

