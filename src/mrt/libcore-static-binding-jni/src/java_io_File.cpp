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
#include "java2c_rule.h"
#include "java_io_File.h"
#include "mstring_inline.h"
#include "chelper.h"
using namespace maplert;

template<typename Type>
uint32_t NormalizeConvert(uint32_t count, MString *&newObj, const MString &pathName){
  Type prevChar = 0;
  uint32_t index = 0;

  Type *newSrc = nullptr;
  bool startCopy = false;
  Type *src = reinterpret_cast<Type*>(pathName.GetContentsPtr());
  for (uint32_t i = 0; i < count; ++i) {
    const Type current = src[i];
    // Remove duplicate slashes.
    if ((current == '/' && prevChar == '/')) {
      if (newObj == nullptr) {
        newObj = MString::NewStringObject<Type>(count, [](MString&) { });
        newSrc = reinterpret_cast<Type*>(newObj->GetContentsPtr());
        size_t typeWidth = sizeof(Type);
        if (memcpy_s(newSrc, count * typeWidth, src, (i + 1) * typeWidth) != EOK) {
          LOG(ERROR) << "NormalizeConvert not return EOK";
          return 0;
        }
        startCopy = true;
        index = i;
      }
    } else if(startCopy) {
      newSrc[index++] = src[i];
    }
    prevChar = current;
  }
  // Omit the trailing slash, except when pathname == "/".
  if (prevChar == '/' && count > 1) {
    if (newObj == nullptr) {
      newObj = MString::NewStringObject<Type>(count, [](MString&) { });
      newSrc = reinterpret_cast<Type*>(newObj->GetContentsPtr());
      size_t typeWidth = sizeof(Type);
      if (memcpy_s(newSrc, count * typeWidth, src, (count - 1) * typeWidth) != EOK) {
        LOG(ERROR) << "NormalizeConvert not return EOK";
        return 0;
      }
      index = count - 1;
    } else {
      --index;
    }
  }
  uint32_t res = (index != 0) ? index : count;
  return res;
}

extern "C" jboolean Java_libcore_io_Linux_access__Ljava_lang_String_2I(const JNIEnv *env, jobject,
                                                                       const jstring javaPath, jint mode) {
  if (UNLIKELY(javaPath == nullptr) || UNLIKELY(env == nullptr)) {
    return JNI_FALSE;
  }
  std::string path = (reinterpret_cast<MString*>(javaPath))->GetChars();
  int rc = access(path.c_str(), mode);
  return static_cast<jboolean>(rc == 0);
}

#ifdef __cplusplus
extern "C" {
#endif

MRT_EXPORT jstring Native_Ljava_2Fio_2FUnixFileSystem_3B_7Cnormalize_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2FString_3B(
    __attribute__((unused))jobject jthis, jstring pathName) {
  maplert::Java2CRule j2cRule(INIT_ARGS);
  MString *pathNameObj = MString::JniCastNonNull(pathName);
  uint32_t count = pathNameObj->GetLength();
  uint32_t index = 0;
  MString *newObj = nullptr;
  if (pathNameObj->IsCompress()) {
    index = NormalizeConvert<uint8_t>(count, newObj, *pathNameObj);
    if (index != count) {
      newObj->SetCountValue((index << 1) | 1);
      j2cRule.Epilogue();
      return reinterpret_cast<jstring>(newObj);
    }
  } else {
    index = NormalizeConvert<uint16_t>(count, newObj, *pathNameObj);
    if (index != count) {
      newObj->SetCountValue(index << 1);
      j2cRule.Epilogue();
      return reinterpret_cast<jstring>(newObj);
    }
  }
  // if exception happen, it will unwinder directly.
  j2cRule.Epilogue();
  RC_LOCAL_INC_REF(pathName);
  return pathName;
}

#ifdef __cplusplus
}
#endif
