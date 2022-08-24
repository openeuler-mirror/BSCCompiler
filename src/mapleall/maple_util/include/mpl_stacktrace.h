/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_UTIL_INCLUDE_MPL_STACKTRACE
#define MAPLE_UTIL_INCLUDE_MPL_STACKTRACE

#ifdef __unix__
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <link.h>
#endif

#include <iomanip>
#include <string>
#include <vector>

#include "mpl_logging.h"

namespace maple {

namespace stacktrace {

static std::string demangle(const char *mangledName) {
#ifdef __unix__
  int status = 0;
  char *name = abi::__cxa_demangle(mangledName, NULL, NULL, &status);

  if (status != 0) {
    return mangledName;
  }

  std::string res = std::string(name);
  std::free(name);

  return res;
#else
  return std::string();
#endif
}

class Frame {
 public:
  explicit Frame(void *addr) : addr(addr) {
    init();
  }

  Frame(const Frame &) = default;
  Frame &operator=(const Frame &) = default;
  ~Frame(){};

 public:
  std::string getFilename() const {
    return filename;
  }

  std::string getName() const {
    return name;
  }

  /* return addr in memory */
  const void *getAddr() const {
    return addr;
  }

  /* return the difference between the addr in the ELF file and the addr in memory */
  uintptr_t getLinkAddr() const {
    return linkAddr;
  }

  /* return addr in the ELF file */
  uintptr_t getElfAddr() const {
    uintptr_t castedAddr = reinterpret_cast<uintptr_t>(addr);
    ASSERT(castedAddr >= linkAddr, "invalid Frame");
    return castedAddr - linkAddr;
  }

  friend std::string to_string(const Frame &fr) {
    std::stringstream ss;
    if (fr.getName().empty())
      ss << fr.getAddr();
    else
      ss << fr.getName();
    ss << " ["
       << "0x" << std::hex << fr.getElfAddr() << "]";
    ss << (" in " + fr.getFilename());
    return ss.str();
  }

  friend std::ostream &operator<<(std::ostream &os, const Frame &fr) {
    os << to_string(fr);
    return os;
  }

 private:
  void init() {
#ifdef __unix__
    Dl_info info;
    void *extraInfo = nullptr;

    int status = dladdr1(addr, &info, &extraInfo, RTLD_DL_LINKMAP);
    if (status != 1) {
      LogInfo::MapleLogger() << "maple::Stacktrace: dladdr can't resolve address " << addr << std::endl;
      return;
    }

    link_map *linkMap = static_cast<link_map *>(extraInfo);

    filename = info.dli_fname ? std::string(info.dli_fname) : "??";
    name = info.dli_sname ? demangle(info.dli_sname) : "??";
    linkAddr = reinterpret_cast<uintptr_t>(linkMap->l_addr);
#else
    return;
#endif
  }

 private:
  const void *addr;

  uintptr_t linkAddr;
  std::string filename;
  std::string name;
};

template <class Allocator>
class Stacktrace {
 public:
  __attribute__((noinline)) Stacktrace(size_t maxDepth = 256) {
    init(1, maxDepth);
  }

  Stacktrace(const Stacktrace &st) : frames(st.frames) {}

  Stacktrace &operator=(const Stacktrace &st) {
    frames = st.frames;
    return *this;
  }

  Stacktrace(const Stacktrace &&st) : frames(std::move(st.frames)) {}

  Stacktrace &operator=(const Stacktrace &&st) {
    frames = std::move(st.frames);
    return *this;
  }

  ~Stacktrace() = default;

  friend std::string to_string(const Stacktrace &st) {
    std::stringstream ss;
    size_t count = 0;
    constexpr size_t width = 2;
    for (auto it = st.cbegin(); it != st.cend(); ++it) {
      ss << std::setw(width) << count++ << "# " << *it << std::endl;
    }
    return ss.str();
  }

  friend std::ostream &operator<<(std::ostream &os, const Stacktrace &st) {
    os << to_string(st);
    return os;
  }

 public:
  using iterator = typename std::vector<Frame, Allocator>::iterator;
  using const_iterator = typename std::vector<Frame, Allocator>::const_iterator;
  using reverse_iterator = typename std::reverse_iterator<iterator>;
  using const_reverse_iterator = typename std::reverse_iterator<const_iterator>;

  const_iterator cbegin() const {
    return frames.cbegin();
  }

  const_iterator cend() const {
    return frames.cend();
  }

  const_reverse_iterator crbegin() const {
    return frames.crbegin();
  }

  const_reverse_iterator crend() const {
    return frames.crend();
  }

  size_t size() const {
    return frames.size();
  }

 private:
  std::vector<Frame, Allocator> frames;

 private:
  void __attribute__((noinline)) init(size_t nskip, size_t maxDepth) {
    ++nskip;  // skip current frame

    typedef typename std::allocator_traits<Allocator>::template rebind_alloc<void *> allocator_void_ptr;
    allocator_void_ptr allocator;
    size_t bufferSize = maxDepth;
    void **buffer = allocator.allocate(bufferSize);

    size_t nframes = collectFrames(buffer, maxDepth);
    ++nskip;  // skip frame of call "colectFrames(...)"

    if (nskip <= nframes) {
      for (size_t i = 0; i < nframes - nskip; ++i) {
        frames.emplace_back(buffer[nskip + i]);
      }
    }

    allocator.deallocate(buffer, bufferSize);
  }

  static size_t __attribute__((noinline)) collectFrames(void **outbuf, int maxFrames) {
#ifdef __unix__
    return backtrace(outbuf, maxFrames);
#else
    return 0;
#endif
  }
};

}  // namespace stacktrace

template <class Allocator = std::allocator<stacktrace::Frame> >
using Stacktrace = stacktrace::Stacktrace<Allocator>;

}  // namespace maple

#endif  //MAPLE_UTIL_INCLUDE_MPL_STACKTRACE
