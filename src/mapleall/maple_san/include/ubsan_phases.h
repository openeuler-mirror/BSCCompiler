#ifndef MAPLE_SAN_INCLUDE_UBSAN_PHASES_H
#define MAPLE_SAN_INCLUDE_UBSAN_PHASES_H

#include <string>
#include "maple_phase.h"
#include "san_common.h"


namespace maple {
    MAPLE_FUNC_PHASE_DECLARE(MEDoUbsanBound, MeFunction)
} // namespace maple

#endif // MAPLE_SAN_INCLUDE_UBSAN_PHASES_H
