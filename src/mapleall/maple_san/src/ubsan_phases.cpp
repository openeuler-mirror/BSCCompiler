#ifdef ENABLE_MAPLE_SAN

#include "ubsan_phases.h"
#include "ubsan_bounds.h"


namespace maple {
  void MEDoUbsanBound::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
    aDep.SetPreservedAll();
  }

  bool MEDoUbsanBound::PhaseRun(MeFunction &func) {
    BoundCheck boundCheck(&func);
    boundCheck.addBoundsChecking();
    return true;
  }
}

#endif