#include "../include/ts2cpp.h"

namespace t2crt {

Object::Ctor   Object::ctor = Object::Ctor    (&Function::ctor, Function::ctor.prototype);
Function::Ctor Function::ctor = Function::Ctor(&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);
Number::Ctor   Number::ctor = Number::Ctor    (&Function::ctor, Function::ctor.prototype, Object::ctor.prototype);

ARRAY_CTOR_DEF(int)
ARRAY_CTOR_DEF(long)
ARRAY_CTOR_DEF(double)
ARRAY_CTOR_DEF(JS_Val)
ARRAY_CTOR_DEF(Object)
ARRAY_CTOR_DEF(ObjectP)

} // namepsace t2crt

