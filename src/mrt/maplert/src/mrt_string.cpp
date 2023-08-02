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

#include "mrt_string.h"
#include <unordered_set>
#include "jni.h"
#include "allocator/page_allocator.h"
#include "mstring_inline.h"
#include "base/low_mem_set.h"
#include "libs.h"
#include "exception/mrt_exception.h"
#include "literalstrname.h"
using namespace std;

namespace maplert {
enum ZeroCodeT {
  kZeroNotcode = 0,
  kZeroDocode
};

int GetMUtf8ByteCount(const uint16_t *utf16Raw, size_t charCount, ZeroCodeT zCode, int charSet);
void MUtf8Encode(uint8_t *utf8Res, const uint16_t *utf16Raw, size_t charCount, ZeroCodeT zCode, int charSet);
void MUtf8Decode(const uint8_t *utf8, size_t utf8Len, uint16_t *utf16Res);
uint32_t CountInMutfLen(const char *utf8, size_t utf8Len);
extern "C" char *MRT_GetStringContentsPtrRaw(jstring jstr) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  return reinterpret_cast<char*>(stringObj->GetContentsPtr());
}

extern "C" jchar *MRT_GetStringContentsPtrCopy(jstring jstr, jboolean *isCopy) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  if (stringObj->IsCompress()) {
    uint32_t len = stringObj->GetLength();
    jchar *chars = reinterpret_cast<jchar*>(malloc(len * sizeof(jchar) + 1));
    if (UNLIKELY(chars == nullptr)) {
      return nullptr;
    }
    const uint8_t *src = stringObj->GetContentsPtr();
    jchar *pos = chars;
    jchar *end = pos + len;
    while (pos < end) {
      *pos++ = *src++;
    }
    if (isCopy != nullptr) {
      *isCopy = JNI_TRUE;
    }
    return chars;
  } else {
    if (isCopy != nullptr) {
      *isCopy = JNI_FALSE;
    }
    return reinterpret_cast<jchar*>(stringObj->GetContentsPtr());
  }
}

extern "C" jstring MRT_NewHeapJStr(const jchar *ca, jint len, bool isJNI) {
  MString *res = MString::NewStringObject(ca, static_cast<uint32_t>(len), isJNI);
  if (res == nullptr) {
    return nullptr;
  }
  return res->AsJstring();
}

extern "C" jstring MCC_CStrToJStr(const char *ca, jint len) {
  MString *res = MString::NewStringObject(reinterpret_cast<const uint8_t*>(ca), static_cast<uint32_t>(len));
  return reinterpret_cast<jstring>(res);
}

extern "C" jstring CStrToJStr(const char *ca, jint len)
__attribute__((alias("MCC_CStrToJStr")));

MString *StringNewStringFromString(const MString &stringObj) {
  uint32_t length = stringObj.GetLength();
  uint8_t *contents = stringObj.GetContentsPtr();
  MString *res = nullptr;
  if (stringObj.IsCompress()) {
    res = MString::NewStringObject<uint8_t>(contents, length);
  } else {
    res = MString::NewStringObject<uint16_t>(reinterpret_cast<uint16_t*>(contents), length);
  }
  return res;
}

MString *StringNewStringFromSubstring(const MString &stringObj, int32_t start, uint32_t length) {
  MString *res = nullptr;
  const uint8_t *src = stringObj.GetContentsPtr();
  if (stringObj.IsCompress()) {
    res = MString::NewStringObject<uint8_t>(src + start, length);
  } else {
    const uint16_t *srcStart = reinterpret_cast<const uint16_t*>(src) + start;
    bool isCompress = false;
    if (stringObj.GetCountValue() < static_cast<uint32_t>(start)) {
      isCompress = false;
    } else {
      isCompress = MString::IsCompressChars<uint16_t>(srcStart, static_cast<uint32_t>(length));
    }
    res = isCompress ? MString::NewStringObject<uint16_t, uint8_t>(srcStart, length) :
                       MString::NewStringObject<uint16_t>(srcStart, length);
  }
  return res;
}

MString *StringNewStringFromCharArray(int32_t offset, uint32_t charCount, const MArray &arrayObj) {
  uint16_t *arrayData = reinterpret_cast<uint16_t*>(arrayObj.ConvertToCArray());
  uint16_t *content = arrayData + offset;
  const bool compressible = MString::IsCompressChars<uint16_t>(content, charCount);
  MString *res = compressible ? MString::NewStringObject<uint16_t, uint8_t>(content, charCount) :
                                MString::NewStringObject<uint16_t>(content, charCount);
  return res;
}

MString *StringNewStringFromByteArray(const MArray &arrayObj, int32_t highByteT, int32_t offset, uint32_t byteLength) {
  uint8_t *arrayData = reinterpret_cast<uint8_t*>(arrayObj.ConvertToCArray());
  uint8_t *content = arrayData + offset;
  const bool compressible = (highByteT == 0) && MString::IsCompressChars<uint8_t>(content, byteLength);
  MString *res = nullptr;
  if (compressible) {
    res = MString::NewStringObject<uint8_t>(content, byteLength);
  } else {
    res = MString::NewStringObject<uint16_t>(byteLength, [&](MString &stringObj) {
      uint32_t highByte = (static_cast<uint32_t>(highByteT)) << 8; // shift one byte to obtain highbyte
      uint16_t *resDst = reinterpret_cast<uint16_t*>(stringObj.GetContentsPtr());
      for (uint32_t i = 0; i < byteLength; ++i) {
        resDst[i] = highByte | content[i];
      }
    });
  }
  return res;
}

template<typename srcType, typename dstType>
static inline void MStringDoReplace(const MString &srcStrObj, const MString &dstStrObj, uint16_t oldChar,
                                    uint16_t newChar, uint32_t len) {
  srcType *src = reinterpret_cast<srcType*>(srcStrObj.GetContentsPtr());
  dstType *dst = reinterpret_cast<dstType*>(dstStrObj.GetContentsPtr());
  for (uint32_t i = 0; i < len; ++i) {
    srcType c = src[i];
    dst[i] = (c == oldChar) ? newChar : c;
  }
}

MString *JStringDoReplace(const MString &stringObj, uint16_t oldChar, uint16_t newChar) {
  bool srcIsCompressible = stringObj.IsCompress();
  uint32_t srcLength = stringObj.GetLength();
  uint8_t *srcValue = stringObj.GetContentsPtr();
  bool compressible = MString::GetCompressFlag() && MString::IsCompressChar(newChar) && (srcIsCompressible ||
                      (!MString::IsCompressChar(oldChar) &&
                      MString::IsCompressCharsExcept(reinterpret_cast<uint16_t*>(srcValue), srcLength, oldChar)));
  MString *res = nullptr;
  if (compressible) {
    res = MString::NewStringObject<uint8_t>(srcLength, [&](MString &newStringObj) {
      if (srcIsCompressible) {
        MStringDoReplace<uint8_t, uint8_t>(stringObj, newStringObj, oldChar, newChar, srcLength);
      } else {
        MStringDoReplace<uint16_t, uint8_t>(stringObj, newStringObj, oldChar, newChar, srcLength);
      }
    });
  } else {
    res = MString::NewStringObject<uint16_t>(srcLength, [&](MString &newStringObj) {
      if (srcIsCompressible) {
        MStringDoReplace<uint8_t, uint16_t>(stringObj, newStringObj, oldChar, newChar, srcLength);
      } else {
        MStringDoReplace<uint16_t, uint16_t>(stringObj, newStringObj, oldChar, newChar, srcLength);
      }
    });
  }
  return res;
}

static void GetCompressedNewValue(const bool stringCompress, const MString &stringObj, uint32_t stringLen,
                                  uint32_t string1Len, uint16_t *newValue) {
  if (stringCompress) {
    uint8_t *value = stringObj.GetContentsPtr();
    for (uint32_t i = 0; i < stringLen; ++i) {
      newValue[i + string1Len] = value[i];
    }
  } else {
    const int doubleSize = 2;
    uint16_t *value = reinterpret_cast<uint16_t*>(stringObj.GetContentsPtr());
    uint32_t cpyLen = 0;
    if (stringLen <= UINT32_MAX / doubleSize) {
      cpyLen = stringLen * doubleSize;
    } else {
      LOG(FATAL) << "stringLen * doubleSize >  UINT32_MAX" << maple::endl;
    }
    if (memcpy_s(newValue + string1Len, cpyLen, value, cpyLen) != EOK) {
      LOG(ERROR) << "memcpy_s() not return 0 in GetCompressedNewValue()" << maple::endl;
    }
  }
}

// This Function is to concat two String, the String maybe compress or uncompress, when it's compress,
// the String is saved as uint8_t and when it's uncompress, it's saved double the storage as uint16_t
MString *StringConcat(MString &stringObj1, MString &stringObj2) {
  uint32_t string1Count = stringObj1.GetCountValue();
  uint32_t string2Count = stringObj2.GetCountValue();
  uint32_t string1Len = string1Count >> 1;
  uint32_t string2Len = string2Count >> 1;
  if (string1Len == 0) {
    RC_LOCAL_INC_REF(&stringObj2);
    return &stringObj2;
  }
  if (string2Len == 0) {
    RC_LOCAL_INC_REF(&stringObj1);
    return &stringObj1;
  }

  const bool string1Compress = MString::IsCompressedFromCount(string1Count);
  const bool string2Compress = MString::IsCompressedFromCount(string2Count);
  const bool compressible = MString::GetCompressFlag() && string1Compress && string2Compress;
  uint32_t len = string1Len + string2Len;
  MString *res = nullptr;
  if (compressible) {
    res = MString::NewStringObject<uint8_t>(len, [&](MString &newStringObj) {
      uint8_t *newValue = newStringObj.GetContentsPtr();
      uint8_t *value1 = stringObj1.GetContentsPtr();
      uint8_t *value2 = stringObj2.GetContentsPtr();
      errno_t returnValueOfMemcpyS1 = memcpy_s(newValue, string1Len, value1, string1Len);
      errno_t returnValueOfMemcpyS2 = memcpy_s(newValue + string1Len, string2Len, value2, string2Len);
      if (returnValueOfMemcpyS1 != EOK || returnValueOfMemcpyS2 != EOK) {
        LOG(ERROR) << "memcpy_s() not return 0 in StringConcat()" << maple::endl;
      }
    });
  } else {
    res = MString::NewStringObject<uint16_t>(len, [&](MString &newStringObj) {
      uint16_t *newValue = reinterpret_cast<uint16_t*>(newStringObj.GetContentsPtr());
      GetCompressedNewValue(string1Compress, stringObj1, string1Len, 0, newValue);
      GetCompressedNewValue(string2Compress, stringObj2, string2Len, string1Len, newValue);
    });
  }
  return res;
}

template<typename MemoryType>
static inline int32_t FastIndexOf(const MemoryType &chars, int32_t ch, int32_t start, int32_t length) {
  const MemoryType *p = &chars + start;
  const MemoryType *end = &chars + length;
  while (p < end) {
    if (*p++ == ch) {
      return static_cast<int32_t>((p - 1) - &chars);
    }
  }
  return -1;
}

int32_t StringFastIndexOf(const MString &stringObj, int32_t ch, int32_t start) {
  uint32_t count = stringObj.GetCountValue();
  int32_t len = static_cast<int32_t>(count >> 1);
  if (start < 0) {
    start = 0;
  } else if (start > len) {
    start = len;
  }

  if (MString::IsCompressedFromCount(count)) {
    uint8_t *value = stringObj.GetContentsPtr();
    return FastIndexOf<uint8_t>(*value, ch, start, len);
  } else {
    uint16_t *value = reinterpret_cast<uint16_t*>(stringObj.GetContentsPtr());
    return FastIndexOf<uint16_t>(*value, ch, start, len);
  }
}

// Create and return Java string  from c string (in UTF-8)
MString *NewStringUTF(const char *kCStr, size_t cStrLen) {
  MString *res = MString::NewStringObject<uint8_t, uint8_t>(
      reinterpret_cast<const uint8_t*>(kCStr), static_cast<uint32_t>(cStrLen));
  return res;
}

// Create and return Java string from UTF-16/UTF-8
MString *NewStringFromUTF16(const std::string &str) {
  return MString::NewStringObjectFromUtf16(str);
}

extern "C" void MRT_ReleaseStringUTFChars(jstring jstr __attribute__((unused)), const char *chars) {
  free(const_cast<char*>(chars));
  return;
}

extern "C" void MRT_ReleaseStringChars(jstring jstr, const jchar *chars) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  jchar *contents = reinterpret_cast<jchar*>(stringObj->GetContentsPtr());
  if (stringObj->IsCompress() || chars != contents) {
    free(const_cast<jchar*>(chars));
  }
}

extern "C" jint MRT_StringGetStringLength(jstring jstr) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  return stringObj->GetLength();
}

size_t MRT_GetStringObjectSize(jstring jstr) {
  MString *stringObj = MString::JniCast(jstr);
  return stringObj == nullptr ? MString::GetStringBaseSize() : stringObj->GetStringSize();
}

MArray *MStringToBytes(const MString &stringObj, int32_t offset, int32_t length, uint16_t maxValidChar, int compress) {
  char *src = reinterpret_cast<char*>(stringObj.GetContentsPtr());
  uint32_t srcContentSize = stringObj.GetLength();
  if (UNLIKELY(offset < 0 || length < 0 || static_cast<uint32_t>(offset + length) > srcContentSize)) {
    LOG(FATAL) << "the index to access stringObj is illegal" << maple::endl;
    return nullptr;
  }
  MArray *dst = MArray::NewPrimitiveArray<kElemByte>(static_cast<uint32_t>(length),
      *WellKnown::GetPrimitiveArrayClass(maple::Primitive::kByte));
  int8_t *buf = reinterpret_cast<int8_t*>(dst->ConvertToCArray());
  int8_t *end = buf + length;
  uint16_t tmp = 0;
  if (compress != 0) {
    src = src + offset;
    while (buf < end) {
      tmp = *src++;
      tmp = (tmp > maxValidChar) ? '?' : tmp;
      *buf++ = static_cast<int8_t>(tmp);
    }
  } else {
    src = src + static_cast<int64_t>(offset) * 2; // not compress, offset should be twice
    while (buf < end) {
      tmp = *(reinterpret_cast<uint16_t*>(src));
      src += 2; // uint16_t is u16, when read char*, char ptr need to move 2 at a time
      tmp = (tmp > maxValidChar) ? '?' : tmp;
      *buf++ = static_cast<int8_t>(tmp);
    }
  }
  return dst;
}

// MUTF-8 format string start
MArray *MStringToMutf8(const MString &stringObj, int32_t offset, int32_t length, int compress) {
  char *src = reinterpret_cast<char*>(stringObj.GetContentsPtr());
  MArray *dst = nullptr;
  uint32_t srcContentSize = stringObj.GetLength();
  if (UNLIKELY(offset < 0 || length < 0 || static_cast<uint32_t>(offset + length) > srcContentSize)) {
    LOG(FATAL) << "the index to access stringObj is illegal" << maple::endl;
    return nullptr;
  }
  if (compress != 0) {
    src += offset;
    dst = MArray::NewPrimitiveArray<kElemByte>(static_cast<uint32_t>(length),
        *WellKnown::GetPrimitiveArrayClass(maple::Primitive::kByte));
    int8_t *buf = reinterpret_cast<int8_t*>(dst->ConvertToCArray());
    for (int i = 0; i < length; ++i) {
      buf[i] = src[i];
    }
  } else {
    uint16_t *pos = reinterpret_cast<uint16_t*>(src) + offset;
    int count = GetMUtf8ByteCount(pos, length, kZeroNotcode, 1);
    dst = MArray::NewPrimitiveArray<kElemByte>(static_cast<uint32_t>(count),
        *WellKnown::GetPrimitiveArrayClass(maple::Primitive::kByte));
    uint8_t *buf = reinterpret_cast<uint8_t*>(dst->ConvertToCArray());
    MUtf8Encode(buf, pos, static_cast<uint32_t>(length), kZeroNotcode, 1);
  }
  return dst;
}

extern "C" jstring MRT_NewStringMUTF(const char *inMutf, size_t inMutfLen, bool isJNI) {
  MString *res = nullptr;
  uint32_t length = CountInMutfLen(inMutf, inMutfLen);
  const bool kCompressible = MString::GetCompressFlag() && (length > 0) && (inMutfLen == length);
  if (kCompressible) {
    res = MString::NewStringObject<uint8_t>(reinterpret_cast<const uint8_t*>(inMutf), length, isJNI);
    if (res == nullptr) {
      return nullptr;
    }
  } else {
    res = MString::NewStringObject<uint16_t>(length, [&](MString &newStringObj) {
      uint16_t *outUnicode = reinterpret_cast<uint16_t*>(newStringObj.GetContentsPtr());
      MUtf8Decode(reinterpret_cast<const uint8_t*>(inMutf), inMutfLen, outUnicode);
    }, isJNI);
  }
  return res->AsJstring();
}

extern "C" jsize MRT_GetStringMUTFLength(jstring jstr) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  uint32_t length = stringObj->GetLength();
  uint16_t *utf16Raw = nullptr;
  jint res;
  if (stringObj->IsCompress()) {
    res = static_cast<int>(length);
  } else {
    utf16Raw = reinterpret_cast<uint16_t*>(stringObj->GetContentsPtr());
    res = GetMUtf8ByteCount(utf16Raw, length, kZeroDocode, 0);
  }
  return res;
}

extern "C" char *MRT_GetStringMUTFChars(jstring jstr, jboolean *isCopy) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  if (isCopy != nullptr) {
    *isCopy = JNI_TRUE;
  }
  uint32_t count = stringObj->GetCountValue();
  char *res = nullptr;
  uint32_t len = count >> 1;
  if (MString::IsCompressedFromCount(count)) {
    res = reinterpret_cast<char*>(malloc((len + 1) * sizeof(char)));
    if (UNLIKELY(res == nullptr)) {
      if (isCopy != nullptr) {
        *isCopy = JNI_FALSE;
      }
      return nullptr;
    }
    res[len] = 0;
    const char *data = reinterpret_cast<char*>(stringObj->GetContentsPtr());
    for (uint32_t i = 0; i < len; ++i) {
      res[i] = data[i];
    }
  } else {
    uint16_t *utf16Raw = reinterpret_cast<uint16_t*>(stringObj->GetContentsPtr());
    uint32_t length = static_cast<uint32_t>(GetMUtf8ByteCount(utf16Raw, len, kZeroDocode, 0));
    uint8_t *mutfStr = nullptr;
    mutfStr = reinterpret_cast<uint8_t*>(malloc((length + 1) * sizeof(uint8_t)));
    if (UNLIKELY(mutfStr == nullptr)) {
      if (isCopy != nullptr) {
        *isCopy = JNI_FALSE;
      }
      return nullptr;
    }
    mutfStr[length] = 0;
    if (length == 0) {
      return reinterpret_cast<char*>(mutfStr);
    }
    MUtf8Encode(mutfStr, utf16Raw, len, kZeroDocode, 0);
    res = reinterpret_cast<char*>(mutfStr);
  }
  return res;
}

extern "C" void MRT_GetStringMUTFRegion(jstring jstr, jsize start, jsize length, char *buf) {
  MString *stringObj = MString::JniCastNonNull(jstr);
  if (UNLIKELY(buf == nullptr)) {
    return;
  }
  if (stringObj->IsCompress()) {
    const char *data = reinterpret_cast<char*>(stringObj->GetContentsPtr());
    data = data + start;
    for (jint i = 0; i < length; ++i) {
      buf[i] = data[i];
    }
  } else {
    char *src = reinterpret_cast<char*>(stringObj->GetContentsPtr());
    uint16_t *utf16Raw = nullptr;
    utf16Raw = reinterpret_cast<uint16_t*>(src) + start;
    uint8_t *mutfStr = reinterpret_cast<uint8_t*>(buf);
    if (length == 0) {
      return;
    }
    MUtf8Encode(mutfStr, utf16Raw, length, kZeroDocode, 0);
  }
} // MUTF-8 format string end

// literals in MFile (with only jstring_payload) cannot be inserted into MapleStringPool directly,
// instead, a new string in PERM-space will be created.
// thus, all jstring_payload_p points to the middle of a valid java string object. the begin of
// the MString object can be obtained by a negtive offset from jstring_payload_p
const int kZygotePoolBucketNum = 997; // prime number
const int kAppPoolBucketNum = 113; // prime number
const int kPoolNum = 47;  // prime number
const int kClassNameNum = 1024;
static maple::SpinLock mtx[kPoolNum];
using  MapleStringPool = maple::LowMemSet<MStringRef, MStringHashEqual>;
static MapleStringPool zygoteStringPool[kPoolNum];
static MapleStringPool *appStringAppPool = nullptr;

enum SweepState : uint8_t {
  kClean,
  kReadyToSweep
};

using MUnorderedMap = std::unordered_map<char*, MStringRef, std::hash<char*>,
    std::equal_to<char*>, StdContainerAllocator<std::pair<char * const, MStringRef>, kClassNameStringPool>>;
static MUnorderedMap classnameMap(kClassNameNum);
static maple::SpinLock mtxForClassname;
uint8_t sweepingStateForClassName;
static size_t deadClassNameNum = 0;

void RemoveDeadClassNameFromPoolLocked(MUnorderedMap &pool, uint8_t &state) {
  if (LIKELY(state != static_cast<uint8_t>(kReadyToSweep))) {
    if (UNLIKELY(state != static_cast<uint8_t>(kClean))) {
      LOG(FATAL) << "[StringPool] Concurrent Sweeping ilegal state" << maple::endl;
    }
    return;
  }

  size_t curDead = 0;
  for (auto it = pool.begin(); it != pool.end();) {
    address_t addr = (it->second).GetRef();
    if (IS_HEAP_OBJ(addr) && MRT_IsGarbage(addr)) {
      it = pool.erase(it);
      ++curDead;
    } else {
      ++it;
    }
  }
  deadClassNameNum += curDead;
  state = static_cast<uint8_t>(kClean);
}

MString *NewStringUtfFromPoolForClassName(const MClass &classObj) {
  char *cStr = classObj.GetName();
  if (UNLIKELY(cStr == nullptr)) {
    return nullptr;
  }
  if (!MRT_EnterSaferegion()) {
    __MRT_ASSERT(false, "calling NewStringUtfFromPoolForClassName from saferegion");
  }
  mtxForClassname.Lock();
  (void)MRT_LeaveSaferegion();
  RemoveDeadClassNameFromPoolLocked(classnameMap, sweepingStateForClassName);
  auto it = classnameMap.find(cStr);
  MStringRef newStrObj;
  if (it == classnameMap.end()) {
    mtxForClassname.Unlock();
    string binaryName;
    classObj.GetBinaryName(binaryName);
    newStrObj.SetRef(NewStringFromUTF16(binaryName));
    ScopedHandles sHandles;
    ObjHandle<MStringRef, false> stringInst(newStrObj.GetRef());
    if (!MRT_EnterSaferegion()) {
      __MRT_ASSERT(false, "calling NewStringUtfFromPoolForClassName from saferegion");
    }
    mtxForClassname.Lock();
    (void)MRT_LeaveSaferegion();
    it = classnameMap.find(cStr);
  }

  if (it == classnameMap.end()) {
    RC_LOCAL_INC_REF(reinterpret_cast<MString*>(newStrObj.GetRef()));
    classnameMap[cStr] = newStrObj;
    mtxForClassname.Unlock();
    return reinterpret_cast<MString*>(newStrObj.GetRef());
  } else {
    // When we reuse a string from pool, the string may have been dead in java world,
    // we should set it as renewed to make concurrent marking happy.
    if (reinterpret_cast<MString*>(newStrObj.GetRef()) != nullptr) {
      RC_LOCAL_DEC_REF(reinterpret_cast<MString*>(newStrObj.GetRef()));
    }
    MRT_PreRenewObject((it->second).GetRef());
    RC_LOCAL_INC_REF(reinterpret_cast<MString*>((it->second).GetRef()));
    mtxForClassname.Unlock();
    return reinterpret_cast<MString*>((it->second).GetRef());
  }
}

void CreateAppStringPool() {
  if (appStringAppPool == nullptr) {
    appStringAppPool = new (std::nothrow) MapleStringPool[kPoolNum];
    __MRT_ASSERT(appStringAppPool != nullptr, "fail to allocate app const string pool!");
    for (int i = 0; i < kPoolNum; ++i) {
      appStringAppPool[i].Reserve(kAppPoolBucketNum);
    }
  }
}

int InitializeMapleStringPool() {
  for (int i = 0; i < kPoolNum; ++i) {
    zygoteStringPool[i].Reserve(kZygotePoolBucketNum);
  }
  sweepingStateForClassName = static_cast<uint8_t>(kClean);
  return 0;
}

static int initialize = InitializeMapleStringPool();

void StringPrepareConcurrentSweeping() {
  sweepingStateForClassName = static_cast<uint8_t>(kReadyToSweep);
  deadClassNameNum = 0;
}

size_t ConstStringPoolSize(bool literal) {
  size_t cspSize = 0;
  auto visitor = [literal, &cspSize](const MStringRef strRef) {
    MString *jstr = reinterpret_cast<MString*>(strRef.GetRef());
    if (literal == jstr->IsLiteral()) {
      cspSize += jstr->GetStringSize();
    }
  };
  for (int i = 0; i < kPoolNum; ++i) {
    maple::SpinAutoLock guard(mtx[i]);
    zygoteStringPool[i].ForEach(visitor);
    if (appStringAppPool != nullptr) {
      appStringAppPool[i].ForEach(visitor);
    }
  }
  return cspSize;
}

size_t ConstStringPoolNum(bool literal) {
  size_t cspNum = 0;
  auto visitor = [literal, &cspNum](const MStringRef strRef) {
    MString *s = reinterpret_cast<MString*>(strRef.GetRef());
    if (literal == s->IsLiteral()) {
      ++cspNum;
    }
  };
  for (int i = 0; i < kPoolNum; ++i) {
    maple::SpinAutoLock guard(mtx[i]);
    zygoteStringPool[i].ForEach(visitor);
    if (appStringAppPool != nullptr) {
      appStringAppPool[i].ForEach(visitor);
    }
  }
  return cspNum;
}

size_t ConstStringAppPoolNum(bool literal) {
  size_t cspNum = 0;
  if (appStringAppPool != nullptr) {
    for (int i = 0; i < kPoolNum; ++i) {
      maple::SpinAutoLock guard(mtx[i]);
      appStringAppPool[i].ForEach([literal, &cspNum](const MStringRef strRef) {
        MString *s = reinterpret_cast<MString*>(strRef.GetRef());
        if (literal == s->IsLiteral()) {
          ++cspNum;
        }
      });
    }
  }
  return cspNum;
}

void DumpJString(std::ostream &os, const MString &stringObj, bool dumpName) {
  uint32_t len = stringObj.GetLength();
  const uint8_t *data = stringObj.GetContentsPtr();
  std::ios::fmtflags f(os.flags());

  if (dumpName) {
    if (stringObj.IsCompress()) {
      // need to convert to utf16 to calculate LiteralStrName
      int constexpr maxLiteralLength = 1024; // up to 2KB local stack
      if (len > maxLiteralLength) {
        return; // just skip it
      }

      uint16_t utf16Raw[maxLiteralLength] = { 0 };
      for (uint32_t i = 0; i < len; ++i) {
        utf16Raw[i] = data[i];
      }
      os << LiteralStrName::GetLiteralStrName(reinterpret_cast<const uint8_t*>(utf16Raw), len << 1);
    } else {
      os << LiteralStrName::GetLiteralStrName(data, len << 1);
    }
  } else {
    if (!stringObj.IsCompress()) {
      len = len << 1;
    }
    for (uint32_t i = 0; i < len; ++i) {
      os << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(data[i]);
    }
  }
  os << "\n";
  os.flags(f);
}

void DumpConstStringPool(std::ostream &os, bool literal) {
  auto visitor = [literal, &os](const MStringRef strRef) {
    MString *s = reinterpret_cast<MString*>(strRef.GetRef());
    if (literal == s->IsLiteral()) {
      DumpJString(os, *s, true); // uuid name only
    }
  };
  for (int i = 0; i < kPoolNum; ++i) {
    maple::SpinAutoLock guard(mtx[i]);
    zygoteStringPool[i].ForEach(visitor);
    if (appStringAppPool != nullptr) {
      appStringAppPool[i].ForEach(visitor);
    }
  }
}

void VisitStringPool(const RefVisitor &visitor) {
  for (auto iter = classnameMap.begin(); iter != classnameMap.end(); ++iter) {
    visitor(reinterpret_cast<address_t&>((iter->second).GetRawRef())); // pass direct root ref for moving gc
  }
}

void MRT_ScanLiteralPoolRoots(function<void(uintptr_t)> visitRoot) {
  auto visitor = [&visitRoot](const MStringRef strRef) {
    MString *s = reinterpret_cast<MString*>(strRef.GetRef());
    if (s->IsLiteral()) {
      visitRoot(reinterpret_cast<address_t>(s));
    }
  };
  for (int i = 0; i < kPoolNum; ++i) {
    zygoteStringPool[i].ForEach(visitor);
    if (appStringAppPool != nullptr) {
      appStringAppPool[i].ForEach(visitor);
    }
  }
}

size_t ConcurrentSweepDeadStrings(maplert::MplThreadPool*) {
  maple::SpinAutoLock guard(mtxForClassname);
  RemoveDeadClassNameFromPoolLocked(classnameMap, sweepingStateForClassName);
  return deadClassNameNum;
}

size_t RemoveDeadStringFromPool() {
  size_t count = 0;
  for (auto it = classnameMap.begin(); it != classnameMap.end();) {
    address_t addr = (it->second).GetRef();
    // dead string might already cleared in early sweep
    // object already cleared, only fast check is enough
    if ((IS_HEAP_OBJ(addr) && MRT_IsGarbage(addr))) {
      it = classnameMap.erase(it);
      MRT_DecRefUnsync(addr);
      ++count;
    } else {
      ++it;
    }
  }
  return count;
}

// non-literal strings using this interface, it always returns a off-heap MString.
MString *GetOrInsertStringPool(MString &stringObj) {
  uint32_t count = stringObj.GetCountValue();
  const uint8_t *data = stringObj.GetContentsPtr();

  uint32_t hash = stringObj.GetHash();
  if (hash == 0) {
    hash = LiteralStrName::CalculateHash(reinterpret_cast<const char16_t*>(data),
                                         count >> 1, MString::IsCompressedFromCount(count));
    // this is not thead safe
    stringObj.SetHash(hash);
  }

  uint32_t num = hash % kPoolNum;
  maple::SpinAutoLock guard(mtx[num]);
  // try to find existing string first
  MStringRef retStrObj;
  auto iter = zygoteStringPool[num].Find(&stringObj);
  if (iter != zygoteStringPool[num].End()) { // found
    retStrObj = zygoteStringPool[num].Element(iter);
    return reinterpret_cast<MString*>(retStrObj.GetRef());
  } else if (appStringAppPool != nullptr) {
    iter = appStringAppPool[num].Find(&stringObj);
    if (iter != appStringAppPool[num].End()) { // found
      retStrObj = appStringAppPool[num].Element(iter);
      return reinterpret_cast<MString*>(retStrObj.GetRef());
    }
  }

  // not found: create a new stringObj in perm-spaceto and insert
  uint32_t strObjSize = stringObj.GetStringSize();
  // is there possibly a deadlock here (with mtx[num].lock)?
  address_t newStringObj = MRT_AllocFromPerm(strObjSize);
  if (UNLIKELY(newStringObj == 0)) {
    return nullptr;
  }
  retStrObj.SetRef(newStringObj);
  char *pDstStr = reinterpret_cast<char*>(retStrObj.GetRef());
  char *pSrcStr = reinterpret_cast<char*>(&stringObj);
  for (uint32_t i = 0; i < strObjSize; ++i) {
    pDstStr[i] = pSrcStr[i];
  }

  if (appStringAppPool != nullptr) {
    appStringAppPool[num].Insert(retStrObj);
  } else {
    zygoteStringPool[num].Insert(retStrObj);
  }
  mtx[num].Unlock();
  return reinterpret_cast<MString*>(retStrObj.GetRef());
}
// Note:only used to insert literal (stored in static fields) into pool,
//      currently only used by mpl-linker
// literal might be a full MString object, or only the jstring_payload
MString *GetOrInsertLiteral(MString &literalObj) {
  // all literals already has hash set ready.
  uint32_t num = literalObj.GetHash() % kPoolNum;
  mtx[num].Lock();
  literalObj.SetStringClass();
  MStringRef retStrObj;
  auto iter = zygoteStringPool[num].Find(&literalObj);
  if (iter != zygoteStringPool[num].End()) { // found
    retStrObj = zygoteStringPool[num].Element(iter);
    mtx[num].Unlock();
    return reinterpret_cast<MString*>(retStrObj.GetRef());
  } else if (appStringAppPool != nullptr) {
    iter = appStringAppPool[num].Find(&literalObj);
    if (iter != appStringAppPool[num].End()) {
      retStrObj = appStringAppPool[num].Element(iter);
      mtx[num].Unlock();
      return reinterpret_cast<MString*>(retStrObj.GetRef());
    }
  }

  retStrObj.SetRef(&literalObj);
  if (appStringAppPool != nullptr) {
    appStringAppPool[num].Insert(retStrObj);
  } else {
    zygoteStringPool[num].Insert(retStrObj);
  }
  mtx[num].Unlock();
  return reinterpret_cast<MString*>(retStrObj.GetRef());
}

// Note: used to insert literal into pool, only for compiler generated code
extern "C" jstring MCC_GetOrInsertLiteral(jstring literal) {
  MString *literalObj = reinterpret_cast<MString*>(literal);
  DCHECK(literalObj != nullptr);
  jstring retJstr = reinterpret_cast<jstring>(GetOrInsertLiteral(*literalObj));
  return retJstr;
}

void RemoveStringFromPool(MString &stringObj) {
  uint32_t judgeHash = stringObj.GetHash();
  uint32_t num = judgeHash % kPoolNum;
  mtx[num].Lock();
  auto iter = zygoteStringPool[num].Find(&stringObj);
  if (iter != zygoteStringPool[num].End()) {
    zygoteStringPool[num].Erase(iter);
  } else if (appStringAppPool != nullptr) {
    iter = appStringAppPool[num].Find(&stringObj);
    if (iter != appStringAppPool[num].End()) { // found
      appStringAppPool[num].Erase(iter);
    }
  }
  mtx[num].Unlock();
  return;
} // String pool function interface end

// String compress start
int GetMUtf8ByteCount(const uint16_t *utf16Raw, size_t charCount, ZeroCodeT zCode, int charSet) {
  DCHECK(utf16Raw != nullptr);
  int res = 0;
  for (size_t i = 0; i < charCount; ++i) {
    uint16_t ch = utf16Raw[i];
    if ((!static_cast<bool>(zCode) || ch != 0) && ch <= 0x7f) {
      ++res;
    } else if (ch <= 0x7ff) {
      res += 2;
    } else {
      if (!charSet) {
        if ((ch >= 0xd800 && ch <= 0xdbff) && (i < charCount - 1)) {
          uint16_t ch1 = utf16Raw[i + 1];
          if (ch1 >= 0xdc00 && ch1 <= 0xdfff) {
            ++i;
            res += 4;
            continue;
          }
        }
        res += 3;
      } else {
        if ((ch & 0xfffff800) == 0xd800) {
          uint16_t ch1 = (i < charCount - 1) ? utf16Raw[i + 1] : 0;
          if (!((ch & 0x400) == 0) || !((ch1 & 0x400) != 0)) {
            ++res;
            continue;
          }
          ++i;
          res += 4;
        } else {
          res += 3;
        }
      }
    }
  }
  return res;
}

const int kCodepointOffset1 = 6; // U+0080 - U+07FF   110xxxxx 10xxxxxx
const int kCodepointOffset2 = 12; // U+0800 - U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
const int kCodepointOffset3 = 18; // U+10000- U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
const int kCodeAfterMinusOffset = 10; // codepoint equals itself minus 0x10000
const int kUtf16Bits = 16;  // UTF_16 16 bits
void MUtf8Encode(uint8_t *utf8Res, const uint16_t *utf16Raw, size_t charCount, ZeroCodeT zCode, int charSet) {
  DCHECK(utf16Raw != nullptr);
  DCHECK(utf8Res != nullptr);
  uint32_t offset = 0;
  for (size_t i = 0; i < charCount; ++i) {
    uint16_t ch = utf16Raw[i];
    if ((!static_cast<bool>(zCode) || ch != 0) && ch <= 0x7f) {
      utf8Res[offset++] = ch;
    } else if (ch <= 0x7ff) {
      utf8Res[offset++] = (0xc0 | (0x1f & (ch >> kCodepointOffset1)));
      utf8Res[offset++] = (0x80 | (0x3f & ch));
    } else {
      if (!charSet) {
        if ((ch >= 0xd800 && ch <= 0xdbff) && (i < charCount - 1)) {
          uint16_t ch1 = utf16Raw[i + 1];
          if (ch1 >= 0xdc00 && ch1 <= 0xdfff) {
            i++;
            const uint32_t tmp = (ch << kCodeAfterMinusOffset) + ch1 - 0x035fdc00;
            utf8Res[offset++] = (tmp >> kCodepointOffset3) | 0xf0;
            utf8Res[offset++] = ((tmp >> kCodepointOffset2) &  0x3f) | 0x80;
            utf8Res[offset++] = ((tmp >> kCodepointOffset1) & 0x3f) | 0x80;
            utf8Res[offset++] = (tmp & 0x3f) | 0x80;
            continue;
          }
        }
        utf8Res[offset++] = (0xe0 | (0x0f & (ch >> kCodepointOffset2)));
        utf8Res[offset++] = (0x80 | (0x3f & (ch >> kCodepointOffset1)));
        utf8Res[offset++] = (0x80 | (0x3f & ch));
      } else {
        if ((ch & 0xfffff800) == 0xd800) {
          uint16_t ch1 = (i < charCount - 1) ? utf16Raw[i + 1] : 0;
          if (!((ch & 0x400) == 0) || !((ch1 & 0x400) != 0)) {
            utf8Res[offset++] = 0x3f;
            continue;
          }
          ++i;
          const uint32_t tmp = (ch << kCodeAfterMinusOffset) + ch1 - 0x035fdc00;
          utf8Res[offset++] = (tmp >> kCodepointOffset3) | 0xf0;
          utf8Res[offset++] = ((tmp >> kCodepointOffset2) &  0x3f) | 0x80;
          utf8Res[offset++] = ((tmp >> kCodepointOffset1) & 0x3f) | 0x80;
          utf8Res[offset++] = (tmp & 0x3f) | 0x80;
        } else {
          utf8Res[offset++] = (0xe0 | (0x0f & (ch >> kCodepointOffset2)));
          utf8Res[offset++] = (0x80 | (0x3f & (ch >> kCodepointOffset1)));
          utf8Res[offset++] = (0x80 | (0x3f & ch));
        }
      }
    }
  }
}

uint32_t CountInMutfLen(const char *utf8, size_t utf8Len) {
  DCHECK(utf8 != nullptr);
  size_t count = 0;
  uint32_t res = 0;
  while (true) {
    uint8_t ch = utf8[count] & 0xff;
    ++count;
    if (count > utf8Len) {
      return res;
    }
    if (ch < 0x80) {
      ++res;
    } else if ((ch & 0xe0) == 0xc0) {
      ++count;
      ++res;
    } else if ((ch & 0xf0) == 0xe0) {
      count += 2;
      ++res;
    } else {
      count += 3;
      res += 2;
    }
  }
}

void MUtf8Decode(const uint8_t *utf8, size_t utf8Len, uint16_t *utf16Res) {
  DCHECK(utf8 != nullptr);
  DCHECK(utf16Res != nullptr);
  size_t count = 0;
  int outCount = 0;
  while (true) {
    uint8_t ch = utf8[count++] & 0xff;
    if (count > utf8Len) {
      return;
    }
    if (ch < 0x80) {
      utf16Res[outCount++] = ch;
    } else if ((ch & 0xe0) == 0xc0) {
      uint8_t sch = utf8[count++] & 0xff;
      utf16Res[outCount++] = (static_cast<uint16_t>(ch & 0x1f) << kCodepointOffset1) | (sch & 0x3f);
    } else if ((ch & 0xf0) == 0xe0) {
      uint8_t sch = utf8[count++] & 0xff;
      uint8_t tch = utf8[count++] & 0xff;
      utf16Res[outCount++] = (static_cast<uint16_t>(ch & 0x0f) << kCodepointOffset2) |
          (static_cast<uint16_t>(sch & 0x3f) << kCodepointOffset1) | (tch & 0x3f);
    } else {
      uint8_t sch = utf8[count++] & 0xff;
      uint8_t tch = utf8[count++] & 0xff;
      uint8_t fch = utf8[count++] & 0xff;
      const uint32_t tmp = ((ch & 0x0f) << kCodepointOffset3) | ((sch & 0x3f) << kCodepointOffset2) |
                           ((tch & 0x3f) << kCodepointOffset1) | (fch & 0x3f);
      uint32_t pair = 0;
      pair |= ((tmp >> kCodeAfterMinusOffset) + 0xd7c0) & 0xffff;
      pair |= ((tmp & 0x03ff) + 0xdc00) << kUtf16Bits;
      utf16Res[outCount++] = static_cast<uint16_t>(pair & 0xffff);
      utf16Res[outCount++] = static_cast<uint16_t>(pair >> kUtf16Bits);
    }
  }
}

extern "C" bool MRT_IsStrCompressed(const jstring jstr) {
  MString *strObj = MString::JniCastNonNull(jstr);
  return strObj->IsCompress();
}

// invoke by compiler
bool MCC_String_Equals_NotallCompress(jstring thisStr, jstring anotherStr) {
  MString *thisStrObj = MString::JniCastNonNull(thisStr);
  MString *anotherStrObj = MString::JniCastNonNull(anotherStr);
  bool thisIsCompress = thisStrObj->IsCompress();
  uint8_t *thisStrSrc = thisStrObj->GetContentsPtr();
  uint8_t *anotherStrSrc = anotherStrObj->GetContentsPtr();
  uint32_t thisStrLen = thisStrObj->GetLength();
  uint8_t *compressChars = nullptr;
  uint16_t *uncompressChars = nullptr;
  if (thisIsCompress) {
    compressChars = thisStrSrc;
    uncompressChars = reinterpret_cast<uint16_t*>(anotherStrSrc);
  } else {
    compressChars = anotherStrSrc;
    uncompressChars = reinterpret_cast<uint16_t*>(thisStrSrc);
  }
  for (uint32_t i = 0; i < thisStrLen; ++i) {
    if (compressChars[i] != uncompressChars[i]) {
      return false;
    }
  }
  return true;
}

static MString *StringAppend(uint8_t *stringsContent[], const uint32_t stringsLen[], const bool isStringsCompress[],
                             int32_t sumLength, uint32_t numOfStringAppend) {
  if (UNLIKELY(stringsContent == nullptr)) {
    return nullptr;
  } else if (UNLIKELY(sumLength == 0)) {
    return MString::NewConstEmptyStringObject();
  } else if (UNLIKELY(sumLength < 0)) {
    MRT_ThrowNewExceptionUnw("java/lang/OutOfMemoryError");
    return nullptr;
  }
  MString *newStringObj = nullptr;
  bool isAllCompress = true;
  for (uint32_t i = 0; i < numOfStringAppend; ++i) {
    isAllCompress = isAllCompress && isStringsCompress[i];
  }
  if (isAllCompress) {
    newStringObj = MString::NewStringObject<uint8_t>(static_cast<uint32_t>(sumLength), [&](MString &stringObj) {
      uint8_t *newContent = stringObj.GetContentsPtr();
      for (uint32_t i = 0; i < numOfStringAppend; ++i) {
        if (stringsLen[i] != 0) {
          errno_t tmpResult = memcpy_s(newContent, stringsLen[i], stringsContent[i], stringsLen[i]);
          if (UNLIKELY(tmpResult != EOK)) {
            LOG(FATAL) << "memcpy_s()#1 in StringAppend() return " << tmpResult << "rather than 0.";
          }
          newContent += stringsLen[i];
        }
      }
    });
  } else {
    newStringObj = MString::NewStringObject<uint16_t>(static_cast<uint32_t>(sumLength), [&](MString &stringObj) {
      uint16_t *newContent = reinterpret_cast<uint16_t*>(stringObj.GetContentsPtr());
      for (uint32_t i = 0; i < numOfStringAppend; ++i) {
        if (stringsLen[i] != 0) {
          if (isStringsCompress[i]) {
            uint8_t *content = stringsContent[i];
            for (uint32_t j = 0; j < stringsLen[i]; ++j) {
              newContent[j] = content[j];
            }
          } else {
            uint32_t copyLen = stringsLen[i] << 1;
            errno_t tmpResult = memcpy_s(newContent, copyLen, reinterpret_cast<uint16_t*>(stringsContent[i]), copyLen);
            CHECK(tmpResult == EOK);
          }
          newContent += stringsLen[i];
        }
      }
    });
  }
  return newStringObj;
}

enum TYPESTRINGAPPEND {
  kString = 0x01,
  kChar = 0x02,
  kBoolean = 0x03,
  kInt = 0x04
};
const int kMaxIntcharLen = 13;
const int kMaxNunmberString = 20;
const int kSwitchTypeOffset = 3;

uint8_t nullBuff[4] = { 'n', 'u', 'l', 'l' };
uint8_t trueBuff[4] = { 't', 'r', 'u', 'e' };
uint8_t falseBuff[5] = { 'f', 'a', 'l', 's', 'e' };
extern "C" jstring MCC_StringAppend(uint64_t toStringFlag, ...) {
  uint32_t numOfString = 0;
  uint8_t *stringsContent[kMaxNunmberString];
  uint32_t stringsLen[kMaxNunmberString];
  bool isStringsCompress[kMaxNunmberString];
  uint16_t charbuff[kMaxNunmberString];
  char intBuff[kMaxNunmberString][kMaxIntcharLen];
  jint sumLength = 0;

  va_list args;
  va_start(args, toStringFlag);

  uint32_t type = toStringFlag & 0x07;
  while (toStringFlag) {
    switch (type) {
      case kChar: {
        jchar c = static_cast<jchar>(va_arg(args, int32_t));
        charbuff[numOfString] = c;
        stringsContent[numOfString] = reinterpret_cast<uint8_t*>(&charbuff[numOfString]);
        stringsLen[numOfString] = 1;
        sumLength += 1;
        isStringsCompress[numOfString] = MString::IsCompressChar(c);
        break;
      }
      case kBoolean: {
        jboolean b = static_cast<jboolean>(va_arg(args, int32_t));
        if (b) {
          stringsContent[numOfString] = trueBuff;
          stringsLen[numOfString] = 4;
          sumLength += 4;
        } else {
          stringsContent[numOfString] = falseBuff;
          stringsLen[numOfString] = 5;
          sumLength += 5;
        }
        isStringsCompress[numOfString] = true;
        break;
      }
      case kInt: {
        jint intValue = static_cast<jint>(va_arg(args, int32_t));
        char *intChar = intBuff[numOfString];
        stringsContent[numOfString] = reinterpret_cast<uint8_t*>(intChar);
        int len = sprintf_s(intChar, kMaxIntcharLen, "%d", intValue);
        if (UNLIKELY(len < 0)) {
          LOG(ERROR) << "MCC_StringAppend sprintf_s fail" << maple::endl;
          len = 0;
        }
        stringsLen[numOfString] = len;
        sumLength += len;
        isStringsCompress[numOfString] = true;
        break;
      }
      case kString: {
        jstring tmpString = reinterpret_cast<jstring>(va_arg(args, jstring));
        uint32_t count;
        if (tmpString == nullptr) {
          stringsContent[numOfString] = nullBuff;
          count = 9;
        } else {
          MString *string = MString::JniCastNonNull(tmpString);
          stringsContent[numOfString] = string->GetContentsPtr();
          count = string->GetCountValue();
        }
        bool isCompressTmp = (count == 0) ? true : MString::IsCompressedFromCount(count);
        isStringsCompress[numOfString] = isCompressTmp;
        uint32_t len = count >> 1;
        sumLength += len;
        stringsLen[numOfString] = len;
        break;
      }
      default: {
        LOG(FATAL) << "Unexpected primitive type: " << type;
      }
    }

    toStringFlag = toStringFlag >> kSwitchTypeOffset;
    type = toStringFlag & 0x07;
    ++numOfString;
  }
  va_end(args);
  MString *newStringObj = StringAppend(stringsContent, stringsLen, isStringsCompress, sumLength, numOfString);
  if (newStringObj == nullptr) {
    return nullptr;
  }
  return newStringObj->AsJstring();
}

extern "C" jstring MCC_StringAppend_StringString(jstring strObj1, jstring strObj2) {
  MString *stringObj1 = MString::JniCast(strObj1);
  MString *stringObj2 = MString::JniCast(strObj2);
  const int arrayLen = 2;
  uint8_t *stringsContent[arrayLen];
  uint32_t stringsLen[arrayLen];
  bool isStringsCompress[arrayLen];
  jint sumLength = 0;

  for (uint32_t i = 0; i < arrayLen; ++i) {
    MString *tmpString = (i == 0) ? stringObj1 : stringObj2;
    uint32_t count;
    if (tmpString == nullptr) {
      stringsContent[i] = nullBuff;
      count = 9;
    } else {
      stringsContent[i] = tmpString->GetContentsPtr();
      count = tmpString->GetCountValue();
    }
    bool isCompressTmp = (count == 0) ? true : MString::IsCompressedFromCount(count);
    isStringsCompress[i] = isCompressTmp;
    uint32_t len = count >> 1;
    sumLength += len;
    stringsLen[i] = len;
  }
  MString *newStringObj = StringAppend(stringsContent, stringsLen, isStringsCompress, sumLength, arrayLen);
  if (newStringObj == nullptr) {
    return nullptr;
  }
  return newStringObj->AsJstring();
}

// count value of String is the length shift left 1 bit and use the last bit to save whether the string is compress
extern "C" jstring MCC_StringAppend_StringInt(jstring strObj1, jint intValue) {
  MString *stringObj = MString::JniCast(strObj1);
  const int arraySize = 2;
  uint8_t *stringsContent[arraySize];
  char intBuff[kMaxIntcharLen];
  uint32_t stringsLen[arraySize];
  bool isStringsCompress[arraySize];
  jint sumLength = 0;

  MString *tmpString = stringObj;
  uint32_t count;
  if (tmpString == nullptr) {
    stringsContent[0] = nullBuff;
    count = 9;
  } else {
    stringsContent[0] = tmpString->GetContentsPtr();
    count = tmpString->GetCountValue();
  }
  bool isCompressTmp = (count == 0) ? true : MString::IsCompressedFromCount(count);
  isStringsCompress[0] = isCompressTmp;
  uint32_t len = count >> 1;
  sumLength += len;
  stringsLen[0] = len;

  stringsContent[1] = reinterpret_cast<uint8_t*>(intBuff);
  int intLen = sprintf_s(intBuff, kMaxIntcharLen, "%d", intValue);
  if (UNLIKELY(intLen < 0)) {
    LOG(ERROR) << "MCC_StringAppend sprintf_s fail" << maple::endl;
    intLen = 0;
  }
  stringsLen[1] = intLen;
  sumLength += intLen;
  isStringsCompress[1] = true;
  MString *newStringObj = StringAppend(stringsContent, stringsLen, isStringsCompress, sumLength, arraySize);
  if (newStringObj == nullptr) {
    return nullptr;
  }
  return newStringObj->AsJstring();
}

extern "C" jstring MCC_StringAppend_StringJcharString(jstring strObj1, uint16_t charValue, jstring strObj2) {
  MString *stringObj1 = MString::JniCast(strObj1);
  MString *stringObj2 = MString::JniCast(strObj2);
  const int arraySize = 3;
  uint8_t *stringsContent[arraySize];
  uint32_t stringsLen[arraySize];
  bool isStringsCompress[arraySize];
  jint sumLength = 0;

  for (uint32_t i = 0; i < arraySize; ++i) {
    if (i == 1) {
      continue;
    }
    MString *tmpString = (i == 0) ? stringObj1 : stringObj2;
    uint32_t count;
    if (tmpString == nullptr) {
      stringsContent[i] = nullBuff;
      count = 9;
    } else {
      stringsContent[i] = tmpString->GetContentsPtr();
      count = tmpString->GetCountValue();
    }
    bool isCompressTmp = (count == 0) ? true : MString::IsCompressedFromCount(count);
    isStringsCompress[i] = isCompressTmp;
    uint32_t len = count >> 1;
    sumLength += len;
    stringsLen[i] = len;
  }

  stringsContent[1] = reinterpret_cast<uint8_t*>(&charValue);
  stringsLen[1] = 1;
  sumLength += 1;
  isStringsCompress[1] = MString::IsCompressChar(charValue);
  MString *newStringObj = StringAppend(stringsContent, stringsLen, isStringsCompress, sumLength, arraySize);
  if (newStringObj == nullptr) {
    return nullptr;
  }
  return newStringObj->AsJstring();
}

#ifdef __OPENJDK__
template<typename subType, typename srcType>
int32_t StringNativeIndexOf(MString &subStrObj, const srcType *srcArrayData,
                            int32_t srcOffset, int32_t srcCount,
                            int32_t fromIndex, int32_t subLen) {
  if (UNLIKELY(srcArrayData == nullptr)) {
    return -1;
  }
  if (fromIndex >= srcCount) {
    return (subLen == 0 ? srcCount : -1);
  }
  if (fromIndex < 0) {
    fromIndex = 0;
  }
  if (subLen == 0) {
    return fromIndex;
  }
  subType *subStrArrayData = reinterpret_cast<subType*>(subStrObj.GetContentsPtr());
  subType first = subStrArrayData[0];
  int32_t max = srcOffset + (srcCount - subLen);

  for (int i = srcOffset + fromIndex; i <= max; ++i) {
    if (srcArrayData[i] != first) {
      while ((++i <= max) && (srcArrayData[i] != first)) {}
    }

    if (i <= max) {
      int j = i + 1;
      int end = j + subLen - 1;
      for (int k = 1; (j < end) && (srcArrayData[j] == subStrArrayData[k]); ++j, ++k) {}
      if (j == end) {
        return i - srcOffset;
      }
    }
  }
  return -1;
}

int32_t StringNativeIndexOfP3(MString &subStrObj, MString &srcStrObj, int32_t fromIndex) {
  bool srcIsCompress = srcStrObj.IsCompress();
  bool subStrIsCompress = subStrObj.IsCompress();
  char *srcChar = reinterpret_cast<char*>(srcStrObj.GetContentsPtr());
  uint32_t srcCount = srcStrObj.GetLength();
  uint32_t subLen = subStrObj.GetLength();
  uint32_t res = -1;
  if (srcIsCompress && !subStrIsCompress) {
    res = StringNativeIndexOf<uint16_t, char>(subStrObj, srcChar, 0, srcCount, fromIndex, subLen);
  } else if (srcIsCompress && subStrIsCompress) {
    res = StringNativeIndexOf<char, char>(subStrObj, srcChar, 0, srcCount, fromIndex, subLen);
  } else {
    uint16_t *srcArrayData = reinterpret_cast<uint16_t*>(srcChar);
    if (subStrIsCompress) {
      res = StringNativeIndexOf<char, uint16_t>(subStrObj, srcArrayData, 0, srcCount, fromIndex, subLen);
    } else {
      res = StringNativeIndexOf<uint16_t, uint16_t>(subStrObj, srcArrayData, 0, srcCount, fromIndex, subLen);
    }
  }
  return res;
}

int32_t StringNativeIndexOfP5(MString &subStrObj, MArray &srcArray,
                              int32_t srcOffset, int32_t srcCount, int32_t fromIndex) {
  uint16_t *srcArrayData = reinterpret_cast<uint16_t*>(srcArray.ConvertToCArray());
  uint32_t subLen = subStrObj.GetLength();
  bool subStrIsCompress = subStrObj.IsCompress();
  int32_t res = -1;
  if (subStrIsCompress) {
    res = StringNativeIndexOf<char, uint16_t>(subStrObj, srcArrayData, srcOffset, srcCount, fromIndex, subLen);
  } else {
    res = StringNativeIndexOf<uint16_t, uint16_t>(subStrObj, srcArrayData, srcOffset, srcCount, fromIndex, subLen);
  }
  return res;
}

template<typename subType, typename srcType>
int32_t StringNativeLastIndexOf(MString &subStrObj, const srcType *srcArrayData, int32_t srcOffset,
                                int32_t srcCount, int32_t fromIndex, int32_t subLen) {
  if (UNLIKELY(srcArrayData == nullptr)) {
    return -1;
  }
  int32_t rightIndex = srcCount - subLen;
  if (fromIndex < 0) {
    return -1;
  }
  if (fromIndex > rightIndex) {
    fromIndex = rightIndex;
  }
  if (subLen == 0) {
    return fromIndex;
  }

  subType *subStrArrayData = reinterpret_cast<subType*>(subStrObj.GetContentsPtr());
  int32_t strLastIndex = subLen - 1;
  uint16_t strLastChar = subStrArrayData[strLastIndex];
  int32_t min = srcOffset + subLen - 1;
  int i = min + fromIndex;

  while (true) {
    bool flag = false;
    while (i >= min && srcArrayData[i] != strLastChar) {
      --i;
    }
    if (i < min) {
      return -1;
    }
    int j = i - 1;
    int start = j - (subLen - 1);
    int k = strLastIndex - 1;
    while (j > start) {
      if (srcArrayData[j--] != subStrArrayData[k--]) {
        --i;
        flag = true;
        break;
      }
    }
    if (flag) {
      continue;
    }
    return start - srcOffset + 1;
  }
}

int32_t StringNativeLastIndexOfP3(MString &subStrObj, MString &srcStrObj, int32_t fromIndex) {
  uint32_t subLen = subStrObj.GetLength();
  uint32_t srcCount = srcStrObj.GetLength();
  int32_t res = 0;
  char *srcChar = reinterpret_cast<char*>(srcStrObj.GetContentsPtr());
  bool subStrIsCompress = subStrObj.IsCompress();
  bool srcStrIsCompress = srcStrObj.IsCompress();
  if (srcStrIsCompress && !subStrIsCompress) {
    res = StringNativeLastIndexOf<uint16_t, char>(subStrObj, srcChar, 0, srcCount, fromIndex, subLen);
  } else if (srcStrIsCompress && subStrIsCompress) {
    res = StringNativeLastIndexOf<char, char>(subStrObj, srcChar, 0, srcCount, fromIndex, subLen);
  } else {
    uint16_t *srcArrayData = reinterpret_cast<uint16_t*>(srcChar);
    if (subStrIsCompress) {
      res = StringNativeLastIndexOf<char, uint16_t>(subStrObj, srcArrayData, 0, srcCount, fromIndex, subLen);
    } else {
      res = StringNativeLastIndexOf<uint16_t, uint16_t>(subStrObj, srcArrayData, 0, srcCount, fromIndex, subLen);
    }
  }
  return res;
}

int32_t StringNativeLastIndexOfP5(MString &subStrObj, MArray &srcArray,
                                  int32_t srcOffset, int32_t srcCount, int32_t fromIndex) {
  uint16_t *srcArrayData = reinterpret_cast<uint16_t*>(srcArray.ConvertToCArray());
  uint32_t subLen = subStrObj.GetLength();
  bool subStrIsCompress = subStrObj.IsCompress();
  int32_t res = 0;
  if (subStrIsCompress) {
    res = StringNativeLastIndexOf<char, uint16_t>(subStrObj, srcArrayData, srcOffset,
                                                  srcCount, fromIndex, subLen);
  } else {
    res = StringNativeLastIndexOf<uint16_t, uint16_t>(subStrObj, srcArrayData, srcOffset,
                                                      srcCount, fromIndex, subLen);
  }
  return res;
}

const int kSurrogatesBits = 10;

static int minSupplementaryCodePoint = 0x010000;
static uint16_t minHighSurrogate = 0xd800;
static uint16_t minLowSurrogate = 0xdc00;
static uint16_t maxHighSurrogate = 0xdbff;
static uint16_t maxLowSurrogate = 0xdfff;

static uint32_t validMaxCodePoint = ((0X10FFFF + 1) >> kUtf16Bits);
static uint16_t validMinHighSurrogate = (minHighSurrogate - (minSupplementaryCodePoint >> kSurrogatesBits));
static uint16_t validMinLowSurrogatee = minLowSurrogate;

// codepoint hava two part, each part is u16
// the leading surrogate code unit used to represent the character in the UTF-16 encoding
MString *StringNewStringFromCodePoints(MArray &mArray, int32_t offset, int32_t count) {
  uint32_t *jintArray = reinterpret_cast<uint32_t*>(mArray.ConvertToCArray());
  if (count <= 0) {
    return nullptr;
  }
  uint16_t *jcharArray = reinterpret_cast<uint16_t*>(malloc(count * 2 * sizeof(uint16_t))); // codepoint hava two part
  if (UNLIKELY(jcharArray == nullptr)) {
    return nullptr;
  }
  int32_t end = offset + count;
  int32_t length = 0;

  for (int i = offset; i < end; ++i) {
    if ((jintArray[i] >> kUtf16Bits) == 0) {
      jcharArray[length] = static_cast<uint16_t>(jintArray[i]);
      ++length;
    } else if((jintArray[i] >> kUtf16Bits) < validMaxCodePoint) {
      jcharArray[length++] = ((jintArray[i] >> kSurrogatesBits) + validMinHighSurrogate);
      jcharArray[length++] = ((jintArray[i] & 0x3ff) + validMinLowSurrogatee);
    } else {
      free(jcharArray);
      jcharArray = nullptr;
      MRT_ThrowNewException("java/lang/IllegalArgumentException",
                            "Exception IllegalArgumentException in newStringFromCodePoints");
      return nullptr;
    }
  }

  const bool compressible = MString::IsCompressChars<uint16_t>(jcharArray, length);
  MString *res = nullptr;
  if (compressible) {
    res = MString::NewStringObject<uint16_t, uint8_t>(jcharArray, length);
  } else {
    res = MString::NewStringObject<uint16_t>(jcharArray, length);
  }
  free(jcharArray);
  jcharArray = nullptr;
  return res;
}

static inline bool IsHighSurrogate(uint16_t ch) {
  return (ch >= minHighSurrogate && ch < (maxHighSurrogate + 1));
}

static inline bool IsLowSurrogate(uint16_t ch) {
  return (ch >= minLowSurrogate && ch < (maxLowSurrogate + 1));
}

static inline int32_t ToCodePoint(uint16_t high, uint16_t low) {
  return ((high << kSurrogatesBits) + low) +
      (minSupplementaryCodePoint - (minHighSurrogate << kSurrogatesBits) - minLowSurrogate);
}

int32_t StringNativeCodePointAt(MString &strObj, int32_t index) {
  uint32_t length = strObj.GetLength();
  if (index < 0 || static_cast<uint32_t>(index) >= length) {
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                          "StringNativeCodePointAt(): input index out of Bounds");
    return -1;
  }
  bool strIsCompress = strObj.IsCompress();
  if (strIsCompress) {
    char *strDataC = reinterpret_cast<char*>(strObj.GetContentsPtr());
    char ch = strDataC[index];
    return ch;
  }

  uint16_t *strData = reinterpret_cast<uint16_t*>(strObj.GetContentsPtr());
  uint16_t c1 = strData[index];
  if (IsHighSurrogate(c1) && static_cast<uint32_t>(++index) < length) {
    uint16_t c2 = strData[index];
    if (IsLowSurrogate(c2)) {
      return ToCodePoint(c1, c2);
    }
  }
  return c1;
}

int32_t StringNativeCodePointBefore(MString &strObj, int32_t index) {
  uint32_t length = strObj.GetLength();
  if (index < 1 || static_cast<uint32_t>(index) > length) {
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                          "StringNativeCodePointBefore(): input index out of Bounds");
    return -1;
  }
  bool strIsCompress = strObj.IsCompress();
  if (strIsCompress) {
    char *strDataC = reinterpret_cast<char*>(strObj.GetContentsPtr());
    char ch = strDataC[--index];
    return ch;
  }

  uint16_t *strData = reinterpret_cast<uint16_t*>(strObj.GetContentsPtr());
  uint16_t c2 = strData[--index];
  if (IsLowSurrogate(c2) && index > 0) {
    uint16_t c1 = strData[--index];
    if (IsHighSurrogate(c1)) {
      return ToCodePoint(c1, c2);
    }
  }
  return c2;
}

int32_t StringNativeCodePointCount(MString &strObj, int32_t beginIndex, int32_t endIndex) {
  uint32_t length = strObj.GetLength();
  if (beginIndex < 0 || static_cast<uint32_t>(endIndex) > length || beginIndex > endIndex) {
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                          "StringNativeCodePointCount(): input index out of Bounds");
    return -1;
  }
  int n = endIndex - beginIndex;
  bool strIsCompress = strObj.IsCompress();
  if (strIsCompress) {
    return n;
  }
  uint16_t *strData = reinterpret_cast<uint16_t*>(strObj.GetContentsPtr());
  for (int i = beginIndex; i < endIndex;) {
    if (IsHighSurrogate(strData[i++]) && i < endIndex && IsLowSurrogate(strData[i])) {
      --n;
      ++i;
    }
  }
  return n;
}

int32_t StringNativeOffsetByCodePoint(MString &strObj, int32_t index, int32_t codePointOffset) {
  uint32_t length = strObj.GetLength();
  if (index < 0 || static_cast<uint32_t>(index) > length) {
    MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                          "StringNativeOffsetByCodePoint(): input index out of Bounds");
    return -1;
  }
  bool strIsCompress = strObj.IsCompress();
  uint16_t *strData = reinterpret_cast<uint16_t*>(strObj.GetContentsPtr());
  int x = index;
  int resCount = x + codePointOffset;
  if (codePointOffset >= 0) {
    if (strIsCompress) {
      if (static_cast<uint32_t>(resCount) < length) {
        return resCount;
      } else {
        MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                              "StringNativeOffsetByCodePoint(): input index out of Bounds");
        return -1;
      }
    }
    int i = 0;
    while (static_cast<uint32_t>(x) < length && i < codePointOffset) {
      if (IsHighSurrogate(strData[x++]) && static_cast<uint32_t>(x) < length && IsLowSurrogate(strData[x])) {
        ++x;
      }
      ++i;
    }
    if (i < codePointOffset) {
      MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                            "StringNativeOffsetByCodePoint(): input index out of Bounds");
      return -1;
    }
  } else {
    if (strIsCompress) {
      if (resCount < 0) {
        MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                              "StringNativeOffsetByCodePoint(): input index out of Bounds");
        return -1;
      } else {
        return resCount;
      }
    }
    int j = codePointOffset;
    while (x > 0 && j < 0) {
      if (IsLowSurrogate(strData[--x]) && x > 0 && IsHighSurrogate(strData[x - 1])) {
        --x;
      }
      ++j;
    }
    if (j < 0) {
      MRT_ThrowNewException("java/lang/StringIndexOutOfBoundsException",
                            "StringNativeOffsetByCodePoint(): input index out of Bounds");
      return -1;
    }
  }
  return x;
}
#endif // __OPENJDK__
} // namespace maplert
