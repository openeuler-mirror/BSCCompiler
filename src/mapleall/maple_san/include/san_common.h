#ifndef MAPLE_SAN_INCLUDE_SAN_COMMON_H
#define MAPLE_SAN_INCLUDE_SAN_COMMON_H
#include "me_function.h"
#include "me_phase_manager.h"
#include "mir_builder.h"
#include "mir_function.h"
#include "mir_module.h"
#include "mir_nodes.h"
#include "types_def.h"

namespace maple {

#if TARGX86_64 || TARGAARCH64
#define LOWERED_PTR_TYPE PTY_a64
constexpr uint8 kSizeOfPtr = 8;
#elif TARGX86 || TARGARM32 || TARGVM
#define LOWERED_PTR_TYPE PTY_a32
constexpr uint8 kSizeOfPtr = 4;
#else
#error "Unsupported target"
#endif

struct ASanStackVariableDescription {
  std::string Name;     // Name of the variable that will be displayed by asan
  size_t Size;          // Size of the variable in bytes.
  size_t LifetimeSize;  // Size in bytes to use for lifetime analysis check.
  size_t Alignment;     // Alignment of the variable (power of 2).
  MIRSymbol *Symbol;    // The actual AllocaInst.
  StmtNode *AllocaInst;
  size_t Offset;  // Offset from the beginning of the frame;
  unsigned Line;  // Line number.

  bool operator<(const ASanStackVariableDescription &rhs) const {
    return this->Line > rhs.Line;
  }
};

struct ASanDynaVariableDescription {
  std::string Name;  // Name of the variable that will be displayed by asan
  BaseNode *Size;    // Size of the variable in bytes.
  StmtNode *AllocaInst;
  size_t Offset;  // Offset from the beginning of the frame;
  unsigned Line;  // Line number.

  bool operator<(const ASanDynaVariableDescription &rhs) const {
    return this->Line > rhs.Line;
  }
};

// Output data struct for ComputeASanStackFrameLayout.
struct ASanStackFrameLayout {
  size_t Granularity;     // Shadow granularity.
  size_t FrameAlignment;  // Alignment for the entire frame.
  size_t FrameSize;       // Size of the frame in bytes.
};

// Struct to contain the information for performing set check
// For elimination of san check
struct set_check {
  std::vector<uint8_t> opcode;             // 1. opcode  enum Opcode : uint8
  std::vector<int32_t> register_terminal;  // 2. register_terminal -> cannot further expand
  std::stack<int32_t> register_live;       // 3. register_live  -> for further expansion
  std::vector<uint32_t> var_terminal;      // 4. var_terminal -> cannot further expand
  std::stack<uint32_t> var_live;           // 5. var_live  -> for further expansion
  std::vector<IntVal> const_int64;       // 6. const, we only track int64 or kConstInt
  std::vector<uint32_t> const_str;         // 7. const str ,we only store the index
  std::vector<int32_t> type_num;  // 8. Type, but we will just track the fieldID for simplicity  iread->GetFieldID()
};

struct san_struct {
  int stmtID;
  int tot_ctr;
  int l_ctr;
  int r_ctr;
};

class PreAnalysis : public AnalysisResult {
 public:
  PreAnalysis(MemPool &memPoolParam) : AnalysisResult(&memPoolParam){};

  ~PreAnalysis() override = default;

  std::vector<MIRSymbol *> usedInAddrof;
};

static const size_t kMinAlignment = 16;
static const unsigned kAllocaRzSize = 32;
static const size_t kNumberOfAccessSizes = 5;

static const uintptr_t kCurrentStackFrameMagic = 0x41B58AB3;
static const uintptr_t kRetiredStackFrameMagic = 0x45E0360E;
static const size_t kMinStackMallocSize = 1 << 6;   // 64B
static const size_t kMaxStackMallocSize = 1 << 16;  // 64K
static const int kMaxAsanStackMallocSizeClass = 10;

// These magic constants should be the same as in
// in asan_internal.h from ASan runtime in compiler-rt.
static const int kAsanStackLeftRedzoneMagic = 0xf1;
static const int kAsanStackMidRedzoneMagic = 0xf2;
static const int kAsanStackRightRedzoneMagic = 0xf3;
static const int kAsanStackUseAfterScopeMagic = 0xf8;

bool isTypeSized(MIRType *type);

int computeRedZoneField(MIRType *type);

size_t TypeSizeToSizeIndex(uint32_t TypeSize);

std::vector<MIRSymbol *> GetGlobalVaribles(const MIRModule &mirModule);

void appendToGlobalCtors(const MIRModule &mirModule, const MIRFunction *func);

void appendToGlobalDtors(const MIRModule &mirModule, const MIRFunction *func);

MIRFunction *getOrInsertFunction(MIRBuilder *mirBuilder, const char *name, MIRType *retType,
                                 std::vector<MIRType *> argTypes);

std::vector<uint8_t> GetShadowBytes(const std::vector<ASanStackVariableDescription> &Vars,
                                    const ASanStackFrameLayout &Layout);

MIRAddrofConst *createSourceLocConst(MIRModule &mirModule, MIRSymbol *Var, PrimType primType);

MIRAddrofConst *createAddrofConst(const MIRModule &mirModule, const MIRSymbol *mirSymbol, PrimType primType);

MIRStrConst *createStringConst(const MIRModule &mirModule, const std::basic_string<char>& Str, PrimType primType);

std::string ComputeASanStackFrameDescription(const std::vector<ASanStackVariableDescription> &vars);

std::vector<uint8_t> GetShadowBytesAfterScope(const std::vector<ASanStackVariableDescription> &Vars,
                                              const ASanStackFrameLayout &Layout);

MIRSymbol *getOrCreateSymbol(MIRBuilder *mirBuilder, const TyIdx tyIdx, const std::string &name, MIRSymKind mClass,
                             MIRStorageClass sClass, MIRFunction *func, uint8 scpID);

ASanStackFrameLayout ComputeASanStackFrameLayout(std::vector<ASanStackVariableDescription> &Vars, size_t Granularity,
                                                 size_t MinHeaderSize);
// Start of Sanrazor
int SANRAZOR_MODE();
CallNode *retCallCOV(const MeFunction &func, int bb_id, int stmt_id, int br_true, int type_of_check);
void recursion(BaseNode *stmt, std::vector<int32> &stmt_reg);
bool isReg_redefined(BaseNode *stmt, std::vector<PregIdx> &stmt_reg);
bool isVar_redefined(BaseNode *stmt, std::vector<uint32> &stmt_reg);
void dep_expansion(BaseNode *stmt, set_check &dep, std::map<int32, std::vector<StmtNode *>> reg_to_stmt,
                   std::map<uint32, std::vector<StmtNode *>> var_to_stmt, const MeFunction &func);
void print_dep(set_check dep);
template <typename T>
void print_stack(std::stack<T> &st);
template <typename T>
bool compareVectors(const std::vector<T>& a, const std::vector<T>& b);
int getIndex(std::vector<StmtNode *> v, StmtNode *K);
StmtNode *retLatest_Regassignment(StmtNode *stmt, int32 register_number);
StmtNode *retLatest_Varassignment(StmtNode *stmt, uint32 var_number);
set_check commit(set_check old, set_check latest);
void gen_register_dep(StmtNode *stmt, set_check &br_tmp, std::map<int32, std::vector<StmtNode *>> reg_to_stmt,
                      std::map<uint32, std::vector<StmtNode *>> var_to_stmt, const MeFunction& func);
bool sat_check(const set_check& a, const set_check& b);
std::map<int, san_struct> gen_dynmatch(std::string file_name);
bool dynamic_sat(const san_struct& a, const san_struct& b, bool SCSC);

}  // end namespace maple
#endif  // MAPLE_SAN_INCLUDE_SAN_COMMON_H