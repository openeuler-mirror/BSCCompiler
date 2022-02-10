/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL_FE_BC_INPUT_BC_IO_H
#define MPL_FE_BC_INPUT_BC_IO_H
#include <string>
#include "basic_io.h"

namespace maple {
namespace bc {
class BCIO : public BasicIOMapFile {
 public:
  explicit BCIO(const std::string &fileName);
  ~BCIO() = default;

  static bool IsBigEndian() {
    union {
      uint16 v16;
      uint8 a[sizeof(uint16) / sizeof(uint8)];
    } u;
    u.v16 = 0x1234;
    return u.a[0] == 0x12;
  }

 protected:
  bool isBigEndianHost = true;
};

struct RawData {
  const uint8 *context;
  uint32 size;
};

class BCReader : public BCIO, public BasicIORead {
 public:
  explicit BCReader(const std::string &fileName);
  virtual ~BCReader();
  bool RetrieveHeader(RawData &data);
  void SetEndianTag(bool isBigEndian);
  template <typename T>
  T GetRealInteger(T value) const {
    if (isBigEndian == isBigEndianHost || sizeof(T) == sizeof(uint8)) {
      return value; // Same Endian, not need to convert
    } else {
      union U {
        uint8 a[sizeof(T)];
        T val;
      } u0;
      u0.val = value;
      U u1;
      for (size_t i = 0; i < sizeof(T); ++i) {
        u1.a[i] = u0.a[sizeof(T) - 1 - i];
      }
      return u1.val;
    }
  }

  struct ClassElem {
    std::string className;
    std::string elemName;
    std::string typeName;
  };

  std::string GetStringFromIdx(uint32 idx) const;
  std::string GetTypeNameFromIdx(uint32 idx) const;
  ClassElem GetClassMethodFromIdx(uint32 idx) const;
  ClassElem GetClassFieldFromIdx(uint32 idx) const;
  std::string GetSignature(uint32 idx) const;
  uint32 GetFileIndex() const;
  std::string GetIRSrcFileSignature() const;

 protected:
  virtual std::string GetStringFromIdxImpl(uint32 idx) const = 0;
  virtual std::string GetTypeNameFromIdxImpl(uint32 idx) const = 0;
  virtual ClassElem GetClassMethodFromIdxImpl(uint32 idx) const = 0;
  virtual ClassElem GetClassFieldFromIdxImpl(uint32 idx) const = 0;
  virtual std::string GetSignatureImpl(uint32 idx) const = 0;
  virtual uint32 GetFileIndexImpl() const = 0;
  virtual bool RetrieveHeaderImpl(RawData &data);
  std::string irSrcFileSignature;
};
}  // namespace bc
}  // namespace maple
#endif  // MPL_FE_BC_INPUT_BC_IO_H
