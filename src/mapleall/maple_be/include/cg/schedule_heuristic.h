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
#ifndef MAPLEBE_INCLUDE_CG_SCHEDULE_HEURISTIC_H
#define MAPLEBE_INCLUDE_CG_SCHEDULE_HEURISTIC_H

#include "deps.h"

/*
 * Define a series of priority comparison function objects.
 * @ReturnValue:
 *    - positive: node1 has higher priority
 *    - negative: node2 has higher priority
 *    - zero: node1 == node2
 * And ensure the sort is stable.
 */
namespace maplebe {
/* Prefer max delay priority */
class CompareDelay {
 public:
  int operator() (const DepNode &node1, const DepNode &node2) {
    return static_cast<int>(node1.GetDelay() - node2.GetDelay());
  }
};

/* Prefer min eStart */
class CompareEStart {
 public:
  int operator() (const DepNode &node1, const DepNode &node2) {
    return static_cast<int>(node2.GetEStart() - node1.GetEStart());
  }
};

/* Prefer less lStart */
class CompareLStart {
 public:
  int operator() (const DepNode &node1, const DepNode &node2) {
    return static_cast<int>(node2.GetLStart() - node1.GetLStart());
  }
};

/* Prefer using more unit kind */
class CompareUnitKindNum {
 public:
  explicit CompareUnitKindNum(uint32 maxUnitIndex) : maxUnitIdx(maxUnitIndex) {}

  int operator() (const DepNode &node1, const DepNode &node2) {
    bool use1 = IsUseUnitKind(node1);
    bool use2 = IsUseUnitKind(node2);
    if ((use1 && use2) || (!use1 && !use2)) {
      return 0;
    } else if (!use2) {
      return 1;
    } else {
      return -1;
    }
  }

 private:
  /* Check if a node use a specific unit kind */
  bool IsUseUnitKind(const DepNode &depNode) const {
    uint32 unitKind = depNode.GetUnitKind();
    auto idx = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
    while (idx != 0) {
      ASSERT(maxUnitIdx < kUnitKindLast, "invalid unit index");
      if (idx == maxUnitIdx) {
        return true;
      }
      unitKind &= ~(1u << (idx - 1u));
      idx = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
    }
    return false;
  }

  uint32 maxUnitIdx = 0;
};

/* Prefer slot0 */
class CompareSlotType {
 public:
  int operator() (const DepNode &node1, const DepNode &node2) {
    SlotType slotType1 = node1.GetReservation()->GetSlot();
    SlotType slotType2 = node2.GetReservation()->GetSlot();
    if (slotType1 == kSlots) {
      slotType1 = kSlot0;
    }
    if (slotType2 == kSlots) {
      slotType2 = kSlot0;
    }
    return (slotType2 - slotType1);
  }
};

/* Prefer more succNodes */
class CompareSuccNodeSize {
 public:
  int operator() (const DepNode &node1, const DepNode &node2) {
    return static_cast<int>(node1.GetSuccs().size() - node2.GetSuccs().size());
  }
};

/* Default order */
class CompareInsnID {
 public:
  int operator() (const DepNode &node1, const DepNode &node2) {
    Insn *insn1 = node1.GetInsn();
    ASSERT(insn1 != nullptr, "get insn from depNode failed");
    Insn *insn2 = node2.GetInsn();
    ASSERT(insn2 != nullptr, "get insn from depNode failed");
    return static_cast<int>(insn2->GetId() - insn1->GetId());
  }
};
} /* namespace maplebe */
#endif  // MAPLEBE_INCLUDE_CG_SCHEDULE_HEURISTIC_H
