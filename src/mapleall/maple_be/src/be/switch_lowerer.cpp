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
/*
 * This module analyzes the tag distribution in a switch statement and decides
 * the best strategy in terms of runtime performance to generate code for it.
 * The generated code makes use of 3 code generation techniques:
 *
 * 1. cascade of if-then-else based on equality test
 * 2. rangegoto
 * 3. binary search
 *
 * 1 is applied only if the number of possibilities is <= 6.
 * 2 corresponds to indexed jump, but it requires allocating an array
 * initialized with the jump targets.  Since it causes memory usage overhead,
 * rangegoto is used only if the density is higher than 0.7.
 * If neither 1 nor 2 is applicable, 3 is applied in the form of a decision
 * tree.  In this case, each test would split the tags into 2 halves.  For
 * each half, the above algorithm is then applied recursively until the
 * algorithm terminates.
 *
 * But we don't want to apply 3 right from the beginning if both 1 and 2 do not
 * apply, because there may be regions that have density > 0.7.  Thus, the
 * switch lowerer begins by finding clusters.  A cluster is defined to be a
 * maximal range of tags whose density is > 0.7.
 *
 * In finding clusters, the original switch table is sorted and then each dense
 * region is condensed into 1 switch item; in the switch_items table, each item // either corresponds to an original
 * entry in the original switch table (pair's // second is 0), or to a dense region (pair's second gives the upper limit
 * of the dense range).  The output code is generated based on the switch_items. See BuildCodeForSwitchItems() which is
 * recursive.
*/
#include "switch_lowerer.h"
#include "mir_nodes.h"
#include "mir_builder.h"
#include "mir_lower.h"  /* "../../../maple_ir/include/mir_lower.h" */

namespace maplebe {
static bool CasePairKeyLessThan(const CasePair &left, const CasePair &right) {
  return left.first < right.first;
}

void SwitchLowerer::FindClusters(MapleVector<Cluster> &clusters) const {
  int32 length = static_cast<int>(stmt->GetSwitchTable().size());
  int32 i = 0;
  while (i < length - kClusterSwitchCutoff) {
    for (int32 j = length - 1; j > i; --j) {
      float tmp1 = static_cast<float>(j - i);
      float tmp2 = static_cast<float>(stmt->GetCasePair(static_cast<size_t>(static_cast<uint>(j))).first) -
                   static_cast<float>(stmt->GetCasePair(static_cast<size_t>(static_cast<uint>(i))).first);
      float currDensity = tmp1 / tmp2;
      if (((j - i) >= kClusterSwitchCutoff) &&
          ((currDensity >= kClusterSwitchDensityHigh) ||
           ((currDensity >= kClusterSwitchDensityLow) && (tmp2 < kMaxRangeGotoTableSize)))) {
        clusters.emplace_back(Cluster(i, j));
        i = j;
        break;
      }
    }
    ++i;
  }
}

void SwitchLowerer::InitSwitchItems(MapleVector<Cluster> &clusters) {
  if (clusters.empty()) {
    for (int32 i = 0; i < static_cast<int>(stmt->GetSwitchTable().size()); ++i) {
      switchItems.emplace_back(SwitchItem(i, 0));
    }
  } else {
    int32 j = 0;
    Cluster front = clusters[j];
    for (int32 i = 0; i < static_cast<int>(stmt->GetSwitchTable().size()); ++i) {
      if (i == front.first) {
        switchItems.emplace_back(SwitchItem(i, front.second));
        i = front.second;
        ++j;
        if (static_cast<int>(clusters.size()) > j) {
          front = clusters[j];
        }
      } else {
        switchItems.emplace_back(SwitchItem(i, 0));
      }
    }
  }
}

RangeGotoNode *SwitchLowerer::BuildRangeGotoNode(int32 startIdx, int32 endIdx, LabelIdx newLabelIdx) {
  RangeGotoNode *node = mirModule.CurFuncCodeMemPool()->New<RangeGotoNode>(mirModule);
  node->SetOpnd(stmt->GetSwitchOpnd(), 0);

  node->SetRangeGotoTable(SmallCaseVector(mirModule.CurFuncCodeMemPoolAllocator()->Adapter()));
  node->SetTagOffset(static_cast<int32>(stmt->GetCasePair(static_cast<size_t>(startIdx)).first));
  uint32 curTag = 0;
  node->AddRangeGoto(curTag, stmt->GetCasePair(startIdx).second);
  int64 lastCaseTag = stmt->GetSwitchTable().at(startIdx).first;
  for (int32 i = startIdx + 1; i <= endIdx; ++i) {
    /*
     * The second condition is to solve the problem that compilation falls into a dead loop,
     * because in some cases the two will fall into a dead loop if they are equal.
     */
    while ((stmt->GetCasePair(i).first != (lastCaseTag + 1)) && (stmt->GetCasePair(i).first != lastCaseTag)) {
      /* fill in a gap in the case tags */
      curTag = static_cast<uint32>(static_cast<int32>(++lastCaseTag) - node->GetTagOffset());
      if (stmt->GetDefaultLabel() != 0) {
        node->AddRangeGoto(curTag, stmt->GetDefaultLabel());
      } else if (newLabelIdx != 0) {
        node->AddRangeGoto(curTag, newLabelIdx);
      }
    }
    curTag = static_cast<uint32>(stmt->GetCasePair(static_cast<size_t>(i)).first - node->GetTagOffset());
    node->AddRangeGoto(curTag, stmt->GetCasePair(i).second);
    lastCaseTag = stmt->GetCasePair(i).first;
  }
  /* If the density is high enough, the range is allowed to be large */
  // ASSERT(static_cast<int32>(node->GetRangeGotoTable().size()) <= kMaxRangeGotoTableSize,
  //       "rangegoto table exceeds allowed number of entries");
  ASSERT(node->GetNumOpnds() == 1, "RangeGotoNode is a UnaryOpnd and numOpnds must be 1");
  return node;
}

CompareNode *SwitchLowerer::BuildCmpNode(Opcode opCode, uint32 idx) {
  CompareNode *binaryExpr = mirModule.CurFuncCodeMemPool()->New<CompareNode>(opCode);
  binaryExpr->SetPrimType(PTY_u32);
  binaryExpr->SetOpndType(stmt->GetSwitchOpnd()->GetPrimType());

  MIRType &type = *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(stmt->GetSwitchOpnd()->GetPrimType()));
  MIRConst *constVal =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(stmt->GetCasePair(idx).first), type);
  ConstvalNode *exprConst = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>();
  exprConst->SetPrimType(stmt->GetSwitchOpnd()->GetPrimType());
  exprConst->SetConstVal(constVal);

  binaryExpr->SetBOpnd(stmt->GetSwitchOpnd(), 0);
  binaryExpr->SetBOpnd(exprConst, 1);
  return binaryExpr;
}

GotoNode *SwitchLowerer::BuildGotoNode(int32 idx) {
  if (idx == -1 && stmt->GetDefaultLabel() == 0) {
    return nullptr;
  }
  GotoNode *gotoStmt = mirModule.CurFuncCodeMemPool()->New<GotoNode>(OP_goto);
  if (idx == -1) {
    gotoStmt->SetOffset(stmt->GetDefaultLabel());
  } else {
    gotoStmt->SetOffset(stmt->GetCasePair(idx).second);
  }
  return gotoStmt;
}

CondGotoNode *SwitchLowerer::BuildCondGotoNode(int32 idx, Opcode opCode, BaseNode &cond) {
  if (idx == -1 && stmt->GetDefaultLabel() == 0) {
    return nullptr;
  }
  CondGotoNode *cGotoStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(opCode);
  cGotoStmt->SetOpnd(&cond, 0);
  if (idx == -1) {
    cGotoStmt->SetOffset(stmt->GetDefaultLabel());
  } else {
    cGotoStmt->SetOffset(stmt->GetCasePair(idx).second);
  }
  return cGotoStmt;
}

FreqType SwitchLowerer::SumFreq(uint32 startIdx, uint32 endIdx) {
  ASSERT(startIdx >= 0 && endIdx >=0 && endIdx >= startIdx, "startIdx or endIdx is invalid");
  if (Options::profileUse && cgLowerer->GetLabel2Freq().size() > 0 &&
      (startIdx <= switchItems.size() - 1) && (endIdx <= switchItems.size() - 1) &&
      (startIdx <= endIdx)) {
    FreqType freqSum = 0;
    bool valid = false;
    // Tolerate -1 in [startIdx, endIdx]?
    for (uint32 swIdx = startIdx; swIdx <= endIdx; swIdx++) {
      FreqType freq;
      if (switchItems[swIdx].second == 0) {
        freq = cgLowerer->GetLabel2Freq()[stmt->GetCasePair(static_cast<uint32>(switchItems[swIdx].first)).second];
        if (freq >= 0) {
          freqSum += freq;
          valid = true;
        }
      } else {
        for (int32 caseIdx = switchItems[swIdx].first; caseIdx <= switchItems[swIdx].second; caseIdx++) {
          freq = cgLowerer->GetLabel2Freq()[stmt->GetCasePair(static_cast<uint32>(caseIdx)).second];
          if (freq >= 0) {
            freqSum += freq;
            valid = true;
          }
        }
      }
    }
    return valid ? freqSum : -1;
  } else {
    return -1;
  }
}

/* start and end is with respect to switchItems */
BlockNode *SwitchLowerer::BuildCodeForSwitchItems(int32 start, int32 end, bool lowBlockNodeChecked,
                                                  bool highBlockNodeChecked, FreqType freqSum, LabelIdx newLabelIdx) {
  ASSERT(start >= 0, "invalid args start");
  ASSERT(end >= 0, "invalid args end");
  BlockNode *localBlk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
  FuncProfInfo *funcProfData = mirModule.CurFunction()->GetFuncProfData();
  FreqType freqSumChecked = 0;
  if (Options::profileUse && funcProfData != nullptr) {
    funcProfData->SetStmtFreq(localBlk->GetStmtID(), freqSum);
  }
  if (start > end) {
    return localBlk;
  }
  CondGotoNode *cGoto = nullptr;
  RangeGotoNode *rangeGoto = nullptr;
  IfStmtNode *ifStmt = nullptr;
  CompareNode *cmpNode = nullptr;
  MIRLower mirLowerer(mirModule, mirModule.CurFunction());
  mirLowerer.Init();
  /* if low side starts with a dense item, handle it first */
  while ((start <= end) && (switchItems[start].second != 0)) {
    if (!lowBlockNodeChecked) {
      lowBlockNodeChecked = true;
      if (!(IsUnsignedInteger(stmt->GetSwitchOpnd()->GetPrimType()) &&
          (stmt->GetCasePair(static_cast<size_t>(switchItems[static_cast<uint64>(start)].first)).first == 0))) {
        cGoto = BuildCondGotoNode(-1, OP_brtrue, *BuildCmpNode(OP_lt, switchItems[start].first));
        if (cGoto != nullptr) {
          localBlk->AddStatement(cGoto);
          if (Options::profileUse && funcProfData != nullptr) {
            funcProfData->SetStmtFreq(cGoto->GetStmtID(), freqSum - freqSumChecked);
          }
        }
      }
    }
    rangeGoto = BuildRangeGotoNode(switchItems[static_cast<uint32>(start)].first,
                                   switchItems[static_cast<uint32>(start)].second, newLabelIdx);
    if (Options::profileUse && funcProfData != nullptr) {
      funcProfData->SetStmtFreq(rangeGoto->GetStmtID(), freqSum - freqSumChecked);
      freqSumChecked += SumFreq(static_cast<uint32>(start), static_cast<uint32>(start));
    }
    if (stmt->GetDefaultLabel() == 0) {
      localBlk->AddStatement(rangeGoto);
    } else {
      cmpNode = BuildCmpNode(OP_le, switchItems[static_cast<uint32>(start)].second);
      ifStmt = static_cast<IfStmtNode*>(mirModule.GetMIRBuilder()->CreateStmtIf(cmpNode));
      ifStmt->GetThenPart()->AddStatement(rangeGoto);
      if (Options::profileUse && funcProfData != nullptr) {
        funcProfData->SetStmtFreq(ifStmt->GetThenPart()->GetStmtID(),
                                  freqSum + SumFreq(static_cast<uint32>(start), static_cast<uint32>(start)));
        funcProfData->SetStmtFreq(ifStmt->GetStmtID(),
                                  freqSum + SumFreq(static_cast<uint32>(start), static_cast<uint32>(start)));
      }
      localBlk->AppendStatementsFromBlock(*mirLowerer.LowerIfStmt(*ifStmt, false));
    }
    if (start < end) {
      lowBlockNodeChecked = (stmt->GetCasePair(switchItems[static_cast<uint32>(start)].second).first + 1 ==
                       stmt->GetCasePair(switchItems[static_cast<uint32>(start) + 1].first).first);
    }
    ++start;
  }
  /* if high side starts with a dense item, handle it also */
  while ((start <= end) && (switchItems[static_cast<uint32>(end)].second != 0)) {
    if (!highBlockNodeChecked) {
      cGoto = BuildCondGotoNode(-1, OP_brtrue, *BuildCmpNode(OP_gt, switchItems[end].second));
      if (cGoto != nullptr) {
        localBlk->AddStatement(cGoto);
        if (Options::profileUse && funcProfData != nullptr) {
          funcProfData->SetStmtFreq(cGoto->GetStmtID(), freqSum - freqSumChecked);
        }
      }
      highBlockNodeChecked = true;
    }
    rangeGoto = BuildRangeGotoNode(switchItems[static_cast<uint32>(end)].first,
                                   switchItems[static_cast<uint32>(end)].second, newLabelIdx);
    if (Options::profileUse && funcProfData != nullptr) {
      funcProfData->SetStmtFreq(rangeGoto->GetStmtID(), freqSum - freqSumChecked);
      freqSumChecked += SumFreq(static_cast<uint32>(end), static_cast<uint32>(end));
    }
    if (stmt->GetDefaultLabel() == 0) {
      localBlk->AddStatement(rangeGoto);
    } else {
      cmpNode = BuildCmpNode(OP_ge, switchItems[static_cast<size_t>(end)].first);
      ifStmt = static_cast<IfStmtNode*>(mirModule.GetMIRBuilder()->CreateStmtIf(cmpNode));
      ifStmt->GetThenPart()->AddStatement(rangeGoto);
      if (Options::profileUse && funcProfData != nullptr) {
        funcProfData->SetStmtFreq(ifStmt->GetThenPart()->GetStmtID(),
                                  freqSum + SumFreq(static_cast<uint32>(end), static_cast<uint32>(end)));
        funcProfData->SetStmtFreq(ifStmt->GetStmtID(),
                                  freqSum + SumFreq(static_cast<uint32>(end), static_cast<uint32>(end)));
      }
      localBlk->AppendStatementsFromBlock(*mirLowerer.LowerIfStmt(*ifStmt, false));
    }
    if (start < end) {
      highBlockNodeChecked =
          (stmt->GetCasePair(switchItems[static_cast<uint32>(end)].first).first - 1 ==
           stmt->GetCasePair(switchItems[static_cast<uint32>(end) - 1].first).first) ||
          (stmt->GetCasePair(switchItems[static_cast<uint32>(end)].first).first - 1 ==
           stmt->GetCasePair(switchItems[static_cast<uint32>(end) - 1].second).first);
    }
    --end;
  }
  if (start > end) {
    if (!lowBlockNodeChecked || !highBlockNodeChecked) {
      GotoNode *gotoDft = BuildGotoNode(-1);
      if (gotoDft != nullptr) {
        localBlk->AddStatement(gotoDft);
        if (Options::profileUse && funcProfData != nullptr) {
          funcProfData->SetStmtFreq(gotoDft->GetStmtID(), freqSum - freqSumChecked);
        }
        jumpToDefaultBlockGenerated = true;
      }
    }
    return localBlk;
  }
  if ((start == end) && lowBlockNodeChecked && highBlockNodeChecked) {
    /* only 1 case with 1 tag remains */
    auto *gotoStmt = BuildGotoNode(switchItems[static_cast<uint32>(start)].first);
    if (gotoStmt != nullptr) {
      localBlk->AddStatement(gotoStmt);
      if (Options::profileUse && funcProfData != nullptr) {
        funcProfData->SetStmtFreq(gotoStmt->GetStmtID(), freqSum - freqSumChecked);
      }
    }
    return localBlk;
  }
  if (end < (start + kClusterSwitchCutoff)) {
    /* generate equality checks for what remains */
    std::vector <std::pair<FreqType, int32>> freq2case;
    int32 lastIdx = -1;
    bool freqPriority = false;
    // The setting of kClusterSwitchDensityLow to such a lower value (0.2) makes other strategies less useful
    if (Options::profileUse && funcProfData != nullptr && (cgLowerer->GetLabel2Freq().size() != 0)) {
      for (int32 idx = start; idx <= end; idx++) {
        if (switchItems[static_cast<uint32>(idx)].second == 0) {
          freq2case.push_back(std::make_pair(cgLowerer->GetLabel2Freq()[stmt->GetCasePair(static_cast<uint32>(
              switchItems[static_cast<uint32>(idx)].first)).second], switchItems[static_cast<uint32>(idx)].first));
          lastIdx = idx;
        } else {
          break;
        }
      }

      std::sort(freq2case.rbegin(), freq2case.rend());
      if (freq2case.size() > 0 && freq2case[0].first != freq2case[freq2case.size() - 1].first) {
        freqPriority = true;
      }
    }

    if (Options::profileUse && funcProfData != nullptr && freqPriority) {
      for (std::pair<FreqType, int32> f2c : freq2case) {
        uint32 idx = static_cast<uint32>(f2c.second);
        cGoto = BuildCondGotoNode(static_cast<int32>(idx), OP_brtrue, *BuildCmpNode(OP_eq, idx));
        if (cGoto != nullptr) {
          localBlk->AddStatement(cGoto);
          funcProfData->SetStmtFreq(cGoto->GetStmtID(), freqSum - freqSumChecked);
          freqSumChecked += cgLowerer->GetLabel2Freq()[stmt->GetCasePair(idx).second];
        }
      }

      if (lastIdx != -1) {
        if (lowBlockNodeChecked && (lastIdx < end)) {
          lowBlockNodeChecked = (
              stmt->GetCasePair(static_cast<uint32>(switchItems[static_cast<uint32>(lastIdx)].first)).first + 1 ==
              stmt->GetCasePair(static_cast<uint32>(switchItems[static_cast<uint32>(lastIdx + 1)].first)).first);
        }
        start = lastIdx + 1;
      }
    } else {
      while ((start <= end) && (switchItems[static_cast<uint32>(start)].second == 0)) {
        if ((start == end) && lowBlockNodeChecked && highBlockNodeChecked) {
          /* can omit the condition */
          cGoto = reinterpret_cast<CondGotoNode*>(BuildGotoNode(switchItems[static_cast<uint32>(start)].first));
        } else {
          cGoto = BuildCondGotoNode(switchItems[static_cast<uint32>(start)].first, OP_brtrue,
              *BuildCmpNode(OP_eq, static_cast<uint32>(switchItems[static_cast<uint32>(start)].first)));
        }
        if (cGoto != nullptr) {
          localBlk->AddStatement(cGoto);
        }
        if (lowBlockNodeChecked && (start < end)) {
          lowBlockNodeChecked = (
              stmt->GetCasePair(static_cast<uint32>(switchItems[static_cast<uint32>(start)].first)).first + 1 ==
              stmt->GetCasePair(static_cast<uint32>(switchItems[static_cast<uint32>(start + 1)].first)).first);
        }
        ++start;
      }
    }
    if (start <= end) {  /* recursive call */
      BlockNode *tmp = BuildCodeForSwitchItems(start, end, lowBlockNodeChecked, highBlockNodeChecked,
                                               SumFreq(static_cast<uint32>(start), static_cast<uint32>(end)));
      CHECK_FATAL(tmp != nullptr, "tmp should not be nullptr");
      localBlk->AppendStatementsFromBlock(*tmp);
    } else if (!lowBlockNodeChecked || !highBlockNodeChecked) {
      GotoNode *gotoDft = BuildGotoNode(-1);
      if (gotoDft != nullptr) {
        localBlk->AddStatement(gotoDft);
        jumpToDefaultBlockGenerated = true;
      }
    }
    return localBlk;
  }

  int64 lowestTag = stmt->GetCasePair(static_cast<uint32>(switchItems[static_cast<uint32>(start)].first)).first;
  int64 highestTag = stmt->GetCasePair(static_cast<uint32>(switchItems[static_cast<uint32>(end)].first)).first;

  /*
   * if lowestTag and higesttag have the same sign, use difference
   * if lowestTag and higesttag have the diefferent sign, use sum
   * 1LL << 63 judge lowestTag ^ highestTag operate result highest
   * bit is 1 or not, the result highest bit is 1 express lowestTag
   * and highestTag have same sign , otherwise diefferent sign.highestTag
   * add or subtract lowestTag divide 2 to get middle tag.
   */
  int64 middleTag = ((((static_cast<uint64>(lowestTag)) ^ (static_cast<uint64>(highestTag))) & (1ULL << 63)) == 0)
                      ? (highestTag - lowestTag) / 2 + lowestTag
                      : (highestTag + lowestTag) / 2;
  /* find the mid-point in switch_items between start and end */
  int32 mid = start;
  while (stmt->GetCasePair(switchItems[static_cast<uint32>(mid)].first).first < middleTag) {
    ++mid;
  }
  ASSERT(mid >= start, "switch lowering logic mid should greater than or equal start");
  ASSERT(mid <= end, "switch lowering logic mid should less than or equal end");
  /* generate test for binary search */
  if (stmt->GetDefaultLabel() != 0) {
    cmpNode = BuildCmpNode(OP_lt, static_cast<uint32>(switchItems[static_cast<uint32>(mid)].first));
    ifStmt = static_cast<IfStmtNode*>(mirModule.GetMIRBuilder()->CreateStmtIf(cmpNode));
    bool leftHighBNdChecked = (stmt->GetCasePair(static_cast<uint32>(switchItems.at(mid - 1).first)).first + 1 ==
                               stmt->GetCasePair(static_cast<uint32>(switchItems.at(mid).first)).first) ||
                              (stmt->GetCasePair(static_cast<uint32>(switchItems.at(mid - 1).second)).first + 1 ==
                               stmt->GetCasePair(static_cast<uint32>(switchItems.at(mid).first)).first);
    if (Options::profileUse && funcProfData != nullptr) {
      ifStmt->SetThenPart(BuildCodeForSwitchItems(start, mid - 1, lowBlockNodeChecked, leftHighBNdChecked,
          SumFreq(static_cast<uint32>(start), static_cast<uint32>(mid) - 1)));
      ifStmt->SetElsePart(BuildCodeForSwitchItems(mid, end, true, highBlockNodeChecked,
          SumFreq(static_cast<uint32>(mid), static_cast<uint32>(end))));
    } else {
      ifStmt->SetThenPart(BuildCodeForSwitchItems(start, mid - 1, lowBlockNodeChecked, leftHighBNdChecked, -1));
      ifStmt->SetElsePart(BuildCodeForSwitchItems(mid, end, true, highBlockNodeChecked, -1));
    }
    if (ifStmt->GetElsePart()) {
      ifStmt->SetNumOpnds(kOperandNumTernary);
    }
    if (Options::profileUse && funcProfData != nullptr) {
      if ((funcProfData->GetStmtFreq(ifStmt->GetThenPart()->GetStmtID()) >= 0) &&
          (funcProfData->GetStmtFreq(ifStmt->GetElsePart()->GetStmtID()) >= 0)) {
          funcProfData->SetStmtFreq(ifStmt->GetStmtID(),
              funcProfData->GetStmtFreq(ifStmt->GetThenPart()->GetStmtID()) +
              funcProfData->GetStmtFreq(ifStmt->GetElsePart()->GetStmtID()));
      } else {
        funcProfData->SetStmtFreq(ifStmt->GetStmtID(), -1);
      }
    }
    localBlk->AppendStatementsFromBlock(*mirLowerer.LowerIfStmt(*ifStmt, false));
  }
  return localBlk;
}

BlockNode *SwitchLowerer::LowerSwitch(LabelIdx newLabelIdx) {
  if (stmt->GetSwitchTable().empty()) {  /* change to goto */
    BlockNode *localBlk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    GotoNode *gotoDft = BuildGotoNode(-1);
    if (gotoDft != nullptr) {
      localBlk->AddStatement(gotoDft);
    }
    return localBlk;
  }

  // add case labels to label table's caseLabelSet
  MIRLabelTable *labelTab = mirModule.CurFunction()->GetLabelTab();
  for (CasePair &casePair : stmt->GetSwitchTable()) {
    labelTab->caseLabelSet.insert(casePair.second);
  }

  MapleVector<Cluster> clusters(ownAllocator->Adapter());
  stmt->SortCasePair(CasePairKeyLessThan);
  FindClusters(clusters);
  InitSwitchItems(clusters);
  BlockNode *blkNode = BuildCodeForSwitchItems(0, static_cast<int>(switchItems.size()) - 1, false, false,
                                               SumFreq(0, static_cast<uint32>(switchItems.size() - 1)), newLabelIdx);
  if (!jumpToDefaultBlockGenerated) {
    GotoNode *gotoDft = BuildGotoNode(-1);
    if (gotoDft != nullptr) {
      blkNode->AddStatement(gotoDft);
    }
  }
  return blkNode;
}
}  /* namespace maplebe */
