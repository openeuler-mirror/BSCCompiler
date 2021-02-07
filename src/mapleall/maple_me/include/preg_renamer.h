#ifndef MAPLEME_INCLUDE_ME_PREGRENAMER_H
#define MAPLEME_INCLUDE_ME_PREGRENAMER_H
#include "me_irmap.h"
#include "ssa_pre.h"

namespace maple {

class PregRenamer {
 public:
  PregRenamer(MemPool *mp, MeFunction *f, MeIRMap *hmap) : mp(mp), alloc(mp), func(f), meirmap(hmap) {}
  void RunSelf() const;

 private:
  std::string PhaseName() const {
    return "pregrename";
  }
 private:
  MemPool *mp;
  MapleAllocator alloc;
  MeFunction *func;
  MeIRMap *meirmap;
};

class MeDoPregRename : public MeFuncPhase {
 public:
  explicit MeDoPregRename(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoPregRename() = default;
  AnalysisResult *Run(MeFunction *ir, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "pregrename";
  }
};
}  // namespace maple
#endif  // MAPLEME_INCLUDE_ME_PREGRENAMER_H
