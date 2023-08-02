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
#ifndef MRT_MAPLERT_INCLUDE_ARGVALUE_H_
#define MRT_MAPLERT_INCLUDE_ARGVALUE_H_

#include "panic.h"

namespace maplert {
namespace calljavastubconst{
enum ArgsType {
  kVaArg,
  kJvalue,
  kJArray,
  kNone // no parameter
};

#if defined(__aarch64__)
static constexpr uint32_t kAArch64XregSize = 8;
static constexpr uint32_t kAArch64DregSize = 8;
static constexpr uint32_t kAArch64XregAndDregSize = kAArch64XregSize + kAArch64DregSize;
static constexpr uint32_t kRegArgsSize = kAArch64XregAndDregSize;
#elif defined(__arm__)
static constexpr uint32_t kArmRregSize = 4;
#if defined(__ARM_PCS_VFP)
static constexpr uint32_t kArmDregSize = 8;
#else
static constexpr uint32_t kArmDregSize = 0;
#endif
static constexpr uint32_t kRegArgsSize = kArmRregSize + kArmDregSize;
#else
#error "Unimplemented Architecture"
#endif
} // namespace calljavastubconst

class BaseArgValue {
 public:
  explicit BaseArgValue(const uintptr_t methodArgs)
      : methodArgs(methodArgs), values(regArgValues), valuesSize(calljavastubconst::kRegArgsSize),
        gregIdx(0), fregIdx(0), stackIdx(0) {
    for (uint32_t i = 0; i < calljavastubconst::kRegArgsSize; i++) {
      regArgValues[i].j = 0L; // clean up regArgValues
    }
  }

  virtual ~BaseArgValue() {
    if (values != regArgValues) {
      delete []values;
    }
  }

  virtual void AddReference(MObject *ref) = 0;
  virtual void AddInt32(int32_t value) = 0;
  virtual void AddInt64(int64_t value) = 0;
  virtual void AddFloat(float value) = 0;
  virtual void AddDouble(double value) = 0;
  virtual uint32_t GetFRegSize() const = 0;
  virtual uint32_t GetStackSize() const = 0;
  virtual MObject *GetReceiver() const = 0;

  jvalue *GetData() {
    return &values[0];
  }

  uintptr_t GetMethodArgs() const {
    return methodArgs;
  }

  uint32_t GetGIdx() const noexcept {
    return gregIdx;
  }

 protected:
  void Resize(uint32_t newSize) {
    if (newSize < valuesSize) {
      return;
    }

    jvalue *oldValues = values;
    uint32_t oldValuesSize = valuesSize;
    valuesSize = newSize + kIncSize;
    values = new jvalue[valuesSize];
    std::copy(oldValues, oldValues + oldValuesSize, values);
    if (oldValues != regArgValues) {
      delete []oldValues; // free dynamic array.
    }
  }

  constexpr static uint32_t kIncSize = 8;
  const uintptr_t methodArgs;
  jvalue regArgValues[calljavastubconst::kRegArgsSize];
  jvalue *values;
  uint32_t valuesSize;
  uint32_t gregIdx;
  uint32_t fregIdx;
  uint32_t stackIdx;
};

class ArgValueInterp : public BaseArgValue {
 public:
  explicit ArgValueInterp(const uintptr_t methodArgs) : BaseArgValue(methodArgs) {}
  virtual ~ArgValueInterp() = default;

  virtual void AddReference(MObject *ref) override {
    AddInt64(reinterpret_cast<int64_t>(ref));
  }

  virtual void AddInt32(int32_t value) override {
    AddInt64(value);
  }

  virtual void AddInt64(int64_t value) override {
    Resize(gregIdx + 1);
    values[gregIdx++].j = value;
  }

  virtual void AddFloat(float value) override {
    Resize(gregIdx + 1);
    values[gregIdx++].f = value;
  }

  virtual void AddDouble(double value) override {
    Resize(gregIdx + 1);
    values[gregIdx++].d = value;
  }

  virtual uint32_t GetFRegSize() const override {
    LOG(FATAL) << "ArgValueInterp.GetFREgSize: Should not reach here!" << maple::endl;
    return 0;
  }

  virtual uint32_t GetStackSize() const override {
    LOG(FATAL) << "ArgValueInterp.GetStackSize: Should not reach here!" << maple::endl;
    return 0;
  }

  virtual MObject *GetReceiver() const override {
    return reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(values[0].l));
  }
};

#if defined(__aarch64__)
class ArgValue : public BaseArgValue {
 public:
  explicit ArgValue(const uintptr_t methodArgs) : BaseArgValue(methodArgs) {
    fregIdx = calljavastubconst::kAArch64XregSize;
    stackIdx = calljavastubconst::kAArch64XregAndDregSize;
  }
  virtual ~ArgValue() = default;

  virtual void AddReference(MObject *ref) override {
    AddInt64(reinterpret_cast<int64_t>(ref));
  }

  virtual void AddInt32(int32_t value) override {
    AddInt64(value);
  }

  virtual void AddInt64(int64_t value) override {
    if (gregIdx < calljavastubconst::kAArch64XregSize) {
      Resize(gregIdx + 1);
      values[gregIdx++].j = value;
    } else {
      Resize(stackIdx + 1);
      values[stackIdx++].j = value;
    }
  }

  virtual void AddFloat(float value) override {
    double arg = 0;
    *reinterpret_cast<float*>(&arg) = value;
    AddDouble(arg);
  }

  virtual void AddDouble(double value) override {
    if (fregIdx < calljavastubconst::kAArch64XregAndDregSize) {
      Resize(fregIdx + 1);
      values[fregIdx++].d = value;
    } else {
      Resize(stackIdx + 1);
      values[stackIdx++].d = value;
    }
  }

  virtual uint32_t GetFRegSize() const override {
    return fregIdx - calljavastubconst::kAArch64XregSize;
  }

  virtual uint32_t GetStackSize() const override {
    return static_cast<uint32_t>((stackIdx - calljavastubconst::kAArch64XregAndDregSize) * sizeof(jvalue));
  }

  virtual MObject *GetReceiver() const override {
    return reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(values[0].l));
  }

  // used in annotation, confirmed size and not stored in stack
  inline MObject *GetReferenceFromGidx(uint32_t idx) const noexcept {
    return reinterpret_cast<MObject*>(values[idx].l);
  }
};


// we restructure parameter in stub, copy caller's caller parameter to caller stack
class DecodeStackArgs {
 public:
  explicit DecodeStackArgs(intptr_t *stack) : stackMemery(stack), values(kIncSize), gregIdx(0), valueIdx(0) {
    fregIdx = calljavastubconst::kAArch64XregSize;
    stackIdx = calljavastubconst::kAArch64XregAndDregSize;
  }
  ~DecodeStackArgs() {
    stackMemery = nullptr;
  }

  void DecodeReference() {
    DecodeInt64();
  }

  void DecodeInt32() {
    Resize(valueIdx);
    if (gregIdx < calljavastubconst::kAArch64XregSize) {
      values[valueIdx].i = static_cast<int32_t>(stackMemery[gregIdx]);
      gregIdx++;
    } else {
      values[valueIdx].i = static_cast<int32_t>(stackMemery[stackIdx]);
      stackIdx++;
    }
    valueIdx++;
  }

  void DecodeInt64() {
    Resize(valueIdx);
    if (gregIdx < calljavastubconst::kAArch64XregSize) {
      values[valueIdx].j = stackMemery[gregIdx];
      gregIdx++;
    } else {
      values[valueIdx].j = stackMemery[stackIdx];
      stackIdx++;
    }
    valueIdx++;
  }

  void DecodeFloat() {
    DecodeDouble();
  }

  void DecodeDouble() {
    Resize(valueIdx);
    if (fregIdx < calljavastubconst::kAArch64XregAndDregSize) {
      values[valueIdx].d = *(reinterpret_cast<double*>(&stackMemery[fregIdx]));
      fregIdx++;
    } else {
      values[valueIdx].d = *(reinterpret_cast<double*>(&stackMemery[stackIdx]));
      stackIdx++;
    }
    valueIdx++;
  }

  jvalue *GetData() {
    return &values[0];
  }

 protected:
  void Resize(uint32_t newSize) {
    if (newSize < values.size()) {
      return;
    }
    values.resize(newSize + kIncSize);
  }

  constexpr static uint32_t kIncSize = 8;
  intptr_t *stackMemery;
  std::vector<jvalue> values;
  uint32_t gregIdx;
  uint32_t fregIdx;
  uint32_t stackIdx;
  uint32_t valueIdx;
};
#elif defined(__arm__)
class ArgValue : public BaseArgValue {
 public:
  explicit ArgValue(const uintptr_t methodArgs) : BaseArgValue(methodArgs) {
    fregIdx = calljavastubconst::kArmRregSize;
    stackIdx = calljavastubconst::kArmRregSize + calljavastubconst::kArmDregSize * kWideSize;
  }
  virtual ~ArgValue() = default;

  virtual void AddReference(MObject *ref) override {
    AddInt32(static_cast<int32_t>(reinterpret_cast<int64_t>(ref)));
  }

  // used in annotation, confirmed size and not stored in stack
  inline MObject *GetReferenceFromGidx(uint32_t idx) noexcept {
    return reinterpret_cast<MObject*>(values[idx >> 1].i);
  }

  void AddInt32(int32_t *value) {
    uint32_t &valueIdx = (gregIdx >= kMaxGregSize) ? stackIdx : gregIdx;
    Resize(valueIdx >> 1);
    if ((valueIdx & 0x1) == 0) {
      values[valueIdx >> 1].i = *value;
    } else {
      *(reinterpret_cast<uint32_t*>(&values[valueIdx >> 1]) + 1) = *value;
    }
    valueIdx++;
  }

  virtual void AddInt32(int32_t value) override {
    AddInt32(&value);
  }

  void AddInt64(int64_t *value) {
    uint32_t &tempIdx = (gregIdx >= kMaxGregSize) ? stackIdx : gregIdx;
    if ((tempIdx & 0x1) == 1) {
      tempIdx++;
    }

    uint32_t &valueIdx = (gregIdx >= kMaxGregSize) ? stackIdx : gregIdx;
    Resize(valueIdx >> 1);
    values[valueIdx >> 1].j = *value;
    valueIdx += kWideSize;
  }

  virtual void AddInt64(int64_t value) override {
    AddInt64(&value);
  }

  virtual void AddFloat(float value) override {
#if defined(__ARM_PCS_VFP)
    if (fregOddSpace != 0) {
      __MRT_ASSERT((fregOddSpace & 1) == 1, "Invalid fregOddSpace");
      *(reinterpret_cast<float*>(&values[fregOddSpace >> 1]) + 1) = value;
      fregOddSpace = 0;
      return;
    }
    uint32_t &valueIdx = (fregIdx >= kMaxFregSize) ? stackIdx : fregIdx;
    Resize(valueIdx >> 1);
    if ((valueIdx & 0x1) == 0) {
      values[valueIdx >> 1].f = value;
    } else {
      *(reinterpret_cast<float*>(&values[valueIdx >> 1]) + 1) = value;
    }
    valueIdx++;
#else
    AddInt32(reinterpret_cast<int32_t*>(&value));
#endif
  }

  virtual void AddDouble(double value) override {
#if defined(__ARM_PCS_VFP)
    uint32_t &tempIdx = (fregIdx >= kMaxFregSize) ? stackIdx : fregIdx;
    if ((tempIdx & 0x1) == 1) {
      __MRT_ASSERT(fregOddSpace == 0, "Invalid float odd space!");
      if (fregIdx < kMaxFregSize) {
        fregOddSpace = fregIdx;
      }
      tempIdx++;
    }
    uint32_t &valueIdx = (fregIdx >= kMaxFregSize) ? stackIdx : fregIdx;
    Resize(valueIdx >> 1);
    values[valueIdx >> 1].d = value;
    valueIdx += kWideSize;
#else
    AddInt64(reinterpret_cast<int64_t*>(&value));
#endif
  }

  virtual uint32_t GetFRegSize() const override {
    return ((fregIdx + 1) - kMaxGregSize) >> 1;
  }

  virtual uint32_t GetStackSize() const override {
    return (((stackIdx + 1) - kMaxFregSize) >> 1) * sizeof(jvalue);
  }

  virtual MObject *GetReceiver() const override {
    return reinterpret_cast<MObject*>(values[0].i);
  }

 private:
#if defined(__ARM_PCS_VFP)
  uint32_t fregOddSpace = 0;
#endif
  constexpr static uint32_t kWideSize = 2;
  constexpr static uint32_t kMaxGregSize = calljavastubconst::kArmRregSize;
  constexpr static uint32_t kMaxFregSize = calljavastubconst::kArmRregSize +
                                           calljavastubconst::kArmDregSize * kWideSize;
};

// we restructure parameter in stub, copy caller's caller parameter to caller stack
class DecodeStackArgs {
 public:
  explicit DecodeStackArgs(intptr_t *stack)
      : stackMemery(stack), values(kIncSize), gregIdx(0), oddFregIdx(0), valueIdx(0) {
    fregIdx = calljavastubconst::kArmRregSize;
    stackIdx = calljavastubconst::kArmRregSize + calljavastubconst::kArmDregSize * kWideSize;
  }
  ~DecodeStackArgs() {
    stackMemery = nullptr;
  }

  void DecodeReference() {
    DecodeInt32();
  }

  void DecodeInt32() {
    Resize(valueIdx);
    if (gregIdx < kMaxGregSize) {
      values[valueIdx].i = stackMemery[gregIdx];
      gregIdx++;
    } else {
      values[valueIdx].i = stackMemery[stackIdx];
      stackIdx++;
    }
    valueIdx++;
  }

  void DecodeInt64() {
    Resize(valueIdx);
    if ((gregIdx & 0x1) == 0x1) {
      gregIdx++;
    }
    if (gregIdx < kMaxGregSize) {
      values[valueIdx].j = *(reinterpret_cast<int64_t*>(&stackMemery[gregIdx]));
      gregIdx += kWideSize;
    } else {
      if ((stackIdx & 0x1) == 0x1) {
        stackIdx++;
      }
      values[valueIdx].j = *(reinterpret_cast<int64_t*>(&stackMemery[stackIdx]));
      stackIdx += kWideSize;
    }
    valueIdx++;
  }

  void DecodeFloat() {
#if defined(__ARM_PCS_VFP)
    Resize(valueIdx);
    if ((oddFregIdx & 0x1) == 0x01) {
      values[valueIdx].f = *(reinterpret_cast<float*>(&stackMemery[oddFregIdx]));
      oddFregIdx = 0;
      valueIdx++;
      return;
    }
    if (fregIdx < kMaxFregSize) {
      values[valueIdx].f = *(reinterpret_cast<float*>(&stackMemery[fregIdx]));
      fregIdx++;
    } else {
      values[valueIdx].f = *(reinterpret_cast<float*>(&stackMemery[stackIdx]));
      stackIdx++;
    }
    valueIdx++;
#else
    DecodeInt32();
#endif
  }

  void DecodeDouble() {
#if defined(__ARM_PCS_VFP)
    Resize(valueIdx);
    if ((fregIdx & 0x1) == 0x1) {
      oddFregIdx = fregIdx;
      fregIdx++;
    }
    if (fregIdx < kMaxFregSize) {
      values[valueIdx].d = *(reinterpret_cast<double*>(&stackMemery[fregIdx]));
      fregIdx += kWideSize;
    } else {
      if ((stackIdx & 0x1) == 0x1) {
        stackIdx++;
      }
      values[valueIdx].d = *(reinterpret_cast<double*>(&stackMemery[stackIdx]));
      stackIdx += kWideSize;
    }
    valueIdx++;
#else
    DecodeInt64();
#endif
  }

  jvalue *GetData() {
    return &values[0];
  }

 protected:
  void Resize(uint32_t newSize) {
    if (newSize < values.size()) {
      return;
    }
    values.resize(newSize + kIncSize);
  }

  constexpr static uint32_t kIncSize = 8;
  intptr_t *stackMemery;
  std::vector<jvalue> values;
  uint32_t gregIdx;
  uint32_t fregIdx;
  uint32_t stackIdx;
  uint32_t oddFregIdx;
  uint32_t valueIdx;
  constexpr static uint32_t kWideSize = 2;
  constexpr static uint32_t kMaxGregSize = calljavastubconst::kArmRregSize;
  constexpr static uint32_t kMaxFregSize = calljavastubconst::kArmRregSize +
                                           calljavastubconst::kArmDregSize * kWideSize;
};
#endif
}
#endif
