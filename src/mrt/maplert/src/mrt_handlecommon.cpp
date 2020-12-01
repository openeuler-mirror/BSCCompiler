/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "mrt_handlecommon.h"
namespace maplert{
MClass *GetDcClasingFromFrame() {
  JavaFrame frame;
  (void)MapleStack::GetLastJavaFrame(frame);
  frame.ResolveMethodMetadata();
  MethodMetaBase *md = reinterpret_cast<MethodMetaBase*>(const_cast<uint64_t*>(frame.GetMetadata()));
  return md->GetDeclaringClass();
}

void ParseSignatrueType(char *descriptor, const char *&methodSig) {
  char *tmpDescriptor = descriptor;
  DCHECK(tmpDescriptor != nullptr) << "ParseSignatrueType::tmpDescriptor is nullptr" << maple::endl;
  if (*methodSig != 'L' && *methodSig != '[') {
    *descriptor = *methodSig;
  } else {
    if (*methodSig == '[') {
      while (*methodSig == '[') {
        *tmpDescriptor++ = *methodSig++;
      }
    }
    if (*methodSig != 'L') {
      *tmpDescriptor = *methodSig;
    } else {
      while (*methodSig != ';') {
        *tmpDescriptor++ = *methodSig++;
      }
      *tmpDescriptor = ';';
    }
  }
}

string GeneIllegalArgumentExceptionString(const MClass *from, const MClass *to) {
  string fromClassName, toClassName;
  string str = "Expected receiver of type ";
  if (to != nullptr) {
    to->GetTypeName(toClassName);
    str += toClassName;
  } else {
    str += "void";
  }
  str += ", but got ";
  if (from != nullptr) {
    from->GetTypeName(fromClassName);
    str += fromClassName;
  } else {
    str += "void";
  }
  return str;
}

string GeneClassCastExceptionString(const MClass *from, const MClass *to) {
  string str, fromClassName, toClassName;
  if (from != nullptr) {
    from->GetTypeName(fromClassName);
    str = fromClassName;
  } else {
    str = "void";
  }
  str += " cannot be cast to ";
  if (to != nullptr) {
    to->GetTypeName(toClassName);
    str += toClassName;
  } else {
    str += "void";
  }
  return str;
}

void FillArgsInfoNoCheck(const ArgsWrapper &args, const char *typesMark, uint32_t arrSize,
                         BaseArgValue &paramArray, uint32_t begin) {
  uint32_t paramNum = arrSize - 1;
  for (uint32_t i = begin; i < paramNum; i++) {
    switch (typesMark[i]) {
      case 'C':
        paramArray.AddInt32(static_cast<jchar>(args.GetJint()));
        break;
      case 'S':
        paramArray.AddInt32(static_cast<jshort>(args.GetJint()));
        break;
      case 'B':
        paramArray.AddInt32(static_cast<jbyte>(args.GetJint()));
        break;
      case 'Z':
        paramArray.AddInt32(static_cast<jboolean>(args.GetJint()));
        break;
      case 'I':
        paramArray.AddInt32(args.GetJint());
        break;
      case 'D':
        paramArray.AddDouble(args.GetJdouble());
        break;
      case 'F':
        paramArray.AddFloat(args.GetJfloat());
        break;
      case 'J':
        paramArray.AddInt64(args.GetJlong());
        break;
      default:
        paramArray.AddReference(reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(args.GetObject())));
        break;
    }
  }
}

void DoInvoke(const MethodMeta &method, jvalue &result, ArgValue &paramArray) {
  MClass *retType = method.GetReturnType();
  CHECK_E_V(retType == nullptr, "DoInvoke: reType is nullptr");
  uintptr_t addr = method.GetFuncAddress();
  char *mark = retType->GetName();
  __MRT_ASSERT(mark != nullptr, "DoInvoke: mark is null");
  uint32_t stackSize = paramArray.GetStackSize();
  uint32_t dregSize = paramArray.GetFRegSize();
  switch (*mark) {
    case 'V':
      RuntimeStub<void>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'I':
      result.i = RuntimeStub<jint>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'B':
      result.b = RuntimeStub<jbyte>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'C':
      result.c = RuntimeStub<jchar>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'S':
      result.s = RuntimeStub<jshort>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'Z':
      result.z = RuntimeStub<jboolean>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'D':
      result.d = RuntimeStub<jdouble>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'F':
      result.f = RuntimeStub<jfloat>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    case 'J':
      result.j = RuntimeStub<jlong>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
    default: // L [
      result.l = RuntimeStub<jobject>::SlowCallCompiledMethod(addr, paramArray.GetData(), stackSize, dregSize);
      return;
  }
}

bool GetPrimShortType(const MClass *klass, char &type) {
  if (klass == nullptr) {
    LOG(FATAL) << "klass is nullptr." << maple::endl;
  }
  const char *descriptor = klass->GetName();
  if (!strcmp(descriptor, "Ljava/lang/Boolean;")) {
    type = 'Z';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Byte;")) {
    type = 'B';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Character;")) {
    type = 'C';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Short;")) {
    type = 'S';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Integer;")) {
    type = 'I';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Long;")) {
    type = 'J';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Float;")) {
    type = 'F';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Double;")) {
    type = 'D';
    return true;
  } else if (!strcmp(descriptor, "Ljava/lang/Void;")) {
    type = 'V';
    return true;
  }
  return false;
}

bool CheckPrimitiveCanBoxed(char shortType) {
  switch (shortType) { // full through
    case 'Z':
    case 'B':
    case 'C':
    case 'S':
    case 'I':
    case 'F':
    case 'D':
    case 'J':
      return true;
    default:  // void or not primitive
      return false;
  }
}

bool GetPrimShortTypeAndValue(const MObject *o, char &type, jvalue &value, const MClass *fromType) {
  const MClass *klass = (o == nullptr) ? fromType : o->GetClass();
  if (fromType->IsAbstract() && o == nullptr) {
    return true;
  }
  FieldMeta *valuefield = klass->GetDeclaredField("value");
  if (valuefield == nullptr && klass != WellKnown::GetMClassObject()) {
    return false;
  }
  if (o == nullptr) {
    return true;
  }
  if (valuefield == nullptr) {
    return false;
  }
  if (klass == WellKnown::GetMClassBoolean()) {
    type = 'Z';
    value.z = MRT_LOAD_JBOOLEAN(o, valuefield->GetOffset());
  } else if (klass == WellKnown::GetMClassByte()) {
    type = 'B';
    value.b = MRT_LOAD_JBYTE(o, valuefield->GetOffset());
  } else if (klass == WellKnown::GetMClassCharacter()) {
    type = 'C';
    value.c = MRT_LOAD_JCHAR(o, valuefield->GetOffset());
  } else if (klass == WellKnown::GetMClassFloat()) {
    type = 'F';
    value.f = MRT_LOAD_JFLOAT(o, valuefield->GetOffset());
  } else if (klass == WellKnown::GetMClassDouble()) {
    type = 'D';
    value.d = MRT_LOAD_JDOUBLE(o, valuefield->GetOffset());
  } else if (klass == WellKnown::GetMClassInteger()) {
    type = 'I';
    value.i = MRT_LOAD_JINT(o, valuefield->GetOffset());
  } else if (klass == WellKnown::GetMClassLong()) {
    type = 'J';
    value.j = MRT_LOAD_JLONG(o, valuefield->GetOffset());
  } else if (klass == WellKnown::GetMClassShort()) {
    type = 'S';
    value.s = MRT_LOAD_JSHORT(o, valuefield->GetOffset());
  }
  if (type != '0') {
    return true;
  }
  return false;
}
}
