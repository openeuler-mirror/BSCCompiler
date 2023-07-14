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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_REGISTER_LIMIT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_REGISTER_LIMIT_H

#include "aarch64_isa.h"
#include "insn.h"

namespace maplebe {
// Check insn is VectorInsn, and third opnd's element size is H.  H - half word(16-bit)
inline bool IsVectorInsnAndThirdOpndIsH(const Insn &insn) {
  if (insn.GetNumOfRegSpec() == 0 || (insn.GetOperandSize() - 1) != kInsnThirdOpnd) {
    return false;
  }
  if (!insn.GetOperand(kInsnThirdOpnd).IsRegister()) {
    return false;
  }

  // Get third opnd's vector spec
  auto regSpecIter = insn.GetRegSpecList().begin();
  for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
    auto *opndDesc = insn.GetDesc()->GetOpndDes(i);
    if (i == kInsnThirdOpnd) {
      if (opndDesc->IsVectorOperand()) {
        break;
      } else {
        return false;
      }
    }
    if (opndDesc->IsVectorOperand()) {
      ++regSpecIter;
    }
  }

  // third opnd's element size is H
  return ((*regSpecIter)->vecElementSize == k16BitSize);
}

// Third Opnd restricted to V0-V15 when element size is H(01).
inline std::pair<regno_t, regno_t> VectorByElementInsnThirdRegisterLimit(const Insn &insn, uint32 opndIdx) {
  if (opndIdx != kInsnThirdOpnd || !IsVectorInsnAndThirdOpndIsH(insn)) {
    return kInvalidRegLimit;
  }
  return {V0, V15};
}

inline std::pair<regno_t, regno_t> MOP_vsmaddvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumaddvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmullvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumullvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmullvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumullvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmullvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumullvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmull2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumull2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmuluuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmulvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlauuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlauuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlavvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlavvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlsuuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlsuuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlsvvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vmlsvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmlalvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumlalvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmlalvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumlalvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmlal2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumlal2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmlslvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumlslvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmlslvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumlslvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmlsl2vvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumlsl2vvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsmlsl2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vumlsl2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmulhuuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmulhuuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmulhvvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmulhvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqrdmulhuuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqrdmulhuuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqrdmulhvvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqrdmulhvvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmulluuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmullvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmullvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmull2vvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmull2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlalvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlalvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlal2vvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlal2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlslvuuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlslvuvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlsl2vvuRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}

inline std::pair<regno_t, regno_t> MOP_vsqdmlsl2vvvRegisterLimit(const Insn &insn, uint32 opndIdx) {
  return VectorByElementInsnThirdRegisterLimit(insn, opndIdx);
}
}  // namespace maplebe
#endif // MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_REGISTER_LIMIT_H