/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef SIMPLE_BIT_SET_H
#define SIMPLE_BIT_SET_H

#include <bitset>
#include <functional>
#include <cstring>
#include <securec.h>
#include "mpl_logging.h"

namespace maple {
#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define ALIGN(number, alignSize) \
  (((number) + (alignSize) - 1) & ~((alignSize) - 1))

constexpr size_t kSimpleBitSetAligned = 64;  // 64 bit(8 Byte) alignment

template <size_t NB, std::enable_if_t<NB % kSimpleBitSetAligned == 0, bool> = true>
class SimpleBitSet {
  using WordT = uint64_t;  // bit size should be same as kSimpleBitSetAligned
  static constexpr size_t kSimpleBitSetSize = ((NB - 1) / kSimpleBitSetAligned) + 1;
  static constexpr uint64_t kAllOnes = 0xFFFFFFFFFFFFFFFF;

 public:
  SimpleBitSet() = default;

  virtual ~SimpleBitSet() = default;

  SimpleBitSet(const SimpleBitSet &) = default;

  SimpleBitSet &operator=(const SimpleBitSet &) = default;

  // no need explicit
  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet(const T &input) {
    memcpy_s(bitSet, sizeof(bitSet), &input, sizeof(T));
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet &operator=(const T &input) {
    memcpy_s(bitSet, sizeof(bitSet), &input, sizeof(T));
    return *this;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  bool operator==(const T &input) const {
    if (*this == SimpleBitSet<NB>(input)) {
      return true;
    }
    return false;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  bool operator!=(const T &input) const {
    if (*this == SimpleBitSet<NB>(input)) {
      return false;
    }
    return true;
  }

  bool operator==(const SimpleBitSet &input) const {
    if (input.GetWordSize() == kSimpleBitSetSize) {
      for (size_t i = 0; i < kSimpleBitSetSize; ++i) {
        if (bitSet[i] != input.GetWord(i)) {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  bool operator!=(const SimpleBitSet &input) const {
    if (*this == input) {
      return false;
    }
    return true;
  }

  bool operator<(const SimpleBitSet &input) const {
    if (input.GetWordSize() == kSimpleBitSetSize) {
      for (int i = (kSimpleBitSetSize - 1); i >= 0 ; --i) {
        if (bitSet[i] > input.GetWord(i)) {
          return false;
        } else if (bitSet[i] < input.GetWord(i)) {
          return true;
        }
      }
      return false;
    }
    CHECK_FATAL_FALSE("size should be same");
  }

#define CALC_INDEX_AND_BIT(pos, index, bit)  \
  size_t (index) = (pos) / kSimpleBitSetAligned; \
  size_t (bit) = (pos) % kSimpleBitSetAligned

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet &operator<<(const T &input) {
    for (size_t i = 0; i < kSimpleBitSetSize; ++i) {
      bitSet[i] <<= input;
    }
    return *this;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet &operator>>(const T &input) {
    for (size_t i = 0; i < kSimpleBitSetSize; ++i) {
      bitSet[i] >>= input;
    }
    return *this;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet &operator&=(const T &input) {
    CheckAndSet<T>(
        input, [](WordT &dst, T src) { dst &= src; },
        [this](WordT &dst, T &src, size_t bit) { dst &= BitMask(src, bit); });
    return *this;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet operator&(const T &input) const {
    SimpleBitSet<NB> ret(*this);
    ret &= input;
    return ret;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet &operator|=(const T &input) {
    CheckAndSet<T>(
        input, [](WordT &dst, T src) { dst |= src; },
        [this](WordT &dst, T &src, size_t bit) { dst |= BitMask(src, bit); });
    return *this;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet operator|(const T &input) {
    SimpleBitSet<NB> ret(*this);
    ret |= input;
    return ret;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet &operator^=(const T &input) {
    CheckAndSet<T>(
        input, [](WordT &dst, T src) { dst ^= src; },
        [this](WordT &dst, T &src, size_t bit) { dst ^= BitMask(src, bit); });
    return *this;
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  SimpleBitSet operator^(const T &input) {
    SimpleBitSet<NB> ret(*this);
    ret ^= input;
    return ret;
  }

  // flip all bits
  SimpleBitSet &operator~() {
    for (size_t i = 0; i < kSimpleBitSetSize; ++i) {
      bitSet[i] = ~bitSet[i];
    }
    return *this;
  }

  SimpleBitSet &operator&=(const SimpleBitSet &input) {
    Set(
        input, [](WordT &dst, WordT src) { dst &= src; },
        // fast path set high bits to 0
        [](WordT *startPos, size_t leftSize) {
            memset_s(startPos, leftSize * sizeof(WordT), 0, leftSize * sizeof(WordT)); });
    return *this;
  }

  SimpleBitSet operator&(const SimpleBitSet &input) const {
    SimpleBitSet<NB> ret(*this);
    ret &= input;
    return ret;
  }

  SimpleBitSet &operator|=(const SimpleBitSet &input) {
    Set(
        input, [](WordT &dst, WordT src) { dst |= src; }, [](WordT *startPos, size_t leftSize) { /* do nothing */ });
    return *this;
  }

  SimpleBitSet operator|(const SimpleBitSet &input) const {
    SimpleBitSet<NB> ret(*this);
    ret |= input;
    return ret;
  }

  SimpleBitSet &operator^=(const SimpleBitSet &input) {
    Set(
        input, [](WordT &dst, WordT src) { dst ^= src; },
        [](WordT *startPos, size_t leftSize) {
          for (size_t i = 0; i < leftSize; ++i) {
            startPos[0] ^= 0ULL;
          }
        });
    return *this;
  }

  SimpleBitSet operator^(const SimpleBitSet &input) const {
    SimpleBitSet<NB> ret(*this);
    ret &= input;
    return ret;
  }

#define POS_CHECK(pos, index, bit, fail)      \
  CALC_INDEX_AND_BIT(pos, index, bit);        \
  if (unlikely((index >= kSimpleBitSetSize))) { \
    return fail;                              \
  }                                           \
  (void)0  // avoid warning

  /**
     * reset pos bit to 0
     * @param pos
     * @return true if reset success, false if overflow
     */
  bool Reset(size_t pos) {
    POS_CHECK(pos, index, bit, false);
    bitSet[index] &= ~(1ULL << bit);
    return true;
  }

  /**
     * reset all bits to 0
     * @return
     */
  void Reset() {
    memset_s(bitSet, sizeof(WordT) * kSimpleBitSetSize, 0, sizeof(WordT) * kSimpleBitSetSize);
  }

  /**
     * set pos bit to 1
     * @param pos
     * @return true if reset success, false if overflow
     */
  bool Set(size_t pos) {
    POS_CHECK(pos, index, bit, false);
    bitSet[index] |= 1ULL << bit;
    return true;
  }

  /**
     * check pos is 1
     * @param pos
     * @return 1 if pos is 1, 0 false if pos is 0, -1 if overflow
     */
  int operator[](size_t pos) const {
    POS_CHECK(pos, index, bit, -1);
    return static_cast<int>((bitSet[index] & (1ULL << bit)) != 0);
  }

#undef POS_CHECK

  WordT GetWord(size_t index) const {
    if (unlikely(index >= kSimpleBitSetSize)) {
      CHECK_FATAL_FALSE("index out of range");
    }
    return bitSet[index];
  }

  template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
  void SetWord(size_t index, const T &input) {
    if (unlikely(index >= kSimpleBitSetSize)) {
      CHECK_FATAL_FALSE("index out of range");
    }
    bitSet[index] = input;
}

  size_t GetWordSize() const {
    return kSimpleBitSetSize;
  }

  inline std::bitset<NB> ToBitset() const {
    std::bitset<NB> bs;
    std::bitset<NB> tmp;
    for (size_t i = 0; i < kSimpleBitSetSize; ++i) {
      tmp = this->GetWord(i);
      tmp <<= (i * kSimpleBitSetAligned);
      bs |= tmp;
    }
    return bs;
  }

  inline size_t GetHashOfbitset() const {
    std::hash<std::bitset<NB>> hashFn;
    return hashFn(this->ToBitset());
  }

 private:
  template <typename T>
  T &BitMask(T &input, size_t bit) {  // retrive input's $bit bits
    if (bit != 0) {
      input &= (~(kAllOnes << bit));
    }
    return input;
  }

  // @param2 is used to avoid shift count >= width of type [-Werror,-Wshift-count-overflow]
  template <typename T, std::enable_if_t<sizeof(T) % 8 == 0, bool> = true>  // 8 bits
  inline void CheckAndSet(const T &input, std::function<void(WordT &, T &)> set,
                          std::function<void(WordT &, T &, size_t)> lastSet) {
    size_t inputBitSize = sizeof(input) * 8;  // 8 bits
    CALC_INDEX_AND_BIT(inputBitSize, index, bit);
    if (unlikely(index >= kSimpleBitSetSize)) {
      index = kSimpleBitSetSize - 1;
      bit = 0;
    }
    T tmp = input;
    if (sizeof(T) == sizeof(WordT)) {
      set(bitSet[0], tmp);
    } else {
      for (size_t i = 0; i < index; ++i) {
        set(bitSet[i], tmp);
        tmp >>= kSimpleBitSetAligned;
      }
      lastSet(bitSet[index], tmp, bit);
    }
  }

  /**
    *
    * @param input
    * @param normalSet  handle (input size <= this size) part
    * @param overflowSet handle (this size - input size) part
    */
  inline void Set(const SimpleBitSet &input, std::function<void(WordT &dst, WordT src)> normalSet,
                  std::function<void(WordT *startPos, size_t leftSize)> overflowSet) {
    if (input.GetWordSize() <= kSimpleBitSetSize) {
      size_t i = 0;
      for (; i < input.GetWordSize(); ++i) {
        normalSet(bitSet[i], input.GetWord(i));
      }
      overflowSet(&bitSet[i], (kSimpleBitSetSize - i));
    } else if (input.GetWordSize() > kSimpleBitSetSize) {
      // ignore overflow part
      for (size_t i = 0; i < kSimpleBitSetSize; ++i) {
        normalSet(bitSet[i], input.GetWord(i));
      }
    }
  }

#undef CALC_INDEX_AND_BIT
  WordT bitSet[kSimpleBitSetSize]{};
};
} // namespace maple
#endif  // SIMPLE_BIT_SET_H