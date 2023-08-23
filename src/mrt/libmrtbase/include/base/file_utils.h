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

#ifndef _MAPLE_FILE_IO_H
#define _MAPLE_FILE_IO_H

#include <fcntl.h>
#include <cstdint>
#include <string>
#include <vector>

namespace maple {

class File final {
 public:
  File();
  File(int fd_);
  File(int fd_, const std::string &path);
  File(const std::string &path, int flags);
  File(const std::string &path, int flags, mode_t mode);

  int GetFd() const;

  virtual ~File();

  int64_t Read(char *buffer, int64_t count, int64_t offset) const;
  int64_t Write(const char *buffer, int64_t count, int64_t offset);
  int Close();
  int Flush();
  bool Unlink();
  bool Opened() const;

  bool ReadFully(std::string &buffer);
  bool WriteFully(const void *buffer, size_t count);
  void ReadLines(std::vector<std::string> &lines);

  int64_t GetLength() const;

 private:
  int fd;
  std::string filePath;
  static constexpr unsigned int maxPathLength = 4096;
  bool Open(const std::string &path, int flags, mode_t mode);
  void Destroy();
};

class FileUtils {
 public:
  // Open a file with O_RDONLY flag
  static File *OpenFileReadOnly(const std::string &name);

  // Open a file with O_RDWR flag
  static File *OpenFileReadWrite(const std::string &name);

  // Open a file whith user specified flags (man 2 open)
  static File *OpenFile(const std::string &name, int flags);

  // Create a new file
  static File *CreateFile(const std::string &name, unsigned int flags);

  // Create a file with O_RDWR flag
  static File *CreateFileReadWrite(const std::string &name);

  // Create a file with O_WRONLY flag
  static File *CreateFileWriteOnly(const std::string &name);

  // Judge if one file exists or not
  static bool FileExists(const std::string &name);

  // Judge if one directory exists or not
  static bool DirExists(const std::string &name);

  // Delete one directory and its subdirs recursely by default
  static void RmDir(const std::string &dir);
};

}  // namespace maple

#endif // _MAPLE_FILE_IO_H
