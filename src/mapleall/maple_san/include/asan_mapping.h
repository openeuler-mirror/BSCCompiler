//
// Created by wchenbt on 4/1/21.
//
#ifndef MAPLE_SAN_INCLUDE_ASAN_MAPPING_H
#define MAPLE_SAN_INCLUDE_ASAN_MAPPING_H

#include "san_common.h"
#include <cstdint>

static const uint64_t kDefaultShadowScale = 3;
static const uint64_t kDefaultShadowOffset32 = 1ULL << 29;  // 0x20000000
static const uint64_t kDefaultShadowOffset64 = 1ULL << 44;
static const uint64_t kDynamicShadowSentinel =
        std::numeric_limits<uint64_t>::max();
static const uint64_t kSmallX86_64ShadowOffsetBase = 0x7FFFFFFF;  // < 2G.
static const uint64_t kSmallX86_64ShadowOffsetAlignMask = ~0xFFFULL;;
static const uint64_t kAArch64_ShadowOffset64 = 1ULL << 36;

struct ShadowMapping {
  int Scale;
  uint64_t Offset;
  bool OrShadowOffset;
};

static inline ShadowMapping getShadowMapping() {
  ShadowMapping Mapping;
  Mapping.Scale = kDefaultShadowScale;
#if TARGAARCH64
  Mapping.Offset = kAArch64_ShadowOffset64;
  Mapping.OrShadowOffset = false;
#elif TARGX86_64
  Mapping.Offset = (kSmallX86_64ShadowOffsetBase &
                          (kSmallX86_64ShadowOffsetAlignMask << Mapping.Scale));
    Mapping.OrShadowOffset = !(Mapping.Offset & (Mapping.Offset - 1)) &&
                               Mapping.Offset != kDynamicShadowSentinel;
#elif TARGX86 || TARGARM32 || TARGVM
      Mapping.Offset = kDefaultShadowOffset32;
      Mapping.OrShadowOffset = !(Mapping.Offset & (Mapping.Offset - 1)) &&
                               Mapping.Offset != kDynamicShadowSentinel;
#else
      Mapping.Offset = kDefaultShadowOffset64;
      Mapping.OrShadowOffset = !(Mapping.Offset & (Mapping.Offset - 1)) &&
                               Mapping.Offset != kDynamicShadowSentinel;
#endif
  return Mapping;
}

#endif  // MAPLE_SAN_INCLUDE_ASAN_MAPPING_H
