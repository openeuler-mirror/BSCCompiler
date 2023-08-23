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
#include "java_lang_String.h"
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include "securec.h"
#include "mstring_inline.h"
#include "libs.h"
#include "exception/mrt_exception.h"
#include "mrt_monitor_api.h"
#include "java2c_rule.h"
using namespace std;
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

MRT_EXPORT jchar Native_java_lang_String_charAt__I(jstring jthis, jint index) {
  MString *stringObj = MString::JniCast(jthis);
  if (UNLIKELY(stringObj == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return 0;
  }
  int32_t len = static_cast<int32_t>(stringObj->GetLength());
  if (index < 0 || index >= len) {
    char msg[50] = { 0 };  // message size is 50, buffer size 45
    if (snprintf_s(msg, sizeof(msg), 45, "length=%d; index=%d", len, index) < 0) {
      return 0;
    }
    MRT_ThrowNewExceptionUnw("java/lang/StringIndexOutOfBoundsException", msg);
    return 0;
  }
  return stringObj->CharAt(static_cast<uint32_t>(index));
}

jchar Java_java_lang_String_charAt__I(JNIEnv*, jstring jthis, jint index) {
  MString *stringObj = MString::JniCast(jthis);
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return 0;
  }

  int32_t len = static_cast<int32_t>(stringObj->GetLength());
  if (index < 0 || index >= len) {
    char msg[50] = { 0 };  // message size is 50, buffer size 45
    if (snprintf_s(msg, sizeof(msg), 45, "length=%d; index=%d", len, index) < 0) {
      MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException");
      return 0;
    }
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException", msg);
    return 0;
  }
  return stringObj->CharAt(static_cast<uint32_t>(index));
}

// getChars without bounds checks, for use by other classes
// within the java.lang package only.  The caller is responsible for
// ensuring that start >= 0 && start <= end && end <= count.
//
// native void getCharsNoCheck(int start, int end, char[] buffer, int index)
// Ljava_2Flang_2FString_3B_7CgetCharsNoCheck_7C_28IIACI_29V
static void local_String_getCharsNoCheck(jstring jstr, jint start, jint end, jcharArray buffer, jint index) {
  MArray *mArray = reinterpret_cast<MArray*>(buffer);
  jchar *dst = reinterpret_cast<jchar*>(mArray->ConvertToCArray()) + index;
  MString *stringObj = MString::JniCastNonNull(jstr);
  if (stringObj->IsCompress()) {
    const uint8_t *src = stringObj->GetContentsPtr() + start;
    for (jint i = start; i < end; ++i) {
      *dst++ = *src++;
    }
  } else {
    uint16_t *src = reinterpret_cast<uint16_t*>(stringObj->GetContentsPtr()) + start;
    size_t memcpyLen = static_cast<size_t>(end - start) * sizeof(uint16_t);
    if (memcpyLen == 0) {
      return;
    }

    if (memcpy_s(dst, memcpyLen, src, memcpyLen) != EOK) {
      LOG(ERROR) << "In local_String_getCharsNoCheck Function memcpy_s() return wrong" << maple::endl;
    }
  }
}

MRT_EXPORT void Native_java_lang_String_getCharsNoCheck__II_3CI(
    jstring jthis,
    jint start,         // idx of 1st jchar in string to copy
    jint end,           // idx after last jchar in string to copy
    jcharArray buffer,  // dst jchar array
    jint index) {       // start offset in dst to copy to
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return;
  }
  local_String_getCharsNoCheck(jthis, start, end, buffer, index);
}

void Java_java_lang_String_getCharsNoCheck__II_3CI(
    JNIEnv*,
    jstring jthis,
    jint start,         // idx of 1st jchar in string to copy
    jint end,           // idx after last jchar in string to copy
    jcharArray buffer,  // dst jchar array
    jint index) {       // start offset in dst to copy to
  local_String_getCharsNoCheck(jthis, start, end, buffer, index);
}

/*
 * Converts this string to a new character array.
 *
 * @return  a newly allocated character array whose length is the length
 *          of this string and whose contents are initialized to contain
 *          the character sequence represented by this string.
 */
// public native char[] toCharArray()
// Ljava_2Flang_2FString_3B_7CtoCharArray_7C_28_29AC
static jcharArray local_String_toCharArray(jstring jstr) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  int32_t len = static_cast<int32_t>(stringObj->GetLength());
  jcharArray charArray = reinterpret_cast<jcharArray>(MArray::NewPrimitiveArray<kElemHWord>(static_cast<uint32_t>(len),
      *WellKnown::GetPrimitiveArrayClass(maple::Primitive::kChar)));
  if (charArray != nullptr) {
    const char *src = reinterpret_cast<char*>(stringObj->GetContentsPtr());
    jchar *dst = reinterpret_cast<jchar*>(reinterpret_cast<MArray*>(charArray)->ConvertToCArray());
    if (stringObj->IsCompress()) {
      for (jint i = 0; i < len; i++) {
        dst[i] = src[i];
      }
    } else {
      uint32_t cpyLen = static_cast<unsigned int>(len) << 1;
      if (cpyLen == 0) {
        return charArray;
      }

      if (memcpy_s(dst, cpyLen, src, cpyLen) != EOK) {
        LOG(ERROR) << "In local_String_toCharArray Function memcpy_s() return wrong" << maple::endl;
      }
    }
  }
  return charArray;
}

MRT_EXPORT jcharArray Native_java_lang_String_toCharArray__(jstring jthis) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  jcharArray charArray;

  charArray = local_String_toCharArray(jthis);
  if (charArray == nullptr) {
    MRT_CheckThrowPendingExceptionUnw();
  }
  return charArray;
}

jcharArray Java_java_lang_String_toCharArray__(JNIEnv *env, jstring jthis) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  ScopedObjectAccess soa;
  jcharArray chararray;

  chararray = local_String_toCharArray(jthis);
  return reinterpret_cast<jcharArray>(MRT_JNI_AddLocalReference(env, chararray));
}

MRT_EXPORT jstring Native_java_lang_String_fastSubstring__II(jstring jthis, jint start, jint length) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  jstring subStr = reinterpret_cast<jstring>(StringNewStringFromSubstring(
      *reinterpret_cast<MString*>(jthis), start, static_cast<uint32_t>(length)));
  return subStr;
}

jstring Java_java_lang_String_fastSubstring__II(
    JNIEnv *env,
    jstring jthis,
    jint start,
    jint length) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  ScopedObjectAccess soa;
  jstring subStr = reinterpret_cast<jstring>(StringNewStringFromSubstring(
      *reinterpret_cast<MString*>(jthis), start, static_cast<uint32_t>(length)));
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, subStr));
}

static jint local_String_compareTo(jstring jthis, jstring jarg) {
  if (jthis == jarg) {
    return 0;
  }
  MString *thisStringObj = MString::JniCastNonNull(jthis);
  MString *annotherStringObj = MString::JniCastNonNull(jarg);
  int32_t lenThis = static_cast<int32_t>(thisStringObj->GetLength());
  int32_t lenArg = static_cast<int32_t>(annotherStringObj->GetLength());

  int len = (lenThis > lenArg) ? lenArg : lenThis;

  uint8_t *srcThis = thisStringObj->GetContentsPtr();
  uint8_t *srcArg = annotherStringObj->GetContentsPtr();

  bool thisCompress = thisStringObj->IsCompress();
  bool argCompress = annotherStringObj->IsCompress();
  if (thisCompress && argCompress) {
    int i = 0;
    for (int j = 0; j < len; ++j) {
      if (srcThis[i] == srcArg[i]) {
        ++i;
        continue;
      }
      return srcThis[i] - srcArg[i];
    }
  } else if (thisCompress || argCompress) {
    uint8_t *compressChars = nullptr;
    uint16_t *uncompressChars = nullptr;
    if (thisCompress) {
      compressChars = srcThis;
      uncompressChars = reinterpret_cast<uint16_t*>(srcArg);
    } else {
      compressChars = srcArg;
      uncompressChars = reinterpret_cast<uint16_t*>(srcThis);
    }
    for (int i = 0; i < len; ++i) {
      if (uncompressChars[i] != static_cast<jchar>(static_cast<int16_t>(compressChars[i]))) {
        return (thisCompress) ? (static_cast<jchar>(static_cast<int16_t>(compressChars[i])) - uncompressChars[i]) :
            (uncompressChars[i] - static_cast<jchar>(static_cast<int16_t>(compressChars[i])));
      }
    }
  } else {
    uint16_t *utf16This = reinterpret_cast<uint16_t*>(srcThis);
    uint16_t *utf16Arg = reinterpret_cast<uint16_t*>(srcArg);
    for (int i = 0; i < len; ++i) {
      if (utf16This[i] == utf16Arg[i]) {
        continue;
      }
      return utf16This[i] - utf16Arg[i];
    }
  }
  return lenThis - lenArg;
}

MRT_EXPORT jint Native_java_lang_String_compareTo__Ljava_lang_String_2(jstring jthis, jstring jarg) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }

  if (UNLIKELY(jarg == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "rhs == null");
    return -1;
  }

  jint res = local_String_compareTo(jthis, jarg);

  return res;
}

jint Java_java_lang_String_compareTo__Ljava_lang_String_2(JNIEnv*, jstring jthis, jstring jarg) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  if (UNLIKELY(jarg == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "rhs == null");
    return -1;
  }
  return local_String_compareTo(jthis, jarg);
}

// func &Ljava_2Flang_2FString_3B_7Cintern_7C_28_29Ljava_2Flang_2FString_3B local native public virtual
// (var %_this <* <$Ljava_2Flang_2FString_3B>>) <* <$Ljava_2Flang_2FString_3B>>
MRT_EXPORT jstring Native_java_lang_String_intern__(jstring jthis) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  } else {
    ScopedObjectAccess soa;
    MString *stringObj = reinterpret_cast<MString*>(jthis);
    return reinterpret_cast<jstring>(stringObj->Intern());
  }
}


jstring Java_java_lang_String_intern__(JNIEnv *env, jstring jthis) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  } else {
    ScopedObjectAccess soa;
    jstring internStr = Native_java_lang_String_intern__(jthis);
    return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, internStr));
  }
}

MRT_EXPORT jstring Native_java_lang_String_doReplace__CC(jstring jthis, jchar oldChar, jchar newChar) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  jstring repStr = reinterpret_cast<jstring>(JStringDoReplace(*reinterpret_cast<MString*>(jthis), oldChar, newChar));
  return repStr;
}

jstring Java_java_lang_String_doReplace__CC(JNIEnv *env, jstring jthis, jchar oldChar, jchar newChar) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  ScopedObjectAccess soa;
  jstring repStr = reinterpret_cast<jstring>(JStringDoReplace(*reinterpret_cast<MString*>(jthis), oldChar, newChar));
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, repStr));
}

MRT_EXPORT jstring Native_java_lang_String_concat__Ljava_lang_String_2(jstring jthis, jstring jarg) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  if (UNLIKELY(jarg == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "string arg == null");
    return nullptr;
  }
  jstring conStr = reinterpret_cast<jstring>(StringConcat(
      *reinterpret_cast<MString*>(jthis), *reinterpret_cast<MString*>(jarg)));
  return conStr;
}

jstring Java_java_lang_String_concat__Ljava_lang_String_2(JNIEnv *env, jstring jthis, jstring jarg) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }

  if (UNLIKELY(jarg == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "string arg == null");
    return nullptr;
  }
  jstring conStr = reinterpret_cast<jstring>(StringConcat(
      *reinterpret_cast<MString*>(jthis), *reinterpret_cast<MString*>(jarg)));
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, conStr));
}

#if PLATFORM_SDK_VERSION < 27
MRT_EXPORT jint Native_java_lang_String_fastIndexOf__II(jstring jthis, jint ch, jint start) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  return StringFastIndexOf(*reinterpret_cast<MString*>(jthis), ch, start);
}

jint Java_java_lang_String_fastIndexOf__II(JNIEnv*, jstring jthis, jint ch, jint start) {
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  return StringFastIndexOf(*reinterpret_cast<MString*>(jthis), ch, start);
}
#endif

MRT_EXPORT jobject Native_java_lang_Object_clone_Ljava_lang_Object__(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  if (UNLIKELY(javaThis == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException",
                             "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  MObject *thisObj = reinterpret_cast<MObject*>(javaThis);
  MClass *jcClone = WellKnown::GetMClassCloneable();
  if (!thisObj->IsInstanceOf(*jcClone)) {
    std::string classNameStr = thisObj->GetClass()->GetName();
    std::string outputstr = std::string("Class ") + classNameStr + std::string(" doesn't implement Cloneable");
    MRT_ThrowNewExceptionUnw("java/lang/CloneNotSupportedException", outputstr.c_str());
  }

  jobject newObj = MRT_CloneJavaObject(javaThis);
  j2cRule.Epilogue();
  return newObj;
}

#ifdef __OPENJDK__
void Java_java_lang_String_nativeGetChars__II_3CI(
    JNIEnv*,
    jstring jthis,
    jint start,         // idx of 1st jchar in string to copy
    jint end,           // idx after last jchar in string to copy
    jcharArray buffer,  // dst jchar array
    jint index) {       // start offset in dst to copy to
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return;
  }

  local_String_getCharsNoCheck(jthis, start, end, buffer, index);
}

jstring Java_java_lang_String_nativeSubString__Ljava_lang_String_2II(
    JNIEnv *env,
    jstring jthis,
    jstring srcStr,
    jint start,
    jint length) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  jstring subStr = reinterpret_cast<jstring>(StringNewStringFromSubstring(
      *reinterpret_cast<MString*>(srcStr), start, static_cast<uint32_t>(length)));
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, subStr));
}

jstring Java_java_lang_String_nativeReplace__CC(JNIEnv *env, jstring jthis, jchar oldChar, jchar newChar) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return nullptr;
  }
  jstring repStr = reinterpret_cast<jstring>(JStringDoReplace(*reinterpret_cast<MString*>(jthis), oldChar, newChar));
  return reinterpret_cast<jstring>(MRT_JNI_AddLocalReference(env, repStr));
}

jint Java_java_lang_String_nativeIndexOf__Ljava_lang_String_2Ljava_lang_String_2I(
    JNIEnv*, jstring jthis, jstring subStr, jstring srcStr, jint fromIndex) {
  if (LIKELY(subStr == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "Parameter subStr must not be null");
    return -1;
  }
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  ScopedObjectAccess soa;
  MString *srcStrObj = reinterpret_cast<MString*>(srcStr);
  DCHECK(srcStrObj != nullptr);
  MString *subStrObj = reinterpret_cast<MString*>(subStr);
  jint res = StringNativeIndexOfP3(*subStrObj, *srcStrObj, fromIndex);
  return res;
}

jint Java_java_lang_String_nativeIndexOf__Ljava_lang_String_2_3CIII(
    JNIEnv*, jstring jthis, jstring subStr, jarray arraySrc, jint srcOffset, jint srcCount, jint fromIndex) {
  if (LIKELY(subStr == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "Parameter subStr must not be null");
    return -1;
  }
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }

  ScopedObjectAccess soa;
  MArray *srcArrayObj = reinterpret_cast<MArray*>(arraySrc);
  DCHECK(srcArrayObj != nullptr);
  MString *subStrObj = reinterpret_cast<MString*>(subStr);
  jint res = StringNativeIndexOfP5(*subStrObj, *srcArrayObj, srcOffset, srcCount, fromIndex);
  return res;
}

jint Java_java_lang_String_nativeLastIndexOf__Ljava_lang_String_2Ljava_lang_String_2I(
    JNIEnv*, jstring jthis, jstring subStr, jstring srcStr, jint fromIndex) {
  if (LIKELY(subStr == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "Parameter subStr must not be null");
    return -1;
  }
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  ScopedObjectAccess soa;
  MString *srcStrObj = reinterpret_cast<MString*>(srcStr);
  DCHECK(srcStrObj != nullptr);
  MString *subStrObj = reinterpret_cast<MString*>(subStr);
  jint res = StringNativeLastIndexOfP3(*subStrObj, *srcStrObj, fromIndex);
  return res;
}

jint Java_java_lang_String_nativeLastIndexOf__Ljava_lang_String_2_3CIII(
    JNIEnv*, jstring jthis, jstring subStr, jarray arraySrc,
    jint srcOffset, jint srcCount, jint fromIndex) {
  if (LIKELY(subStr == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException", "Parameter subStr must not be null");
    return -1;
  }
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  ScopedObjectAccess soa;
  MArray *srcArrayObj = reinterpret_cast<MArray*>(arraySrc);
  DCHECK(srcArrayObj != nullptr);
  MString *subStrObj = reinterpret_cast<MString*>(subStr);
  jint res = StringNativeLastIndexOfP5(*subStrObj, *srcArrayObj, srcOffset, srcCount, fromIndex);
  return res;
}
#endif

#ifdef __OPENJDK__
jint Java_java_lang_String_nativeCodePointAt__I(JNIEnv*, jstring jthis, jint index) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  MString *strObj = reinterpret_cast<MString*>(jthis);
  jint res = StringNativeCodePointAt(*strObj, index);
  return res;
}

jint Java_java_lang_String_nativeCodePointBefore__I(JNIEnv*, jstring jthis, jint index) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  MString *strObj = reinterpret_cast<MString*>(jthis);
  jint res = StringNativeCodePointBefore(*strObj, index);
  return res;
}

jint Java_java_lang_String_nativeCodePointCount__II(JNIEnv*, jstring jthis, jint beginIndex, jint endIndex) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  MString *strObj = reinterpret_cast<MString*>(jthis);
  jint res = StringNativeCodePointCount(*strObj, beginIndex, endIndex);
  return res;
}

jint Java_java_lang_String_nativeOffsetByCodePoint__II(
    JNIEnv*, jstring jthis, jint index, jint codePointOffset) {
  ScopedObjectAccess soa;
  if (UNLIKELY(jthis == nullptr)) {
    MRT_ThrowNewException("java/lang/NullPointerException",
                          "Attempt to invoke a virtual method on a null object reference");
    return -1;
  }
  MString *strObj = reinterpret_cast<MString*>(jthis);
  jint res = StringNativeOffsetByCodePoint(*strObj, index, codePointOffset);
  return res;
}
#endif

#ifdef __cplusplus
}
#endif
