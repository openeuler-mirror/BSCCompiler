#ifndef MAPLE_SAN_INCLUDE_ASAN_PHASES_H
#define MAPLE_SAN_INCLUDE_ASAN_PHASES_H

#include <string>
#include "maple_phase.h"
#include "san_common.h"


namespace maple {
    MAPLE_FUNC_PHASE_DECLARE_BEGIN(MEDoVarCheck, MeFunction)
    PreAnalysis* GetResult();
    private:
        void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;
        PreAnalysis* result = nullptr;
    MAPLE_FUNC_PHASE_DECLARE_END

    MAPLE_FUNC_PHASE_DECLARE(MEDoAsan, MeFunction)

    MAPLE_MODULE_PHASE_DECLARE(MEModuleDoAsan)

} // namespace maple

#endif // MAPLE_SAN_INCLUDE_SAN_PHASES_H
