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
#ifndef HIR2MPL_INCLUDE_JBC_CLASS_CONST_POOL_H
#define HIR2MPL_INCLUDE_JBC_CLASS_CONST_POOL_H
#include "jbc_class_const.h"

namespace maple {
namespace jbc {
class JBCConstPool {
 public:
  explicit JBCConstPool(MapleAllocator &alloc);
  ~JBCConstPool() = default;
  uint16 InsertConst(JBCConst &objConst);
  void InsertConstDummyForWide();

  /*
   * GetConstByIdx
   * GetConstXXXByIdx
   * If safe is true, the return value must be not nullptr or fatal and exit.
   * If safe is false, the return value may be nullptr with error printed.
   */
  const JBCConst *GetConstByIdx(uint16 idx, bool safed = false) const;
  const JBCConst *GetConstByIdxWithTag(uint16 idx, JBCConstTag tag, bool safed = false) const;
  const JBCConst *GetConstValueByIdx(uint16 idx, bool safed = false) const;
  const JBCConst *GetConstValue4ByteByIdx(uint16 idx, bool safed = false) const;
  const JBCConst *GetConstValue8ByteByIdx(uint16 idx, bool safed = false) const;
  std::string GetNameByClassInfoIdx(uint16 idx, bool safed = false) const;
  bool PreProcess(uint16 argMajorVersion);
  bool PrepareFEStructElemInfo(const std::string &ownerClassName);
  JBCConstUTF8 *NewConstUTF8(uint16 &idx, const std::string &str);
  JBCConst4Byte *NewConst4Byte(uint16 &idx, int32 value);
  JBCConst4Byte *NewConst4Byte(uint16 &idx, float value);
  JBCConst8Byte *NewConst8Byte(uint16 &idx, int64 value);
  JBCConst8Byte *NewConst8Byte(uint16 &idx, double value);
  JBCConstClass *NewConstClass(uint16 &idx, const std::string &className);
  JBCConstString *NewConstString(uint16 &idx, const std::string &str);
  JBCConstRef *NewConstRef(uint16 &idx, JBCConstTag tag, const std::string &className, const std::string &name,
                           const std::string &desc);
  JBCConstNameAndType *NewConstNameAndType(uint16 &idx,  const std::string &name, const std::string &desc);

  uint16 GetMajorVersion() const {
    return majorVersion;
  }

  std::string GetStringByConstUTF8(uint16 idx, bool safed = false) const {
    const JBCConst *constUTF8 = GetConstByIdxWithTag(idx, JBCConstTag::kConstUTF8, safed);
    return constUTF8 == nullptr ? "" : static_cast<const JBCConstUTF8*>(constUTF8)->GetString();
  }

 private:
  MapleAllocator &allocator;
  MapleVector<JBCConst*> pool;
  uint16 majorVersion;
};
}  // namespace jbc
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_JBC_CLASS_CONST_POOL_H
