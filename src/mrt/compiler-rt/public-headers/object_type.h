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
#ifndef __MAPLE_LOADER_OBJECT_TYPE__
#define __MAPLE_LOADER_OBJECT_TYPE__

namespace maplert {
// compiler-rt API use IObject for JNI type (jclass/IObject) or Java type (MClass/MObject) and so
#define TYPE_CAST(T, D) reinterpret_cast<T>(D)
#define CONST_TYPE_CAST(T, D) const_cast<T>(reinterpret_cast<const T>(D))
class IObject {
 public:
  IObject() = default;
  ~IObject() {
    data = nullptr;
  }
  IObject(const IObject &c) {
    this->data = c.data;
  }
  IObject &operator=(const IObject &obj) {
    this->data = obj.data;
    return *this;
  }
  bool operator==(const IObject obj) const {
    return data == obj.data;
  }
  bool operator!=(const IObject obj) const {
    return data != obj.data;
  }
  bool operator<(const IObject obj) const {
    return data < obj.data;
  }
  template<typename Type>
  IObject(const Type &c) {
    this->data = CONST_TYPE_CAST(void*, c);
  }
  template<typename Type>
  Type As() {
    return TYPE_CAST(Type, data);
  }
  template<typename Type>
  Type As() const {
    return TYPE_CAST(Type, data);
  }
  template<typename Type>
  IObject &operator=(const Type &obj) {
    data = CONST_TYPE_CAST(void*, obj);
    return *this;
  }
  template<typename Type>
  bool operator==(const Type obj) const {
    return data == CONST_TYPE_CAST(void*, obj);
  }
  template<typename Type>
  bool operator!=(const Type obj) const {
    return data != CONST_TYPE_CAST(void*, obj);
  }
  template<typename Type>
  bool operator<(const Type obj) const {
    return data < CONST_TYPE_CAST(void*, obj);
  }
  bool Empty() const {
    return data == nullptr;
  }
  const void *Get() const {
    return data;
  }
 protected:
  void *data = nullptr;
};
using IEnv = IObject;
using INativeMethod = IObject;
using IAdapterEx = IObject;
using IObjectLocator = IObject;
} // namespace maplert
#endif // __MAPLE_LOADER_OBJECT_TYPE__

