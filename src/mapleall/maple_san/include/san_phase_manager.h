#ifndef MAPLE_SAN_INCLUDE_SAN_PHASES_MANAGER_H
#define MAPLE_SAN_INCLUDE_SAN_PHASES_MANAGER_H

#include "asan_module.h"
#include "maple_phase.h"
#include "maple_phase_manager.h"
#include "mempool.h"


namespace maple {

class MEModuleDoAsan : public FunctionPM {
public:
    explicit MEModuleDoAsan(MemPool *memPool) : FunctionPM(memPool, &id) {}
    PHASECONSTRUCTOR(MEModuleDoAsan);
    bool PhaseRun(MIRModule &m) override;
    std::string PhaseName() const override;
    ~MEModuleDoAsan() override {}
private:
    bool FuncLevelRun(MeFunction &meFunc, AnalysisDataManager &serialADM);
    void GetAnalysisDependence(AnalysisDep &aDep) const override;
    void DoPhasesPopulate(const MIRModule &mirModule);

    std::string meInput = "";
};

} // namespace maple

#endif