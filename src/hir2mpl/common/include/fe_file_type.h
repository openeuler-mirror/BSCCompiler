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
#ifndef HIR2MPL_INCLUDE_COMMON_FE_FILE_TYPE_H
#define HIR2MPL_INCLUDE_COMMON_FE_FILE_TYPE_H
#include <string>
#include <map>
#include "types_def.h"
#include "basic_io.h"

namespace maple {
class FEFileType {
 public:
  enum FileType {
    kUnknownType,
    kClass,
    kJar,
    kDex,
    kAST,
    kMAST,
  };

  inline static FEFileType &GetInstance() {
    return fileType;
  }

  FileType GetFileTypeByExtName(const std::string &extName) const;
  FileType GetFileTypeByPathName(const std::string &pathName) const;
  FileType GetFileTypeByMagicNumber(const std::string &pathName) const;
  FileType GetFileTypeByMagicNumber(BasicIOMapFile &file) const;
  FileType GetFileTypeByMagicNumber(uint32 magic) const;
  void Reset();
  void LoadDefault();
  void RegisterExtName(FileType fileType, const std::string &extName);
  void RegisterMagicNumber(FileType fileType, uint32 magicNumber);
  static std::string GetPath(const std::string &pathName);
  static std::string GetName(const std::string &pathName, bool withExt = true);
  static std::string GetExtName(const std::string &pathName);

 private:
  static FEFileType fileType;
  static const uint32 kMagicClass = 0xBEBAFECA;
  static const uint32 kMagicZip = 0x04034B50;
  static const uint32 kMagicDex = 0x0A786564;
  static const uint32 kMagicAST = 0x48435043;
  static const uint32 kMagicMAST = 0x854C504D;
  std::map<std::string, FileType> mapExtNameType;
  std::map<FileType, uint32> mapTypeMagic;
  std::map<uint32, FileType> mapMagicType;

  FEFileType();
  ~FEFileType() = default;
};
}
#endif
