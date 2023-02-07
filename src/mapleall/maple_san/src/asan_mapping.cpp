//
// Created by wchenbt on 4/1/21.
//

#include "asan_mapping.h"

#include <iostream>
#include <string>

#include "san_common.h"

namespace maple {

inline uint64_t alignTo(uint64_t Value, uint64_t Align, uint64_t Skew = 0) {
  assert(Align != 0u && "Align can't be 0.");
  Skew %= Align;
  return (Value + Align - 1 - Skew) / Align * Align + Skew;
}

static inline bool CompareVars(const ASanStackVariableDescription &a, const ASanStackVariableDescription &b) {
  return a.Alignment > b.Alignment;
}

static size_t VarAndRedzoneSize(size_t Size, size_t Granularity, size_t Alignment) {
  size_t Res = 0;
  if (Size <= 4) {
    Res = 16;
  } else if (Size <= 16) {
    Res = 32;
  } else if (Size <= 128) {
    Res = Size + 32;
  } else if (Size <= 512) {
    Res = Size + 64;
  } else if (Size <= 4096) {
    Res = Size + 128;
  } else {
    Res = Size + 256;
  }
  return alignTo(std::max(Res, 2 * Granularity), Alignment);
}

std::string ComputeASanStackFrameDescription(const std::vector<ASanStackVariableDescription> &vars) {
  std::stringstream stackDescription;
  stackDescription << vars.size();

  for (const auto &var : vars) {
    std::string name = var.Name;
    /*if (var.Line) {
        name += ":";
        name += std::to_string(var.Line);
      }*/
    stackDescription << " " << var.Offset << " " << var.Size << " " << name.size() << " " << name;
  }
  return stackDescription.str();
}

std::vector<uint8_t> GetShadowBytes(const std::vector<ASanStackVariableDescription> &Vars,
                                    const ASanStackFrameLayout &Layout) {
  assert(Vars.size() > 0);
  std::vector<uint8_t> vector;
  vector.clear();
  const size_t granularity = Layout.Granularity;
  vector.resize(Vars[0].Offset / granularity, kAsanStackLeftRedzoneMagic);
  for (const auto &var : Vars) {
    vector.resize(var.Offset / granularity, kAsanStackMidRedzoneMagic);

    vector.resize(vector.size() + var.Size / granularity, 0);
    if (var.Size % granularity) {
      vector.push_back(var.Size % granularity);
    }
  }
  vector.resize(Layout.FrameSize / granularity, kAsanStackRightRedzoneMagic);
  return vector;
}

std::vector<uint8_t> GetShadowBytesAfterScope(const std::vector<ASanStackVariableDescription> &Vars,
                                              const ASanStackFrameLayout &Layout) {
  std::vector<uint8_t> SB = GetShadowBytes(Vars, Layout);
  const size_t Granularity = Layout.Granularity;

  for (const auto &Var : Vars) {
    assert(Var.LifetimeSize <= Var.Size);
    const size_t LifetimeShadowSize = (Var.LifetimeSize + Granularity - 1) / Granularity;
    const size_t Offset = Var.Offset / Granularity;
    std::fill(SB.begin() + Offset, SB.begin() + Offset + LifetimeShadowSize, kAsanStackUseAfterScopeMagic);
  }

  return SB;
}

ASanStackFrameLayout ComputeASanStackFrameLayout(std::vector<ASanStackVariableDescription> &Vars, size_t Granularity,
                                                 size_t MinHeaderSize) {
  assert(Granularity >= 8 && Granularity <= 64 && (Granularity & (Granularity - 1)) == 0);
  assert(MinHeaderSize >= 16 && (MinHeaderSize & (MinHeaderSize - 1)) == 0 && MinHeaderSize >= Granularity);
  const size_t NumVars = Vars.size();
  assert(NumVars > 0);
  for (size_t i = 0; i < NumVars; i++) {
    Vars[i].Alignment = std::max(Vars[i].Alignment, kMinAlignment);
  }

  std::sort(Vars.begin(), Vars.end(), CompareVars);

  ASanStackFrameLayout Layout;
  Layout.Granularity = Granularity;
  Layout.FrameAlignment = std::max(Granularity, Vars[0].Alignment);
  size_t Offset = std::max(std::max(MinHeaderSize, Granularity), Vars[0].Alignment);
  CHECK_FATAL((Offset % Granularity) == 0, "Offset cannot be divided by size of each item.");
  for (size_t i = 0; i < NumVars; i++) {
    bool IsLast = i == NumVars - 1;
    size_t Alignment = std::max(Granularity, Vars[i].Alignment);
    (void)Alignment;  // Used only in asserts.
    size_t Size = Vars[i].Size;
    CHECK_FATAL((Alignment & (Alignment - 1)) == 0, "`Alignment = 1` is not supported");
    CHECK_FATAL(Layout.FrameAlignment >= Alignment, "Stack frame alignment is smaller than the alignment");
    CHECK_FATAL((Offset % Alignment) == 0, "Offset cannot be divided by alignment");
    CHECK_FATAL(Size > 0, "We get variable with 0 byte");
    size_t NextAlignment = IsLast ? Granularity : std::max(Granularity, Vars[i + 1].Alignment);
    size_t SizeWithRedzone = VarAndRedzoneSize(Size, Granularity, NextAlignment);
    Vars[i].Offset = Offset;
    Offset += SizeWithRedzone;
  }
  if (Offset % MinHeaderSize) {
    Offset += MinHeaderSize - (Offset % MinHeaderSize);
  }
  Layout.FrameSize = Offset;
  assert((Layout.FrameSize % MinHeaderSize) == 0);
  return Layout;
}
}  // namespace maple
