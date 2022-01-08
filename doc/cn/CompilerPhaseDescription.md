### 方舟编译器phase层次结构

目前phase主要有两类：module phase和function phase。在其他层次的IR上的phase可以通过继承自MaplePhase完成。MaplePhase在特定的IR层次上完成优化或者进行程序分析。

##### MapleModulePhase

如果一个phase继承自MapleModulePhase，说明这个phase需要在整个module上进行转换；module phase也可以调用更低层次的MaplePhase。

##### MapleFunctionPhase

```c++
 template <class funcT>
 class MapleFunctionPhase : public MaplePhase
```

不同于MapleModulePhase，MapleFunctionPhase是一个模板类，主要是因为Maple中有不同层次的函数级IR。中端的优化phase和后端的优化phase都是该类的派生类。

### phase的内存管理

方舟编译器的phasemanager可以对内存进行有效的管理，以便每个phase可以保留可能被其他phase依赖的分析结果；以及丢弃失效的结果。每个phasemanager中提供一个AnalysisDataManager类（多线程时，每个线程对应一个AnalysisDataManager）用来存储分析结果。为了实现这个功能，每个phase需要实现GetAnalysisDependence函数。如下：

```c++
 void <OPT_PHASENAME>::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<OPT_PHASENAME>();  // 配置当前phase需要依赖的phase。
  aDep.AddPreserved<OPT_PHASENAME>(); // 配置当前phase执行完成后，需要保留的分析结果。
  aDep.SetPreservedAll();             // 保留所有的分析结果。
}
```

#### 分析phase

分析phase中的GetPhasesMempool函数返回的mempool来自于AnalysisDataManager。分析phase的内存池中分配的对象，只有在被其他phase设置为需要丢弃；或者当前phasemanager结束时，才会被释放。否则会一直存在于当前phasemanager中。如果有些信息仅仅是在分析phase内部局部使用，那么可以将其分配在临时内存池中，通过ApplyTempMemPool函数获取一个临时的内存池。

#### 转化phase

对于转化phase，GetPhasesMempool和ApplyTempMemPool获取的内存池类似，都会在该phase结束后进行释放。**如果一个转化phase不实现GetAnalysisDependence，则意味着phase结束后会把当前phase所在的phasemanager中的所有现存分析phase的结果删掉**。



**If a transform phase does not implement the GetAnalysisDependence method, it defaults to not having any prerequisite phases, and invalidating all phases information in analysisDataManager.** Transfrom phase default to be assumed to invalid all ananlysis results.

### 快速开始

假设我们要在中端实现一个叫做“MEHello”的`转化`phase。

- 1 新增两个文件

me_hello_opt.h：

```c++
#ifndef MAPLE_ME_INCLUDE_ME_HELLO_OPT_H
#define MAPLE_ME_INCLUDE_ME_HELLO_OPT_H
#include "me_function.h"    // 使用MeFunction
#include "maple_phase.h"    // 使用MAPLE_FUNC_PHASE_DECLARE宏
namespace maple {

// 对于转化phase，尽量使用这个宏。
MAPLE_FUNC_PHASE_DECLARE(MEHello, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_HELLO_OPT_H

```

me_hello_opt.cpp：

```c++
#include "me_hello_opt.h"
// 将该phase对应的phasemanager头文件引入进来，可以方便地使用其他phase的分析结果。
#include "me_phase_manager.h"

namespace maple {

// 您需要清楚地知道，当前的phase依赖什么分析结果，以及会破坏什么分析结果。
void MEHello::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>(); // phasemanager会保证MEDominace这个phase的分析结果在当前phase中是可用的。
  aDep.SetPreservedAll();          // 表明了当前phase不会破坏任何分析结果。
}

// 这个函数的返回值表明了当前phase是否修改了IR，目前该返回值并未使用。
bool MEHello::PhaseRun(maple::MeFunction &f) {
  // 您可以通过这个宏来获取想要的分析结果，前提是在GetAnalysisDependence配置过，否则会报错。
  auto *dom = GET_ANALYSIS(MEDominance);
  // 使用dom信息，做一些事情。。。
  LogInfo::MapleLogger() << "hello opt on function: " << f.GetName() << '\n';
  return false;
}

}  // namespace maple

```

- 2 告知phasemanager，有新的phase加入了。
  - 2.1 将新phase的头文件(me_hello_opt.h) 添加至me_phase_manager.h
  - 2.2 在me_phase_manager.cpp注册新的phase，如下:

```c++
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEHdse, hdse)
// 后缀CANSKIP的宏表明了这个phase可以跳过。第一个参数是phase的实现类，第二个参数是phase的名字。
// phase的名字可以用在多个选项中，如skip-phases，dump-phase(s)，skip-after等等；同时配置哪些phase需要运行，也是用这个名字。
+MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MEHello, hello)
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(MELfoIVCanon, ivcanon)
```

- 3 配置需要运行的phase列表。
  x修改phase.def文件，如下：

```c++
...
ADDMAPLEMEPHASE("dse", MeOption::optLevel >= 2)
ADDMAPLEMEPHASE("hello", MeOption::optLevel >= 2)
ADDMAPLEMEPHASE("analyzector", JAVALANG)
...
```

- 4 把cpp文件加入到对应的build.gn中。
- 5 编译工具链测试
  maple --run=me:mpl2mpl:mplcg --option="--O2 :--O2 --quiet:--O2 --quiet" test.mpl
  可以看到，我们新增的phase在dse之后正确地输出了预期的内容！

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
