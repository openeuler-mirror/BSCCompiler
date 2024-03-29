/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_SRC_POSITION_H
#define MAPLE_IR_INCLUDE_SRC_POSITION_H
#include "mpl_logging.h"
#include "types_def.h"

namespace maple {
// to store source position information
class SrcPosition {
 public:
  SrcPosition() : lineNum(0), mplLineNum(0) {
    u.fileColumn.fileNum = 0;
    u.fileColumn.column = 0;
    u.word0 = 0;
  }
  SrcPosition(uint16 fnum, uint32 lnum, uint16 cnum, uint32 mlnum)
      : lineNum(lnum),
        mplLineNum(mlnum) {
    u.fileColumn.fileNum = fnum;
    u.fileColumn.column = cnum;
  }

  virtual ~SrcPosition() = default;

  uint32 RawData() const {
    return u.word0;
  }

  uint16 FileNum() const {
    return u.fileColumn.fileNum;
  }

  uint16 Column() const {
    return u.fileColumn.column;
  }

  uint32 LineNum() const {
    return lineNum;
  }

  uint32 MplLineNum() const {
    return mplLineNum;
  }

  void SetFileNum(uint16 n) {
    u.fileColumn.fileNum = n;
  }

  void SetColumn(uint16 n) {
    u.fileColumn.column = n;
  }

  void SetLineNum(uint32 n) {
    lineNum = n;
  }

  void SetRawData(uint32 n) {
    u.word0 = n;
  }

  void SetMplLineNum(uint32 n) {
    mplLineNum = n;
  }

  void CondSetLineNum(uint32 n) {
    lineNum = lineNum != 0 ? lineNum : n;
  }

  void CondSetFileNum(uint16 n) {
    uint16 i = u.fileColumn.fileNum;
    u.fileColumn.fileNum = i != 0 ? i : n;
  }

  void UpdateWith(const SrcPosition pos) {
    u.fileColumn.fileNum = pos.FileNum();
    u.fileColumn.column = pos.Column();
    lineNum = pos.LineNum();
    mplLineNum = pos.MplLineNum();
  }

  bool IsSameFile(const SrcPosition &pos) const {
    return pos.FileNum() == FileNum();
  }

  // as you read: pos0->IsBf(pos) "pos0 Is Before pos"
  bool IsBf(const SrcPosition &pos) const {
    return (pos.FileNum() == FileNum() &&
           ((LineNum() < pos.LineNum()) ||
            ((LineNum() == pos.LineNum()) && (Column() < pos.Column()))));
  }

  bool IsBfMpl(const SrcPosition &pos) const {
    return (pos.FileNum() == FileNum() &&
           ((MplLineNum() < pos.MplLineNum()) ||
            ((MplLineNum() == pos.MplLineNum()) && (Column() < pos.Column()))));
  }

  bool IsEq(const SrcPosition &pos) const {
    return FileNum() == pos.FileNum() && LineNum() == pos.LineNum() && Column() == pos.Column();
  }

  bool IsBfOrEq(const SrcPosition &pos) const {
    return IsBf(pos) || IsEq(pos);
  }

  bool IsEqMpl(const SrcPosition &pos) const {
    return MplLineNum() == pos.MplLineNum();
  }

  bool IsValid() const {
    return FileNum() != 0;
  }

  void DumpLoc(uint32 &lastLineNum, uint16 &lastColumnNum) const {
    if (FileNum() != 0 && LineNum() != 0) {
      if (Column() != 0 && (LineNum() != lastLineNum || Column() != lastColumnNum)) {
        Dump();
        lastLineNum = LineNum();
        lastColumnNum = Column();
      } else if (LineNum() != lastLineNum) {
        DumpLine();
        lastLineNum = LineNum();
      }
    }
  }

  void DumpLine() const {
    LogInfo::MapleLogger() << "LOC " << FileNum() << " " << LineNum() << '\n';
  }

  void Dump() const {
    LogInfo::MapleLogger() << "LOC " << FileNum() << " " << LineNum() << " " << Column() << '\n';
  }

  std::string DumpLocWithColToString() const {
    std::stringstream ss;
    ss << "LOC " << FileNum() << " " << LineNum() << " " << Column();
    return ss.str();
  }

  Loc GetSrcLoc() const {
    Loc loc(static_cast<uint32>(FileNum()), static_cast<uint32>(LineNum()), static_cast<uint32>(Column()));
    return loc;
  }

  void SetInlinedLineNum(uint32 line) {
    inlinedLineNum = line;
  }

  void SetInlinedFileNum(uint32 line) {
    inlinedFileNum = line;
  }

  void SetInlinedFuncStrIdx(const GStrIdx &idx) {
    strIdx = idx;
  }

  uint32 GetInlinedLineNum() const {
    return inlinedLineNum;
  }

  uint32 GetInlinedFileNum() const {
    return inlinedFileNum;
  }

  const GStrIdx &GetInlinedFuncStrIdx() const {
    return strIdx;
  }

 private:
  union {
    struct {
      uint16 fileNum;
      uint16 column : 12;
      uint16 stmtBegin : 1;
      uint16 bbBegin : 1;
      uint16 unused : 2;
    } fileColumn;
    uint32 word0;
  } u;
  uint32 lineNum;     // line number of original src file, like foo.java
  uint32 mplLineNum;  // line number of mpl file
  uint32 inlinedLineNum = 0;
  uint32 inlinedFileNum = 0;
  GStrIdx strIdx;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_SRC_POSITION_H
