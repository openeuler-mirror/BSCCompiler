/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_CFG_H
#define MAPLE_ME_INCLUDE_ME_CFG_H
#include "me_function.h"
#include "me_phase.h"

namespace maple {
class MeCFG : public AnalysisResult {
  using BBPtrHolder = MapleVector<BB*>;
 public:
  using value_type = BBPtrHolder::value_type;
  using size_type = BBPtrHolder::size_type;
  using difference_type = BBPtrHolder::difference_type;
  using pointer = BBPtrHolder::pointer;
  using const_pointer = BBPtrHolder::const_pointer;
  using reference = BBPtrHolder::reference;
  using const_reference = BBPtrHolder::const_reference;
  using iterator = BBPtrHolder::iterator;
  using const_iterator = BBPtrHolder::const_iterator;
  using reverse_iterator = BBPtrHolder::reverse_iterator;
  using const_reverse_iterator = BBPtrHolder::const_reverse_iterator;

  explicit MeCFG(MemPool *memPool, MeFunction &f)
      : AnalysisResult(memPool),
        mp(memPool),
        mecfgAlloc(memPool),
        func(f),
        patternSet(mecfgAlloc.Adapter()),
        bbVec(mecfgAlloc.Adapter()),
        labelBBIdMap(mecfgAlloc.Adapter()),
        bbTryNodeMap(mecfgAlloc.Adapter()),
        endTryBB2TryBB(mecfgAlloc.Adapter()),
        sccTopologicalVec(mecfgAlloc.Adapter()),
        sccOfBB(mecfgAlloc.Adapter()),
        backEdges(mecfgAlloc.Adapter()) {}

  ~MeCFG() = default;

  bool IfReplaceWithAssertNonNull(const BB &bb) const;
  void ReplaceWithAssertnonnull();
  void BuildMirCFG();
  void FixMirCFG();
  void ConvertPhis2IdentityAssigns(BB &meBB) const;
  bool UnreachCodeAnalysis(bool updatePhi = false);
  void WontExitAnalysis();
  void Verify() const;
  void VerifyLabels() const;
  void Dump() const;
  void DumpToFile(const std::string &prefix, bool dumpInStrs = false, bool dumpEdgeFreq = false) const;
  bool FindExprUse(const BaseNode &expr, StIdx stIdx) const;
  bool FindUse(const StmtNode &stmt, StIdx stIdx) const;
  bool FindDef(const StmtNode &stmt, StIdx stIdx) const;
  bool HasNoOccBetween(StmtNode &from, const StmtNode &to, StIdx stIdx) const;
  BB *NewBasicBlock();
  BB &InsertNewBasicBlock(const BB &position, bool isInsertBefore = true);
  void DeleteBasicBlock(const BB &bb);
  BB *NextBB(const BB *bb);
  BB *PrevBB(const BB *bb);
  void CloneBasicBlock(BB &newBB, const BB &orig);
  BB &SplitBB(BB &bb, StmtNode &splitPoint, BB *newBB = nullptr);
  void SplitBBPhysically(BB &bb, StmtNode &splitPoint, BB &newBB);

  const MeFunction &GetFunc() const {
    return func;
  }

  bool GetHasDoWhile() const {
    return hasDoWhile;
  }

  void SetHasDoWhile(bool hdw) {
    hasDoWhile = hdw;
  }

  MapleAllocator &GetAlloc() { return mecfgAlloc; }

  void SetNextBBId(uint32 currNextBBId) {
    nextBBId = currNextBBId;
  }
  uint32 GetNextBBId() const {
    return nextBBId;
  }
  void DecNextBBId() {
    --nextBBId;
  }

  MapleVector<BB*> &GetAllBBs() { return bbVec; }

  iterator begin() {
    return bbVec.begin();
  }
  const_iterator begin() const {
    return bbVec.begin();
  }
  const_iterator cbegin() const {
    return bbVec.cbegin();
  }

  iterator end() {
    return bbVec.end();
  }
  const_iterator end() const {
    return bbVec.end();
  }
  const_iterator cend() const {
    return bbVec.cend();
  }

  reverse_iterator rbegin() {
    return bbVec.rbegin();
  }
  const_reverse_iterator rbegin() const {
    return bbVec.rbegin();
  }
  const_reverse_iterator crbegin() const {
    return bbVec.crbegin();
  }

  reverse_iterator rend() {
    return bbVec.rend();
  }
  const_reverse_iterator rend() const {
    return bbVec.rend();
  }
  const_reverse_iterator crend() const {
    return bbVec.crend();
  }

  reference front() {
    return bbVec.front();
  }

  reference back() {
    return bbVec.back();
  }

  const_reference front() const {
    return bbVec.front();
  }

  const_reference back() const {
    return bbVec.back();
  }

  bool empty() const {
    return bbVec.empty();
  }

  size_t size() const {
    return bbVec.size();
  }
  FilterIterator<const_iterator> valid_begin() const {
    return build_filter_iterator(begin(), std::bind(FilterNullPtr<const_iterator>, std::placeholders::_1, end()));
  }

  FilterIterator<const_iterator> valid_end() const {
    return build_filter_iterator(end());
  }

  FilterIterator<const_reverse_iterator> valid_rbegin() const {
    return build_filter_iterator(rbegin(),
                                 std::bind(FilterNullPtr<const_reverse_iterator>, std::placeholders::_1, rend()));
  }

  FilterIterator<const_reverse_iterator> valid_rend() const {
    return build_filter_iterator(rend());
  }

  const_iterator common_entry() const {
    return begin();
  }

  const_iterator context_begin() const {
    return ++(++begin());
  }

  const_iterator context_end() const {
    return end();
  }

  const_iterator common_exit() const {
    return ++begin();
  }

  uint32 NumBBs() const {
    return nextBBId;
  }

  const MapleUnorderedMap<LabelIdx, BB*> &GetLabelBBIdMap() const {
    return labelBBIdMap;
  }
  BB *GetLabelBBAt(LabelIdx idx) const {
    auto it = labelBBIdMap.find(idx);
    if (it != labelBBIdMap.end()) {
      return it->second;
    }
    return nullptr;
  }

  void SetLabelBBAt(LabelIdx idx, BB *bb) {
    labelBBIdMap[idx] = bb;
  }
  void EraseLabelBBAt(LabelIdx idx) {
    labelBBIdMap.erase(idx);
  }

  BB *GetBBFromID(BBId bbID) const {
    ASSERT(bbID < bbVec.size(), "array index out of range");
    return bbVec.at(bbID);
  }

  void NullifyBBByID(BBId bbID) {
    ASSERT(bbID < bbVec.size(), "array index out of range");
    bbVec.at(bbID) = nullptr;
  }

  BB *GetCommonEntryBB() const {
    return *common_entry();
  }

  BB *GetCommonExitBB() const {
    return *common_exit();
  }

  BB *GetFirstBB() const {
    return *(++(++valid_begin()));
  }

  BB *GetLastBB() const {
    return *valid_rbegin();
  }

  void SetBBTryNodeMap(BB &bb, StmtNode &tryStmt) {
    bbTryNodeMap[&bb] = &tryStmt;
  }

  const MapleUnorderedMap<BB*, StmtNode*> &GetBBTryNodeMap() const {
    return bbTryNodeMap;
  }

  const MapleUnorderedMap<BB*, BB*> &GetEndTryBB2TryBB() const {
    return endTryBB2TryBB;
  }

  const BB *GetTryBBFromEndTryBB(BB *endTryBB) const {
    auto it = endTryBB2TryBB.find(endTryBB);
    return it == endTryBB2TryBB.end() ? nullptr : it->second;
  }
  void SetBBTryBBMap(BB *currBB, BB *tryBB) {
    endTryBB2TryBB[currBB] = tryBB;
  }
  void SetTryBBByOtherEndTryBB(BB *endTryBB, BB *otherTryBB) {
    endTryBB2TryBB[endTryBB] = endTryBB2TryBB[otherTryBB];
  }

  MemPool * GetMempool() const { return mp; }
  void CreateBasicBlocks();

  const MapleVector<SCCOfBBs*> &GetSccTopologicalVec() const {
    return sccTopologicalVec;
  }
  void BBTopologicalSort(SCCOfBBs &scc);
  void BuildSCC();

 private:
  void ReplaceSwitchContainsOneCaseBranchWithBrtrue(BB &bb, MapleVector<BB*> &exitBlocks);
  void AddCatchHandlerForTryBB(BB &bb, MapleVector<BB*> &exitBlocks);
  std::string ConstructFileNameToDump(const std::string &prefix) const;
  void DumpToFileInStrs(std::ofstream &cfgFile) const;
  void ConvertPhiList2IdentityAssigns(BB &meBB) const;
  void ConvertMePhiList2IdentityAssigns(BB &meBB) const;
  bool IsStartTryBB(BB &meBB) const;
  void FixTryBB(BB &startBB, BB &nextBB);
  void SetTryBlockInfo(const StmtNode *nextStmt, StmtNode *tryStmt, BB *lastTryBB, BB *curBB, BB *newBB);

  void VerifySCC();
  void SCCTopologicalSort(std::vector<SCCOfBBs*> &sccNodes);
  void BuildSCCDFS(BB &bb, uint32 &visitIndex, std::vector<SCCOfBBs*> &sccNodes, std::vector<uint32> &visitedOrder,
                   std::vector<uint32> &lowestOrder, std::vector<bool> &inStack, std::stack<uint32> &visitStack);

  MemPool *mp;
  MapleAllocator mecfgAlloc;
  MeFunction &func;
  MapleSet<LabelIdx> patternSet;
  BBPtrHolder  bbVec;
  MapleUnorderedMap<LabelIdx, BB*> labelBBIdMap;
  MapleUnorderedMap<BB*, StmtNode*> bbTryNodeMap;  // maps isTry bb to its try stmt
  MapleUnorderedMap<BB*, BB*> endTryBB2TryBB;      // maps endtry bb to its try bb
  bool hasDoWhile = false;
  uint32 nextBBId = 0;

  // BB SCC
  MapleVector<SCCOfBBs*> sccTopologicalVec;
  uint32 numOfSCCs = 0;
  MapleVector<SCCOfBBs*> sccOfBB;
  MapleSet<std::pair<uint32, uint32>> backEdges;
};

class MeDoMeCfg : public MeFuncPhase {
 public:
  explicit MeDoMeCfg(MePhaseID id) : MeFuncPhase(id) {}
  virtual ~MeDoMeCfg() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "mecfgbuild";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_CFG_H
