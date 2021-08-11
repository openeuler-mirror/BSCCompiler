### Hierarchy in maple phase
At the Current stage, there are two phases classes provided for inheritance.
Other level IR phase can be designed as a subclass of the MaplePhase class.
They perform the optimizations or generate analysis results on the specific IR level.

##### The MapleModulePhase class

If a phases is derived from MapleModulePhase, it indicates that this phase does transformation on the module.
It can run lower level IR phases manager as well.

##### The MapleFunctionPhase class

```c++
 template <class funcT>
 class MapleFunctionPhase : public MaplePhase
```

In constrast to MapleFunctionPhase, it is template class due to different Function level IRs in Maple.
Both CodeGen Function level IR and MidEnd Function level IR derives this class.

### Memory management for maple phase

Maple phase management is able to manage memory so that each phase can keep the information required by other phases and discard useless information. Each phase manager provides an analysisDataManager (In multithreading, each threads provides an analysisDataMangers) which takes responsibility for storing analysis data. 
To implement this functionality, the GetAnalysisDependence Method is required to be implemented be each phases.
```c++
 void <OPT_PHASENAME>::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<OPT_PHASENAME>();  // If a previous phase is requried to be executed
  aDep.AddPreserved<OPT_PHASENAME>(); // preserve specific previous phase information
  aDep.SetPreservedAll();             // preserve all previous phase information in analysisDataManager
}
```
#### Analysis phase

GetPhasesMempool() in Analysis phase provides mempool from analysisDataManager.
Any data that is put in analysis phase mempool is not deleted until it is declared to be discarded by other phases or the end of phase manager.
If information generated during analysis phase need to be deleted after this analysis phase, it can be put in Temp mempool in MaplePhase.

#### Transfrom phase

GetPhasesMempool() in Transform phase provides mempool which lives until this transformation finish.
**If a transform phase does not implement the GetAnalysisDependence method, it defaults to not having any prerequisite phases, and invalidating all phases information in analysisDataManager.** Transfrom phase default to be assumed to invalid all ananlysis results.


### Quick Start -- basic code

An example for function level `transform` phase called 'MEHello' in me.
- 1 add two files: me_hello_opt.h and me_hello_opt.cpp

content in me_hello_opt.h
```c++
#ifndef MAPLE_ME_INCLUDE_ME_HELLO_OPT_H
#define MAPLE_ME_INCLUDE_ME_HELLO_OPT_H
#include "me_function.h"    // in order to use MeFunction
#include "maple_phase.h"    // in order to use the macro
namespace maple {

// always use this macro when you work with a transform phase.
MAPLE_FUNC_PHASE_DECLARE(MEHello, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_HELLO_OPT_H

```
content in me_hello_opt.cpp
```c++
#include "me_hello_opt.h"
// include the corresponding phasemanager in order to use information from other phase.
#include "me_phase_manager.h"

namespace maple {

// you need always keep in mind that which analysis results are needed and which results will be destroyed.
void MEHello::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>(); // it is guaranteed that MEDominance's result is available in this phase.
  aDep.SetPreservedAll();          // it means that this phase will not destroy any analysis results.
}

// the return value of this function indicates that whether this phase
// has modified the IR of this function, for now we do not use this value.
bool MEHello::PhaseRun(maple::MeFunction &f) {
  // you can use this macro to get the result which is configured as Required
  // in GetAnalysisDependence; or an error will be reported.
  auto *dom = GET_ANALYSIS(MEDominance);
  // do something using dom info.
  LogInfo::MapleLogger() << "hello opt on function: " << f.GetName() << '\n';
  return false;
}

}  // namespace maple

```
- 2 tell the phase manager to welcome a new phase.
    - 2.1 add the header file(me_hello_opt.h) to me_phase_manager.h
    - 2.2 register the phase in me_phase_manager.cpp, like this:
```c++
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEHdse, hdse)
// the macro suffix CANSKIP indicates that this phase can be skipped.
// first parameter tells the implement class and the second parameter is
// the phase name, which could be used in option like: dump-phase(s),
// skip-phases, skip-after; and it is used to configure the phase list that will run in phasemanager.
+MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEHello, hello)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELfoIVCanon, ivcanon)
```

- 3 add the new phase into run list.
modify the phase.def file, like this:
```c++
...
ADDMAPLEMEPHASE("dse", MeOption::optLevel >= 2)
ADDMAPLEMEPHASE("hello", MeOption::optLevel >= 2)
ADDMAPLEMEPHASE("analyzector", JAVALANG)
...
```
- 4 add the cpp file into corresponding build.gn file.
- 5 compile and test new phase.
maple --run=me:mpl2mpl:mplcg --option="--O2 :--O2 --quiet:--O2 --quiet" test.mpl
we can see that our new hello phase is performed after dse successfully.
```
>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Optimizing Function  < VEC_invariant_p_base_space id=5471 >---
---Preparing Function  < VEC_invariant_p_base_space > [1] ---
---Run Phase [ mecfgbuild ]---
---Run Phase [ cfgOpt ]---
---Run Phase [ loopcanon ]---
    ++ trigger phase [ dominance ]
    ++ trigger phase [ identloops ]
---Run Phase [ splitcriticaledge ]---
---Run Phase [ ssatab ]---
---Run Phase [ aliasclass ]---
---Run Phase [ ssa ]---
    ++ trigger phase [ dominance ]
---Run Phase [ dse ]---
    ++ trigger phase [ fsaa ]
---Run Phase [ hello ]---
hello opt on function: VEC_invariant_p_base_space
---Run Phase [ hprop ]---
    ++ trigger phase [ irmapbuild ]
---Run Phase [ valueRangePropagation ]---
    ++ trigger phase [ identloops ]
---Run Phase [ hdse ]---
---Run Phase [ epre ]---
    ++ trigger phase [ dominance ]
    ++ trigger phase [ identloops ]
  == epre invokes [ hdse ] ==
---Run Phase [ rename2preg ]---
---Run Phase [ lpre ]---
---Run Phase [ storepre ]---
---Run Phase [ copyprop ]---
---Run Phase [ hdse ]---
---Run Phase [ pregrename ]---
---Run Phase [ bblayout ]---
---Run Phase [ meemit ]---

```