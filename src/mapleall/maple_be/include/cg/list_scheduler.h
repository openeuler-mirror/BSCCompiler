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

#ifndef MAPLEBE_INCLUDE_CG_LIST_SCHEDULER_H
#define MAPLEBE_INCLUDE_CG_LIST_SCHEDULER_H

#include <utility>
#include "cg.h"
#include "deps.h"
#include "cg_cdg.h"
#include "schedule_heuristic.h"

namespace maplebe {
#define LIST_SCHEDULE_DUMP CG_DEBUG_FUNC(cgFunc)
using SchedRankFunctor = bool(*)(const DepNode *node1, const DepNode *node2);

constexpr uint32 kClinitAdvanceCycle = 12;
constexpr uint32 kAdrpLdrAdvanceCycle = 4;
constexpr uint32 kClinitTailAdvanceCycle = 6;

class CommonScheduleInfo {
 public:
  explicit CommonScheduleInfo(MemPool &memPool)
      : csiAlloc(&memPool), candidates(csiAlloc.Adapter()), schedResults(csiAlloc.Adapter()) {}
  ~CommonScheduleInfo() = default;

  MapleVector<DepNode*> &GetCandidates() {
    return candidates;
  }
  void AddCandidates(DepNode *depNode) {
    (void)candidates.emplace_back(depNode);
  }
  void EraseNodeFromCandidates(const DepNode *depNode) {
    for (auto iter = candidates.begin(); iter != candidates.end(); ++iter) {
      if (*iter == depNode) {
        (void)candidates.erase(iter);
        return;
      }
    }
  }
  MapleVector<DepNode*>::iterator EraseIterFromCandidates(MapleVector<DepNode*>::iterator depIter) {
      return candidates.erase(depIter);
  }
  MapleVector<DepNode*> &GetSchedResults() {
    return schedResults;
  }
  void AddSchedResults(DepNode *depNode) {
    (void)schedResults.emplace_back(depNode);
  }
  std::size_t GetSchedResultsSize() const {
    return schedResults.size();
  }

 private:
  MapleAllocator csiAlloc;
  // Candidate instructions list of current BB,
  // by control flow sequence
  MapleVector<DepNode*> candidates;
  // Scheduled results list of current BB
  // for global-scheduler, it stored only to the last depNode of current BB
  MapleVector<DepNode*> schedResults;
};

class ListScheduler {
 public:
  ListScheduler(MemPool &memPool, CGFunc &func, SchedRankFunctor rankFunc, bool delayHeu = true, std::string pName = "")
      : listSchedMp(memPool), listSchedAlloc(&memPool), cgFunc(func), rankScheduleInsns(rankFunc),
        doDelayHeuristics(delayHeu), phaseName(std::move(pName)),
        waitingQueue(listSchedAlloc.Adapter()), readyList(listSchedAlloc.Adapter()) {}
  ListScheduler(MemPool &memPool, CGFunc &func, bool delayHeu = true, std::string pName = "")
      : listSchedMp(memPool), listSchedAlloc(&memPool), cgFunc(func),
        doDelayHeuristics(delayHeu), phaseName(std::move(pName)),
        waitingQueue(listSchedAlloc.Adapter()), readyList(listSchedAlloc.Adapter()) {}
  virtual ~ListScheduler() {
    mad = nullptr;
  }

  std::string PhaseName() const {
    if (phaseName.empty()) {
      return "listscheduler";
    } else {
      return phaseName;
    }
  }

  // The entry of list-scheduler
  // cdgNode: current scheduled BB
  void DoListScheduling();
  void ComputeDelayPriority();
  // Compute the earliest start cycle, update maxEStart
  void ComputeEStart(uint32 cycle);
  // Compute the latest start cycle
  void ComputeLStart();
  // Calculate the most used unitKind index
  void CalculateMostUsedUnitKindCount();

  void SetCommonSchedInfo(CommonScheduleInfo &csi) {
    commonSchedInfo = &csi;
  }
  void SetCDGRegion(CDGRegion &cdgRegion) {
    region = &cdgRegion;
  }
  void SetCDGNode(CDGNode &cdgNode) {
    curCDGNode = &cdgNode;
  }
  uint32 GetCurrCycle() const {
    return currCycle;
  }
  uint32 GetMaxLStart() const {
    return maxLStart;
  }
  uint32 GetMaxDelay() const {
    return maxDelay;
  }
  void SetUnitTest(bool flag) {
    isUnitTest = flag;
  }

 protected:
  void Init();
  void CandidateToWaitingQueue();
  void WaitingQueueToReadyList();
  void InitInfoBeforeCompEStart(uint32 cycle, std::vector<DepNode*> &traversalList);
  void InitInfoBeforeCompLStart(std::vector<DepNode*> &traversalList);
  void UpdateInfoBeforeSelectNode();
  void SortReadyList();
  void UpdateEStart(DepNode &schedNode);
  void UpdateInfoAfterSelectNode(DepNode &schedNode);
  void UpdateNodesInReadyList();
  void UpdateAdvanceCycle(const DepNode &schedNode);
  void CountUnitKind(const DepNode &depNode, std::array<uint32, kUnitKindLast> &unitKindCount) const;
  void DumpWaitingQueue() const;
  void DumpReadyList() const;
  void DumpScheduledResult() const;
  void DumpDelay() const;
  void DumpEStartLStartOfAllNodes();
  void DumpDepNodeInfo(const BB &curBB, MapleVector<DepNode*> &nodes, const std::string state) const;
  void DumpReservation(const DepNode &depNode) const;

  void EraseNodeFromReadyList(const DepNode *depNode) {
    for (auto iter = readyList.begin(); iter != readyList.end(); ++iter) {
      if (*iter == depNode) {
        (void)readyList.erase(iter);
        return;
      }
    }
  }
  MapleVector<DepNode*>::iterator EraseIterFromReadyList(MapleVector<DepNode*>::iterator depIter) {
    return readyList.erase(depIter);
  }

  MapleVector<DepNode*>::iterator EraseIterFromWaitingQueue(MapleVector<DepNode*>::iterator depIter) {
    return waitingQueue.erase(depIter);
  }

  // Default rank readyList function by delay heuristic,
  // which uses delay as algorithm of computing priority
  static bool DelayRankScheduleInsns(const DepNode *node1, const DepNode *node2) {
    // p as an acronym for priority
    CompareDelay compareDelay;
    int p1 = compareDelay(*node1, *node2);
    if (p1 != 0) {
      return p1 > 0;
    }

    CompareDataCache compareDataCache;
    int p2 = compareDataCache(*node1, *node2);
    if (p2 != 0) {
      return p2 > 0;
    }

    CompareClassOfLastScheduledNode compareClassOfLSN(lastSchedInsnId);
    int p3 = compareClassOfLSN(*node1, *node2);
    if (p3 != 0) {
      return p3 > 0;
    }

    CompareSuccNodeSize compareSuccNodeSize;
    int p4 = compareSuccNodeSize(*node1, *node2);
    if (p4 != 0) {
      return p4 > 0;
    }

    CompareInsnID compareInsnId;
    int p6 = compareInsnId(*node1, *node2);
    if (p6 != 0) {
      return p6 > 0;
    }

    // default
    return true;
  }

  static bool CriticalPathRankScheduleInsns(const DepNode *node1, const DepNode *node2);

  MemPool &listSchedMp;
  MapleAllocator listSchedAlloc;
  CGFunc &cgFunc;
  MAD *mad = nullptr;  // CPU resources
  CDGRegion *region = nullptr; // the current region
  CDGNode *curCDGNode = nullptr;   // the current scheduled BB
  CommonScheduleInfo *commonSchedInfo = nullptr;  // common scheduling info that prepared by other scheduler
  // The function ptr that computes instruction priority based on heuristic rules,
  // list-scheduler provides default implementations and supports customization by other schedulers
  SchedRankFunctor rankScheduleInsns = nullptr;
  bool doDelayHeuristics = true;  // true: compute delay;  false: compute eStart & lStart
  std::string phaseName;   // for dumping log
  // A node is moved from [candidates] to [waitingQueue] when it's all data dependency are met
  MapleVector<DepNode*> waitingQueue;
  // A node is moved from [waitingQueue] to [readyList] when resources required by it are free and
  // estart-cycle <= curr-cycle
  MapleVector<DepNode*> readyList;
  uint32 currCycle = 0;      // Simulates the CPU clock during scheduling
  uint32 advancedCycle = 0;  // Using after an instruction is scheduled, record its execution cycles
  uint32 maxEStart = 0;      // Update when the eStart of depNodes are recalculated
  uint32 maxLStart = 0;      // Ideal total cycles that is equivalent to critical path length
  uint32 maxDelay = 0;       // Ideal total cycles that is equivalent to max delay
  uint32 scheduledNodeNum = 0;  // The number of instructions that are scheduled
  static uint32 lastSchedInsnId;  // Last scheduled insnId, for heuristic function
  DepNode *lastSchedNode = nullptr;  // Last scheduled node
  bool isUnitTest = false;
};
} // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_LIST_SCHEDULER_H
