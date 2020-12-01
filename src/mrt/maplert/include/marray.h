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
#ifndef MRT_MAPLERT_INCLUDE_MARRAY_H_
#define MRT_MAPLERT_INCLUDE_MARRAY_H_
#include "mobject.h"
#include "mrt_fields_api.h"
namespace maplert {
  const uint32_t kMrtArrayLengthOffset = 12;  // shadow:[4|8] + monitor:4, but we fix it here,
                                              // since content offset is fixed
  const uint32_t kMrtArrayContentOffset = 16; // fixed. aligned to 8B to along hosting of size-8B elements.

enum ArrayElemSize {
  kElemByte = 0,
  kElemHWord = 1,
  kElemWord = 2,
  kElemDWord = 3
};

class MArray : public MObject {
 public:
  uint32_t GetLength() const;
  uint32_t GetArraySize() const;
  uint32_t GetElementSize() const;
  char *GetElementTypeName() const;
  void InitialObjectArray(const MObject *initialElement) const;
  uint8_t *ConvertToCArray() const;
  template<typename T>
  T GetPrimitiveElement(uint32_t index);
  template<typename T>
  void SetPrimitiveElement(uint32_t index, T value);
  MObject *GetObjectElement(uint32_t index) const;
  void SetObjectElement(uint32_t index, const MObject *mObj) const;
  MObject *GetObjectElementNoRc(uint32_t index) const;
  void SetObjectElementNoRc(uint32_t index, const MObject *mObj) const;
  MObject *GetObjectElementOffHeap(uint32_t index) const;
  void SetObjectElementOffHeap(uint32_t index, const MObject *mObj) const;
  bool HasNullElement() const;
  static uint32_t GetArrayContentOffset();
  template<ArrayElemSize elemSizeExp>
  static MArray *NewPrimitiveArray(uint32_t length, const MClass &arrayClass, bool isJNI = false);
  static MArray *NewPrimitiveArray(uint32_t length, const MClass &arrayClass, bool isJNI = false);
  template<ArrayElemSize elemSizeExp>
  static MArray *NewPrimitiveArrayComponentClass(uint32_t length, const MClass &componentClass);
  static MArray *NewPrimitiveArrayComponentClass(uint32_t length, const MClass &componentClass);
  static MArray *NewObjectArray(uint32_t length, const MClass &arrayClass);
  static MArray *NewObjectArrayComponentClass(uint32_t length, const MClass &componentClass);

  template<typename T>
  static inline MArray *JniCast(T array);
  template<typename T>
  static inline MArray *JniCastNonNull(T array);
  inline jarray AsJarray() const;
  inline jobjectArray AsJobjectArray() const;

 private:
  static constexpr uint32_t lengthOffset = kMrtArrayLengthOffset;
  static constexpr uint32_t contentOffset = kMrtArrayContentOffset;
  static constexpr uint32_t objectElemSize = sizeof(reffield_t);
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MARRAY_H_
