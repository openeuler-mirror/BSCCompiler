//
// Created by wchenbt on 4/4/2021.
//
#include "asan_function.h"

#include <string_utils.h>

#include <stack>
#include <typeinfo>

#include "asan_interfaces.h"
#include "asan_stackvar.h"
#include "me_cfg.h"
#include "me_function.h"
#include "mir_builder.h"
#include "mpl_logging.h"
#include "opcode_info.h"
#include "san_common.h"

namespace maple {
bool isBlacklist(int k) {
  return (k == 120 || k == 125);
}

void doInstrumentAddress(AddressSanitizer *Phase, StmtNode *I, StmtNode *InsertBefore, BaseNode *Addr,
                         unsigned Alignment, unsigned Granularity, uint32_t TypeSize, bool IsWrite) {
  // Instrument a 1-, 2-, 4-, 8-, or 16- byte access with one check
  // if the data is properly aligned.
  if ((TypeSize == 8 || TypeSize == 16 || TypeSize == 32 || TypeSize == 64 || TypeSize == 128) &&
      (Alignment >= Granularity || Alignment == 0 || Alignment >= TypeSize / 8)) {
    return Phase->instrumentAddress(I, InsertBefore, Addr, TypeSize, IsWrite, nullptr);
  }
  Phase->instrumentUnusualSizeOrAlignment(I, InsertBefore, Addr, TypeSize, IsWrite);
}

void dumpFunc(MeFunction &func) {
  StmtNodes &stmtNodes = func.GetMirFunc()->GetBody()->GetStmtNodes();
  for (StmtNode &stmt : stmtNodes) {
    stmt.Dump(0);
  }
}

bool AddressSanitizer::instrumentFunction(MeFunction &func) {
  MIRBuilder *builder = func.GetMIRModule().GetMIRBuilder();
  this->func = &func;
  if (func.GetMirFunc()->GetAttr(FUNCATTR_extern)) {
    return false;
  }
  if (func.GetName().find("__asan_") == 0 || func.GetName().find("__san_cov_") == 0) {
    return false;
  }

  bool functionModified = false;

  LogInfo::MapleLogger() << "ASAN instrumenting: " << func.GetName() << "\n";

  initializeCallbacks(func.GetMIRModule());

  FunctionStateRAII cleanupObj(this);

  maybeInsertDynamicShadowAtFunctionEntry(func);

  std::vector<StmtNode *> toInstrument;
  std::vector<StmtNode *> noReturnCalls;

  // Definition for sanrazor

  // Destination map back to initiate stmt
  std::map<uint32, std::vector<StmtNode *>> brgoto_map;
  // Map the stmt by using label ID to the basic block ID, for verification in the coverage
  std::map<uint32, uint32> stmt_to_bbID;

  /*distinguish between user checks and sanitzer checks*/
  std::set<StmtNode *> userchecks;

  std::map<int, StmtNode *> stmt_id_to_stmt;
  std::vector<int> stmt_id_list;

  for (StmtNode &stmt : func.GetMirFunc()->GetBody()->GetStmtNodes()) {
    toInstrument.push_back(&stmt);
    if (CallNode *callNode = dynamic_cast<CallNode *>(&stmt)) {
      MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
      if (calleeFunc->NeverReturns() || calleeFunc->GetName() == "exit") {
        noReturnCalls.push_back(callNode);
      }
    }
    if (stmt.GetOpCode() == OP_brtrue || stmt.GetOpCode() == OP_brfalse) {
      userchecks.insert(&stmt);
    }
  }

  int numInstrumented = 0;

  for (auto stmt : toInstrument) {
    if (isInterestingMemoryAccess(stmt).size()) {
      instrumentMop(stmt);
    } else {
      instrumentMemIntrinsic(dynamic_cast<IntrinsiccallNode *>(stmt));
    }
    numInstrumented++;
  }

  FunctionStackPoisoner fsp(func, *this);
  bool changedStack = fsp.runOnFunction();

  for (auto stmt : noReturnCalls) {
    MapleVector<BaseNode *> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    CallNode *callNode = builder->CreateStmtCall(AsanHandleNoReturnFunc->GetPuidx(), args);
    callNode->InsertAfterThis(*stmt);
  }

  int check_env = SANRAZOR_MODE();
  if (numInstrumented > 0 || changedStack || !noReturnCalls.empty()) {
    functionModified = true;
    if (check_env > 0) {
      LogInfo::MapleLogger() << "****************SANRAZOR instrumenting****************"
                             << "\n";
      MIRType *voidType = GlobalTables::GetTypeTable().GetVoid();
      // type 0 is user check, type 1 is sanitzer check
      int type_of_check = 0;
      // if br_true set to 1, else set to 0
      int br_true = 0;
      // we check one step, than instrument_flag set to false
      bool instrument_flag = false;
      // info to plugin the shared lib
      int bb_id = 0;
      int stmt_id = 0;
      MeCFG *cfg = func.GetCfg();

      std::set<int32> reg_order;
      std::map<int32, std::vector<StmtNode *>> reg_to_stmt;

      std::set<uint32> var_order;
      std::map<uint32, std::vector<StmtNode *>> var_to_stmt;

      std::vector<set_check> san_set_check;
      std::vector<int> san_set_check_ID;

      std::vector<set_check> user_set_check;
      std::vector<int> user_set_check_ID;

      std::vector<StmtNode *> stmt_to_remove;
      std::vector<StmtNode *> call_stmt_to_remove;
      std::vector<std::pair<StmtNode *, BB *>> stmt_to_cleanup;

      for (BB *bb : cfg->GetAllBBs()) {
        if (bb) {
          for (StmtNode &stmt : bb->GetStmtNodes()) {
            std::vector<int32> stmt_reg;
            // OP_regassign -> <REG> = <EXPR>
            if (stmt.GetOpCode() == OP_regassign) {
              std::vector<int32> reg_redef_check_vec;
              set_check regassign_tmp;
              RegassignNode *regAssign = static_cast<RegassignNode *>(&stmt);
              // using PregIdx = int32;
              if (reg_to_stmt.count(regAssign->GetRegIdx()) == 0) {
                reg_order.insert(regAssign->GetRegIdx());
              }
              reg_to_stmt[regAssign->GetRegIdx()].push_back(&stmt);
            } else if (stmt.GetOpCode() == OP_dassign || stmt.GetOpCode() == OP_maydassign) {
              std::vector<uint32> var_redef_check_vec;
              set_check dassign_tmp;
              DassignNode *dassign = static_cast<DassignNode *>(&stmt);
              // uint32
              if (var_to_stmt.count(dassign->GetStIdx().Idx()) == 0) {
                var_order.insert(dassign->GetStIdx().Idx());
              }
              var_to_stmt[dassign->GetStIdx().Idx()].push_back(&stmt);
            }
            // Unsupported OPCODE:
            // 1. iassignoff <prim-type> <offset> (<addr-expr>, <rhs-expr>)
            // 2023-02-07: I added iassignoff as interestedMemoryAccess, the address is
            // calculated by `<addr-expr> + offset`. Hence, the instrumented code is
            // simply the same as iassign
            // 2. callassigned
            // 2023-02-07: I added callassigned to transform the returned variables' names
            // there are dassign OpCodes inside callassigned instruction
            else if (stmt.GetOpCode() == OP_callassigned) {
              /*
                    callassigned <func-name> (<opnd0>, ..., <opndn>) {
                    dassign <var-name0> <field-id0>
                    dassign <var-name1> <field-id1>
                    ...
                    dassign <var-namen> <field-idn> }
                */

            } else if (stmt.GetOpCode() == OP_iassign) {
              // syntax: iassign <type> <field-id> (<addr-expr>, <rhs-expr>)
              // %addr-expr = <rhs-expr>
              //IassignNode *iassign = static_cast<IassignNode*>(&stmt);
              BaseNode *addr_expr = stmt.Opnd(0);
              // addr_expr have 3 cases
              // iread u64 <* <$_TY_IDX111>> 22 (regread ptr %177)
              if (addr_expr->GetOpCode() == OP_iread) {
                std::vector<int32> dump_reg;
                recursion(addr_expr, dump_reg);
                for (int32 reg_tmp : dump_reg) {
                  if (reg_to_stmt.count(reg_tmp) == 0) {
                    reg_order.insert(reg_tmp);
                  }
                  reg_to_stmt[reg_tmp].push_back(&stmt);
                }
              } else if (addr_expr->GetOpCode() == OP_regread) {
                // regread ptr %14
                RegreadNode *regread = static_cast<RegreadNode *>(addr_expr);
                if (reg_to_stmt.count(regread->GetRegIdx()) == 0) {
                  reg_order.insert(regread->GetRegIdx());
                }
                reg_to_stmt[regread->GetRegIdx()].push_back(&stmt);
              } else if (addr_expr->GetOpCode() == OP_dread) {
                // dread i64 %asan_shadowBase
                DreadNode *dread = static_cast<DreadNode *>(addr_expr);
                if (var_to_stmt.count(dread->GetStIdx().Idx()) == 0) {
                  var_order.insert(dread->GetStIdx().Idx());
                }
                var_to_stmt[dread->GetStIdx().Idx()].push_back(&stmt);
              } else if (IsCommutative(addr_expr->GetOpCode())) {
                std::vector<int32> dump_reg;
                recursion(addr_expr->Opnd(0), dump_reg);
                for (int32 reg_tmp : dump_reg) {
                  if (reg_to_stmt.count(reg_tmp) == 0) {
                    reg_order.insert(reg_tmp);
                  }
                  reg_to_stmt[reg_tmp].push_back(&stmt);
                }
              }
            } else if (stmt.GetOpCode() == OP_brtrue || stmt.GetOpCode() == OP_brfalse) {
              set_check br_tmp;
              dep_expansion(stmt.Opnd(0), br_tmp, reg_to_stmt, var_to_stmt, func);
              gen_register_dep(&stmt, br_tmp, reg_to_stmt, var_to_stmt, func);

              CondGotoNode *cgotoNode = static_cast<CondGotoNode *>(&stmt);
              StmtNode *nextStmt = stmt.GetRealNext();
              instrument_flag = false;
              // if it is a user check
              if (userchecks.count(&stmt) > 0) {
                instrument_flag = true;
                user_set_check.push_back(br_tmp);
                user_set_check_ID.push_back(stmt.GetStmtID());
              } else if (nextStmt != nullptr) {
                if (CallNode *testcallNode = dynamic_cast<CallNode *>(nextStmt)) {
                  MIRFunction *testcalleeFunc =
                      GlobalTables::GetFunctionTable().GetFunctionFromPuidx(testcallNode->GetPUIdx());
                  // instrument if it is a call to sanitzer
                  if (testcalleeFunc->GetName().find("__asan_report_") == 0) {
                    san_set_check.push_back(br_tmp);
                    san_set_check_ID.push_back(stmt.GetStmtID());
                    instrument_flag = true;
                  }
                }
              }
              if (instrument_flag) {
                uint32 goto_id = cgotoNode->GetOffset();
                brgoto_map[goto_id].push_back(&stmt);
                uint32 lb_id = (static_cast<LabelNode *>(&stmt))->GetLabelIdx();
                // save the BB id for checking
                stmt_to_bbID[lb_id] = bb->UintID();
                stmt_id_to_stmt[stmt.GetStmtID()] = &stmt;
                stmt_id_list.push_back(stmt.GetStmtID());
              }
            }
          }
        }
      }
      if (brgoto_map.size() > 0) {
        // We loop again, if
        for (BB *bb : cfg->GetAllBBs()) {
          if (bb) {
            for (StmtNode &stmt : bb->GetStmtNodes()) {
              uint32 label_index = (static_cast<LabelNode *>(&stmt))->GetLabelIdx();
              auto iter = brgoto_map.find(label_index);
              // Some instruction with goto, will have the same LB id, as result
              // we may double count our coverage, so, we exclude op_goto
              if (iter != brgoto_map.end() && stmt.GetOpCode() != OP_goto) {
                std::vector<StmtNode *> tmp = brgoto_map[label_index];
                for (auto stmt_tmp : tmp) {
                  uint32 tmp_label_index = (static_cast<LabelNode *>(stmt_tmp))->GetLabelIdx();
                  auto id_check = stmt_to_bbID.find(tmp_label_index);
                  if (id_check == stmt_to_bbID.end()) {
                    //LogInfo::MapleLogger() << "[+] Can't fetch the ID "<<"\n";
                    bb_id = 0;
                  } else {
                    bb_id = stmt_to_bbID[tmp_label_index];
                  }
                  stmt_id = stmt_tmp->GetStmtID();
                  // We reverse the logic here
                  // Since brtrue, means jump if the check equal to true
                  // The instruction itself will need to be false in order for being executed
                  if (stmt_tmp->GetOpCode() == OP_brtrue) {
                    br_true = 0;
                  } else {
                    br_true = 1;
                  }
                  //record whether it is a usercheck or sancheck
                  auto search = userchecks.find(stmt_tmp);
                  if (search != userchecks.end()) {
                    type_of_check = 0;
                  } else {
                    type_of_check = 1;
                  }
                  CallNode *caller_cov = retCallCOV(func, bb_id, stmt_id, br_true, type_of_check);
                  CallNode *callee_cov = retCallCOV(func, bb_id, stmt_id, br_true ^ 1, type_of_check);
                  caller_cov->InsertBeforeThis(*stmt_tmp);
                  callee_cov->InsertBeforeThis(stmt);
                  stmt_to_cleanup.emplace_back(caller_cov, bb);
                  stmt_to_cleanup.emplace_back(callee_cov, bb);
                }
              }
            }
          }
        }
      }

      if (check_env == 2) {
        LogInfo::MapleLogger() << "Solving Sat"
                               << "\n";
        // If is eliminate mode
        std::string fn_UC = func.GetMIRModule().GetFileName() + "_UC";
        std::string fn_SC = func.GetMIRModule().GetFileName() + "_SC";
        std::map<int, san_struct> san_struct_UC = gen_dynmatch(fn_UC);
        std::map<int, san_struct> san_struct_SC = gen_dynmatch(fn_SC);
        std::map<int, std::set<int>> SC_SC_mapping;
        std::map<int, std::set<int>> UC_SC_mapping;

        for (auto const &[id_UC, val_UC] : san_struct_UC) {
          for (auto const &[id_SC, val_SC] : san_struct_SC) {
            // For SC-UC case, SC must be var a
            if (dynamic_sat(val_SC, val_UC, false)) {
              if (UC_SC_mapping.count(id_SC)) {
                UC_SC_mapping[id_SC].insert(id_UC);
              } else {
                std::set<int> tmp_set;
                tmp_set.insert(id_UC);
                UC_SC_mapping[id_SC] = tmp_set;
              }
              if (UC_SC_mapping.count(id_UC)) {
                UC_SC_mapping[id_UC].insert(id_SC);
              } else {
                std::set<int> tmp_set;
                tmp_set.insert(id_SC);
                UC_SC_mapping[id_UC] = tmp_set;
              }
            }
          }
        }

        for (auto const &[id_SC_1, val_SC_1] : san_struct_SC) {
          for (auto const &[id_SC_2, val_SC_2] : san_struct_SC) {
            if (id_SC_1 != id_SC_2) {
              if (dynamic_sat(val_SC_1, val_SC_2, false)) {
                if (SC_SC_mapping.count(id_SC_1)) {
                  SC_SC_mapping[id_SC_1].insert(id_SC_2);
                } else {
                  std::set<int> tmp_set;
                  tmp_set.insert(id_SC_2);
                  SC_SC_mapping[id_SC_1] = tmp_set;
                }
                if (SC_SC_mapping.count(id_SC_2)) {
                  SC_SC_mapping[id_SC_2].insert(id_SC_1);
                } else {
                  std::set<int> tmp_set;
                  tmp_set.insert(id_SC_1);
                  SC_SC_mapping[id_SC_2] = tmp_set;
                }
              }
            }
          }
        }
        ////san san deletion
        int SCSC_SAT_CNT = 0;
        int SCSC_SAT_RUNS = 0;
        for (int san_i = 0; san_i < san_set_check.size(); san_i++) {
          for (int san_j = san_i + 1; san_j < san_set_check.size(); san_j++) {
            SCSC_SAT_RUNS += 1;
            int san_i_stmt_ID = san_set_check_ID[san_i];
            int san_j_stmt_ID = san_set_check_ID[san_j];
            if (SC_SC_mapping.count(san_i_stmt_ID)) {
              if (SC_SC_mapping[san_i_stmt_ID].count(san_j_stmt_ID)) {
                if (sat_check(san_set_check[san_i], san_set_check[san_j])) {
                  SCSC_SAT_CNT += 1;
                  StmtNode *erase_stmt;
                  // we just assume the larger the stmtID
                  // the later the stmt appears, which mostly work
                  if (san_i_stmt_ID > san_j_stmt_ID) {
                    erase_stmt = stmt_id_to_stmt[san_i_stmt_ID];
                  } else {
                    erase_stmt = stmt_id_to_stmt[san_j_stmt_ID];
                  }
                  if (std::count(stmt_to_remove.begin(), stmt_to_remove.end(), erase_stmt) == 0) {
                    stmt_to_remove.push_back(erase_stmt);
                    //stmt_to_remove.push_back(erase_stmt->GetRealNext());
                    call_stmt_to_remove.push_back(erase_stmt->GetRealNext()->GetRealNext());
                  }
                }
              }
            }
          }
        }
        int UCSC_SAT_CNT = 0;
        int UCSC_SAT_RUNS = 0;
        for (int san_i = 0; san_i < san_set_check.size(); san_i++) {
          for (int user_j = 0; user_j < user_set_check.size(); user_j++) {
            UCSC_SAT_RUNS += 1;
            int san_i_stmt_ID = san_set_check_ID[san_i];
            int user_j_stmt_ID = user_set_check_ID[user_j];
            if (UC_SC_mapping.count(san_i_stmt_ID)) {
              if (UC_SC_mapping[san_i_stmt_ID].count(user_j_stmt_ID)) {
                print_dep(user_set_check[user_j]);
                print_dep(san_set_check[san_i]);
                bool goflag = false;
                if (sat_check(user_set_check[user_j], san_set_check[san_i])) {
                  goflag = true;
                } else {
                  san_set_check[san_i].opcode.erase(std::remove_if(san_set_check[san_i].opcode.begin(),
                                                                   san_set_check[san_i].opcode.end(), isBlacklist),
                                                    san_set_check[san_i].opcode.end());
                  if (sat_check(user_set_check[user_j], san_set_check[san_i])) {
                    goflag = true;
                  }
                }
                if (goflag) {
                  UCSC_SAT_CNT += 1;
                  StmtNode *erase_stmt = stmt_id_to_stmt[san_i_stmt_ID];
                  if (std::count(stmt_to_remove.begin(), stmt_to_remove.end(), erase_stmt) == 0) {
                    stmt_to_remove.push_back(erase_stmt);
                    //stmt_to_remove.push_back(erase_stmt->GetRealNext());
                    call_stmt_to_remove.push_back(erase_stmt->GetRealNext()->GetRealNext());
                  }
                }
              }
            }
          }
        }
        LogInfo::MapleLogger() << "UC size: " << user_set_check.size() << "\n ";
        LogInfo::MapleLogger() << "SC size: " << san_set_check.size() << "\n ";

        LogInfo::MapleLogger() << "Total UC-SC pairs: " << UCSC_SAT_RUNS << " Eliminate: " << UCSC_SAT_CNT << "\n ";
        LogInfo::MapleLogger() << "Total SC-SC pairs: " << SCSC_SAT_RUNS << " Eliminate: " << SCSC_SAT_CNT << "\n ";

        LogInfo::MapleLogger() << "Removing phase: \n";
        for (BB *bb : cfg->GetAllBBs()) {
          if (bb) {
            for (StmtNode &stmt : bb->GetStmtNodes()) {
              if (std::count(stmt_to_remove.begin(), stmt_to_remove.end(), &stmt)) {
                if (CallNode *testcallNode = dynamic_cast<CallNode *>(&stmt)) {
                  //bb->RemoveStmtNode(&stmt);
                  stmt_to_cleanup.emplace_back(&stmt, bb);
                } else {
                  set_check br_tmp;
                  dep_expansion(stmt.Opnd(0), br_tmp, reg_to_stmt, var_to_stmt, func);
                  std::set<uint32> tmp_var_set;
                  while (!br_tmp.var_live.empty()) {
                    uint32 var_to_check = br_tmp.var_live.top();
                    tmp_var_set.insert(var_to_check);
                    br_tmp.var_live.pop();
                  }
                  bool term_flag = false;
                  StmtNode *prevStmt = stmt.GetPrev();
                  while (!term_flag && prevStmt != nullptr) {
                    if (prevStmt->GetOpCode() == OP_brtrue || prevStmt->GetOpCode() == OP_brfalse) {
                      set_check br_local_tmp;
                      bool trigger = false;
                      dep_expansion(prevStmt->Opnd(0), br_local_tmp, reg_to_stmt, var_to_stmt, func);
                      while (!br_local_tmp.var_live.empty()) {
                        uint32 var_to_check = br_local_tmp.var_live.top();
                        if (func.GetMIRModule().CurFunction()->GetSymbolTabSize() >= int(var_to_check)) {
                          MIRSymbol *var = func.GetMIRModule().CurFunction()->GetSymbolTabItem(var_to_check);
                          if (var->GetName().find("asan_addr") == 0) {
                            trigger = true;
                            tmp_var_set.insert(var_to_check);
                          }
                        }
                        br_local_tmp.var_live.pop();
                      }
                      // we hit a possible UC, we terminate here
                      if (!trigger) {
                        term_flag = true;
                      } else {
                        prevStmt = prevStmt->GetPrev();
                        // bb->RemoveStmtNode(prevStmt->GetRealNext());
                        stmt_to_cleanup.emplace_back(prevStmt->GetRealNext(), bb);
                      }
                    } else if (prevStmt->GetOpCode() == OP_dassign) {
                      DassignNode *dassign = static_cast<DassignNode *>(prevStmt);
                      // dump extra dependence
                      set_check br_local_tmp;
                      dep_expansion(prevStmt, br_local_tmp, reg_to_stmt, var_to_stmt, func);
                      while (!br_local_tmp.var_live.empty()) {
                        uint32 var_to_check = br_local_tmp.var_live.top();
                        if (func.GetMIRModule().CurFunction()->GetSymbolTabSize() >= int(var_to_check)) {
                          MIRSymbol *var = func.GetMIRModule().CurFunction()->GetSymbolTabItem(var_to_check);
                          if (var->GetName().find("asan_addr") == 0) {
                            tmp_var_set.insert(var_to_check);
                          }
                        }
                        br_local_tmp.var_live.pop();
                      }
                      if (tmp_var_set.count(dassign->GetStIdx().Idx())) {
                        prevStmt = prevStmt->GetPrev();
                        stmt_to_cleanup.emplace_back(prevStmt->GetRealNext(), bb);
                        // bb->RemoveStmtNode(prevStmt->GetRealNext());
                      } else {
                        prevStmt = prevStmt->GetPrev();
                      }
                    } else if (CallNode *testcallNode = dynamic_cast<CallNode *>(prevStmt)) {
                      // stop if we hit a Call
                      term_flag = true;
                    } else {
                      prevStmt = prevStmt->GetPrev();
                    }
                  }
                  stmt_to_cleanup.emplace_back(&stmt, bb);
                  // bb->RemoveStmtNode(&stmt);
                }
              }
            }
          }
        }
        for (auto bb_pair : stmt_to_cleanup) {
          bb_pair.second->RemoveStmtNode(bb_pair.first);
        }
        int erase_ctr = 0;
        LogInfo::MapleLogger() << "Clean up redundant call stmt "
                               << "\n";
        BlockNode *bodyNode = func.GetMirFunc()->GetBody();
        for (auto stmt : call_stmt_to_remove) {
          // stmt->Dump();
          erase_ctr += 1;
          bodyNode->RemoveStmt(stmt);
        }
        LogInfo::MapleLogger() << "Erased: " << erase_ctr << "\n";
      }
      if ((func.GetName().compare("main") == 0) && (check_env == 1)) {
        //Register the call, such it dump the coverage at the exit
        __san_cov_flush = getOrInsertFunction(builder, "__san_cov_flush", voidType, {});
        //Insert the atexit to the starting point of the main
        MapleVector<BaseNode *> args(func.GetMIRModule().GetMPAllocator().Adapter());
        StmtNode *stmt_tmp = builder->CreateStmtCall(__san_cov_flush->GetPuidx(), args);
        func.GetMirFunc()->GetBody()->InsertFirst(stmt_tmp);
      }
      LogInfo::MapleLogger() << "****************SANRAZOR Done****************"
                             << "\n";
    }
  }
  // dump IRs of each block
  dumpFunc(func);
  LogInfo::MapleLogger() << "ASAN done instrumenting: " << functionModified << " " << func.GetName() << "\n";

  return functionModified;
}

void AddressSanitizer::instrumentMemIntrinsic(IntrinsiccallNode *stmtNode) {
  if (stmtNode == nullptr) {
    return;
  }

  switch (stmtNode->GetIntrinsic()) {
    case INTRN_C_memset: {
      MapleVector<BaseNode *> args(module->GetMPAllocator().Adapter());
      args.emplace_back(stmtNode->Opnd(0));
      args.emplace_back(stmtNode->Opnd(1));
      args.emplace_back(stmtNode->Opnd(2));

      CallNode *registerCallNode = module->GetMIRBuilder()->CreateStmtCall(AsanMemset->GetPuidx(), args);
      func->GetMirFunc()->GetBody()->ReplaceStmt1WithStmt2(stmtNode, registerCallNode);
      return;
    }
    case INTRN_C_memmove: {
      MapleVector<BaseNode *> args(module->GetMPAllocator().Adapter());
      args.emplace_back(stmtNode->Opnd(0));
      args.emplace_back(stmtNode->Opnd(1));
      args.emplace_back(stmtNode->Opnd(2));

      CallNode *registerCallNode = module->GetMIRBuilder()->CreateStmtCall(AsanMemmove->GetPuidx(), args);
      func->GetMirFunc()->GetBody()->ReplaceStmt1WithStmt2(stmtNode, registerCallNode);
      return;
    }
    case INTRN_C_memcpy: {
      MapleVector<BaseNode *> args(module->GetMPAllocator().Adapter());
      args.emplace_back(stmtNode->Opnd(0));
      args.emplace_back(stmtNode->Opnd(1));
      args.emplace_back(stmtNode->Opnd(2));

      CallNode *registerCallNode = module->GetMIRBuilder()->CreateStmtCall(AsanMemcpy->GetPuidx(), args);
      func->GetMirFunc()->GetBody()->ReplaceStmt1WithStmt2(stmtNode, registerCallNode);
      return;
    }
    default: {
      return;
    }
  }
}

std::vector<MemoryAccess> AddressSanitizer::isInterestingMemoryAccess(StmtNode *stmtNode) {
  LogInfo::MapleLogger() << "ASAN isInterestingMemoryAccess " << stmtNode->GetSrcPos().DumpLocWithColToString() << "\n";

  std::vector<MemoryAccess> memAccess;
  if (LocalDynamicShadow == stmtNode) {
    LogInfo::MapleLogger() << "ASAN isInterestingMemoryAccess is done.\n";
    return memAccess;
  }

  std::stack<BaseNode *> baseNodeStack;
  baseNodeStack.push(stmtNode);
  while (!baseNodeStack.empty()) {
    BaseNode *baseNode = baseNodeStack.top();
    CHECK_FATAL((baseNode != nullptr && baseNode != 0), "Invalid IR node pointer.");
    baseNodeStack.pop();
    switch (baseNode->GetOpCode()) {
      case OP_iassign: {
        IassignNode *iassign = dynamic_cast<IassignNode *>(baseNode);  // iassign
        CHECK_FATAL((iassign != nullptr), "Invalid IR node with OpCode OP_iassign");
        MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign->GetTyIdx());
        MIRPtrType *pointerType = static_cast<MIRPtrType *>(mirType);
        MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
        size_t align = pointedTy->GetAlign();
        if (pointedTy->IsStructType()) {
          MIRStructType *mirStructType = dynamic_cast<MIRStructType *>(pointedTy);
          if (iassign->GetFieldID() > 0) {
            pointedTy = mirStructType->GetFieldType(iassign->GetFieldID());
            align = pointedTy->GetAlign();
          } else {
            align = pointedTy->GetSize();
          }
        }
        BaseNode *addr = module->GetMIRBuilder()->CreateExprIaddrof(PTY_u64, iassign->GetTyIdx(), iassign->GetFieldID(),
                                                                    iassign->Opnd(0));
        struct MemoryAccess memoryAccess = {stmtNode, true, pointedTy->GetSize() << 3, align, addr};
        memAccess.emplace_back(memoryAccess);
        break;
      }
      case OP_iassignoff: {
        IassignoffNode *iassignoff = dynamic_cast<IassignoffNode *>(baseNode);
        CHECK_FATAL((iassignoff != nullptr), "Invalid IR node with OpCode OP_iassignoff");
        int32 offset = iassignoff->GetOffset();
        BaseNode *addrNode = iassignoff->GetBOpnd(0);
        PrimType primType = iassignoff->GetPrimType();
        PrimType addrPrimType = addrNode->GetPrimType();
        BaseNode *addrExpr = nullptr;
        size_t primTypeSize = GetPrimTypeSize(primType);
        if (offset == 0) {
          addrExpr = addrNode;
        } else {
          MIRType *mirType = module->CurFuncCodeMemPool()->New<MIRType>(MIRTypeKind::kTypePointer, addrPrimType);
          MIRIntConst *offsetConstVal = module->CurFuncCodeMemPool()->New<MIRIntConst>(offset, *mirType);
          ConstvalNode *offsetNode = module->CurFuncCodeMemPool()->New<ConstvalNode>(addrPrimType);
          offsetNode->SetConstVal(offsetConstVal);
          addrExpr = module->CurFuncCodeMemPool()->New<BinaryNode>(OP_add, addrPrimType, addrNode, offsetNode);
        }
        BaseNode *addr = module->GetMIRBuilder()->CreateExprIaddrof(addrPrimType, TyIdx(primType), 0, addrExpr);
        struct MemoryAccess memoryAccess = {stmtNode, true, primTypeSize << 3, primTypeSize, addr};
        memAccess.emplace_back(memoryAccess);
        break;
      }
      case OP_iassignfpoff:
      case OP_iassignpcoff:
        break;
      case OP_iread: {
        IreadNode *iread = nullptr;
        if (baseNode->IsSSANode()) {
          iread = dynamic_cast<IreadNode *>(dynamic_cast<IreadSSANode *>(baseNode)->GetNoSSANode());
        } else {
          iread = dynamic_cast<IreadNode *>(baseNode);
        }
        CHECK_FATAL((iread != nullptr), "Invalid IR node with OpCode OP_iread.");
        MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
        MIRPtrType *pointerType = static_cast<MIRPtrType *>(mirType);
        MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
        size_t align = pointedTy->GetAlign();
        if (pointedTy->IsStructType()) {
          MIRStructType *mirStructType = dynamic_cast<MIRStructType *>(pointedTy);
          if (iread->GetFieldID() > 0) {
            pointedTy = mirStructType->GetFieldType(iread->GetFieldID());
            align = pointedTy->GetAlign();
          } else {
            align = pointedTy->GetSize();
          }
        }
        BaseNode *addr =
            module->GetMIRBuilder()->CreateExprIaddrof(PTY_u64, iread->GetTyIdx(), iread->GetFieldID(), iread->Opnd(0));
        struct MemoryAccess memoryAccess = {stmtNode, false, pointedTy->GetSize() << 3, align, addr};
        memAccess.emplace_back(memoryAccess);
        break;
      }
      case OP_ireadoff:
      case OP_ireadfpoff:
      case OP_ireadpcoff:
        break;
      default: {
      }
    }
    for (int j = baseNode->NumOpnds() - 1; j >= 0; j--) {
      // StmtNode *tmpStmtNode = dynamic_cast<StmtNode *>(baseNode);
      // CHECK_FATAL(tmpStmtNode != nullptr, "Not a StmtNode");
      if (baseNode->GetOpCode() == OP_return) continue;
      baseNodeStack.push(baseNode->Opnd(j));
    }
  }
  LogInfo::MapleLogger() << "ASAN isInterestingMemoryAccess is done.\n";
  return memAccess;
}

void AddressSanitizer::initializeCallbacks(MIRModule &module) {
  MIRBuilder *mirBuilder = module.GetMIRBuilder();
  MIRType *voidType = GlobalTables::GetTypeTable().GetVoid();
  MIRType *Int32Type = GlobalTables::GetTypeTable().GetPrimType(PTY_i32);
  MIRType *Int8PtrType =
      GlobalTables::GetTypeTable().GetOrCreatePointerType(GlobalTables::GetTypeTable().GetInt8()->GetTypeIndex());

#ifdef ENABLERBTREE
  AsanRBTSafetyCheck = getOrInsertFunction(mirBuilder, "__asan_rbt_safety_check", voidType, {IntPtrTy, Int32Type});
  AsanRBTStackInsert = getOrInsertFunction(mirBuilder, "__asan_rbt_stack_insert", voidType, {IntPtrTy, Int32Type});
  AsanRBTStackDelete = getOrInsertFunction(mirBuilder, "__asan_rbt_stack_delete", voidType, {IntPtrTy, Int32Type});
#else
  for (size_t AccessIsWrite = 0; AccessIsWrite <= 1; AccessIsWrite++) {
    const std::string TypeStr = AccessIsWrite ? "store" : "load";
    AsanErrorCallbackSized[AccessIsWrite] = getOrInsertFunction(
        mirBuilder, (kAsanReportErrorTemplate + TypeStr + "_n").c_str(), voidType, {IntPtrTy, IntPtrTy});
    for (size_t AccessSizeIndex = 0; AccessSizeIndex < kNumberOfAccessSizes; AccessSizeIndex++) {
      const std::string Suffix = TypeStr + std::to_string(1ULL << AccessSizeIndex);
      AsanErrorCallback[AccessIsWrite][AccessSizeIndex] =
          getOrInsertFunction(mirBuilder, (kAsanReportErrorTemplate + Suffix).c_str(), voidType, {IntPtrTy});
    }
  }
#endif

  AsanMemmove = getOrInsertFunction(mirBuilder, "__asan_memmove", Int8PtrType, {Int8PtrType, Int8PtrType, IntPtrTy});
  AsanMemcpy = getOrInsertFunction(mirBuilder, "__asan_memcpy", Int8PtrType, {Int8PtrType, Int8PtrType, IntPtrTy});
  AsanMemset = getOrInsertFunction(mirBuilder, "__asan_memset", Int8PtrType, {Int8PtrType, Int32Type, IntPtrTy});

  AsanHandleNoReturnFunc = getOrInsertFunction(mirBuilder, kAsanHandleNoReturnName, voidType, {});
}

void AddressSanitizer::maybeInsertDynamicShadowAtFunctionEntry(MeFunction &F) {
  if (Mapping.Offset != kDynamicShadowSentinel) {
    return;
  }
  MIRBuilder *mirBuilder = F.GetMIRModule().GetMIRBuilder();
  MIRSymbol *GlobalDynamicAddress = mirBuilder->GetOrCreateGlobalDecl(kAsanShadowMemoryDynamicAddress, *IntPtrTy);
  DreadNode *dreadNode = mirBuilder->CreateDread(*GlobalDynamicAddress, PTY_ptr);
  MIRType *Int64PtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(IntPtrTy->GetTypeIndex());
  LocalDynamicShadow = mirBuilder->CreateExprIread(*IntPtrTy, *Int64PtrTy, 0, dreadNode);
}

void AddressSanitizer::instrumentMop(StmtNode *I) {
  std::vector<MemoryAccess> memoryAccess = isInterestingMemoryAccess(I);
  assert(memoryAccess.size() > 0);

  unsigned granularity = 1 << Mapping.Scale;
  for (MemoryAccess access : memoryAccess) {
    doInstrumentAddress(this, I, I, access.ptrOperand, access.alignment, granularity, access.typeSize, access.isWrite);
  }
}

BaseNode *AddressSanitizer::memToShadow(BaseNode *Shadow, MIRBuilder &mirBuilder) {
  Shadow = mirBuilder.CreateExprBinary(OP_ashr, *GlobalTables::GetTypeTable().GetInt64(), Shadow,
                                       mirBuilder.CreateIntConst(Mapping.Scale, IntPtrPrim));
  if (Mapping.Offset == 0) {
    return Shadow;
  }
  BaseNode *ShadowBase;
  if (LocalDynamicShadow) {
    ShadowBase = LocalDynamicShadow;
  } else {
    ShadowBase = mirBuilder.CreateIntConst(Mapping.Offset, IntPtrPrim);
  }
  if (Mapping.OrShadowOffset) {
    return mirBuilder.CreateExprBinary(OP_lior, *GlobalTables::GetTypeTable().GetInt64(), Shadow, ShadowBase);
  } else {
    return mirBuilder.CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetInt64(), Shadow, ShadowBase);
  }
}

void AddressSanitizer::instrumentAddress(StmtNode *OrigIns, StmtNode *InsertBefore, BaseNode *Addr, uint32_t TypeSize,
                                         bool IsWrite, BaseNode *SizeArgument) {
  MIRBuilder *mirBuilder = module->GetMIRBuilder();

#ifdef ENABLERBTREE
  auto i32PrimTy = GlobalTables::GetTypeTable().GetInt32()->GetPrimType();
  MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.emplace_back(Addr);
  args.emplace_back(mirBuilder->CreateIntConst(TypeSize / 8, i32PrimTy));
  func->GetMirFunc()->GetBody()->InsertBefore(InsertBefore,
                                              mirBuilder->CreateStmtCall(AsanRBTSafetyCheck->GetPuidx(), args));
  return;
#endif

  size_t accessSizeIndex = TypeSizeToSizeIndex(TypeSize);
  MIRSymbol *addrSymbol = getOrCreateSymbol(mirBuilder, IntPtrTy->GetTypeIndex(), "asan_addr", kStVar, kScAuto,
                                            module->CurFunction(), kScopeLocal);
  DassignNode *dassignNode = mirBuilder->CreateStmtDassign(addrSymbol->GetStIdx(), 0, Addr);

  func->GetMirFunc()->GetBody()->InsertBefore(InsertBefore, dassignNode);

  // Assign the address to %addr
  MIRType *shadowTy = GlobalTables::GetTypeTable().GetInt8();
  MIRPtrType *shadowPtrTy =
      dynamic_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetOrCreatePointerType(shadowTy->GetTypeIndex()));
  // Get the address of shadow value
  BaseNode *shadowPtr = memToShadow(mirBuilder->CreateDread(*addrSymbol, IntPtrPrim), *mirBuilder);
  BaseNode *cmpVal = mirBuilder->CreateIntConst(0, shadowTy->GetPrimType());
  // Get the value of shadow memory
  MIRSymbol *shadowValue = getOrCreateSymbol(mirBuilder, shadowTy->GetTypeIndex(), "asan_shadowValue", kStVar, kScAuto,
                                             module->CurFunction(), kScopeLocal);
  dassignNode = mirBuilder->CreateStmtDassign(shadowValue->GetStIdx(), 0,
                                              mirBuilder->CreateExprIread(*shadowTy, *shadowPtrTy, 0, shadowPtr));
  dassignNode->InsertAfterThis(*InsertBefore);
  // Check if value != 0
  BinaryNode *cmp = mirBuilder->CreateExprBinary(
      OP_ne, *shadowTy, mirBuilder->CreateDread(*shadowValue, shadowTy->GetPrimType()), cmpVal);
  size_t granularity = 1ULL << Mapping.Scale;

  StmtNode *crashBlock;
  if (TypeSize < 8 * granularity) {
    StmtNode *checkBlock = splitIfAndElseBlock(OP_brfalse, InsertBefore, cmp);
    BinaryNode *cmp2 = createSlowPathCmp(checkBlock, mirBuilder->CreateDread(*addrSymbol, PTY_i64),
                                         mirBuilder->CreateDread(*shadowValue, shadowTy->GetPrimType()), TypeSize);

    crashBlock = splitIfAndElseBlock(OP_brfalse, InsertBefore->GetPrev(), cmp2);

  } else {
    crashBlock = splitIfAndElseBlock(OP_brfalse, InsertBefore, cmp);
  }
  CallNode *crash = generateCrashCode(addrSymbol, IsWrite, accessSizeIndex, SizeArgument);
  crash->InsertBeforeThis(*crashBlock);
}

void AddressSanitizer::instrumentUnusualSizeOrAlignment(StmtNode *I, StmtNode *InsertBefore, BaseNode *Addr,
                                                        uint32_t TypeSize, bool IsWrite) {
  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  BaseNode *size = mirBuilder->CreateIntConst(TypeSize / 8, IntPtrPrim);
  MIRSymbol *addrSymbol = getOrCreateSymbol(mirBuilder, IntPtrTy->GetTypeIndex(), "asan_addr", kStVar, kScAuto,
                                            module->CurFunction(), kScopeLocal);
  DassignNode *dassignNode = mirBuilder->CreateStmtDassign(addrSymbol->GetStIdx(), 0, Addr);
  dassignNode->InsertAfterThis(*InsertBefore);
  BinaryNode *binaryNode =
      mirBuilder->CreateExprBinary(OP_add, *IntPtrTy, mirBuilder->CreateDread(*addrSymbol, IntPtrPrim),
                                   mirBuilder->CreateIntConst(TypeSize / 8 - 1, IntPtrPrim));
  MIRSymbol *lastByteSymbol = getOrCreateSymbol(mirBuilder, IntPtrTy->GetTypeIndex(), "asan_lastByte", kStVar, kScAuto,
                                                module->CurFunction(), kScopeLocal);
  DassignNode *lastByte = mirBuilder->CreateStmtDassign(lastByteSymbol->GetStIdx(), 0, binaryNode);
  lastByte->InsertAfterThis(*InsertBefore);
  instrumentAddress(I, InsertBefore, Addr, 8, IsWrite, size);
  instrumentAddress(I, InsertBefore, mirBuilder->CreateDread(*lastByteSymbol, PTY_ptr), 8, IsWrite, size);
}

BinaryNode *AddressSanitizer::createSlowPathCmp(StmtNode *InsBefore, BaseNode *AddrLong, BaseNode *ShadowValue,
                                                uint32_t TypeSize) {
  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  size_t granularity = static_cast<size_t>(1) << Mapping.Scale;
  // Addr & (Granularity - 1)
  BinaryNode *lastAccessedByte = mirBuilder->CreateExprBinary(OP_band, *IntPtrTy, AddrLong,
                                                              mirBuilder->CreateIntConst(granularity - 1, IntPtrPrim));
  // (Addr & (Granularity - 1)) + size - 1
  if (TypeSize / 8 > 1) {
    lastAccessedByte = mirBuilder->CreateExprBinary(OP_add, *IntPtrTy, lastAccessedByte,
                                                    mirBuilder->CreateIntConst(TypeSize / 8 - 1, IntPtrPrim));
  }
  // (uint8_t) ((Addr & (Granularity-1)) + size - 1)
  MIRSymbol *length = getOrCreateSymbol(mirBuilder, (TyIdx)ShadowValue->GetPrimType(), "asan_length", kStVar, kScAuto,
                                        module->CurFunction(), kScopeLocal);
  DassignNode *dassignNode = mirBuilder->CreateStmtDassign(length->GetStIdx(), 0, lastAccessedByte);
  dassignNode->InsertBeforeThis(*InsBefore);
  // ((uint8_t) ((Addr & (Granularity-1)) + size - 1)) >= ShadowValue
  return mirBuilder->CreateExprBinary(OP_ge, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(ShadowValue->GetPrimType()),
                                      mirBuilder->CreateDread(*length, ShadowValue->GetPrimType()), ShadowValue);
}

CallNode *AddressSanitizer::generateCrashCode(MIRSymbol *Addr, bool IsWrite, size_t AccessSizeIndex,
                                              BaseNode *SizeArgument) {
  CallNode *callNode = nullptr;
  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.emplace_back(mirBuilder->CreateDread(*Addr, IntPtrPrim));
  if (SizeArgument) {
    args.emplace_back(SizeArgument);
    callNode = mirBuilder->CreateStmtCall(AsanErrorCallbackSized[IsWrite]->GetPuidx(), args);
  } else {
    callNode = mirBuilder->CreateStmtCall(AsanErrorCallback[IsWrite][AccessSizeIndex]->GetPuidx(), args);
  }
  return callNode;
}

StmtNode *AddressSanitizer::splitIfAndElseBlock(Opcode op, StmtNode *elsePart, const BinaryNode *cmpStmt) {
  MIRBuilder *mirBuilder = module->GetMIRBuilder();
  auto *cmpNode = mirBuilder->CreateExprCompare(
      cmpStmt->GetOpCode(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(cmpStmt->GetPrimType())),
      *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(cmpStmt->GetPrimType())), cmpStmt->Opnd(0),
      cmpStmt->Opnd(1));
  LabelIdx labelIdx = module->CurFunction()->GetLabelTab()->CreateLabel();
  module->CurFunction()->GetLabelTab()->AddToStringLabelMap(labelIdx);
  CondGotoNode *brStmt = mirBuilder->CreateStmtCondGoto(cmpNode, op, labelIdx);
  brStmt->InsertAfterThis(*elsePart);

  brStmt->SetOffset(labelIdx);
  LabelNode *labelStmt = module->CurFuncCodeMemPool()->New<LabelNode>();
  labelStmt->SetLabelIdx(labelIdx);
  labelStmt->InsertAfterThis(*elsePart);
  return brStmt;
}

bool AddressSanitizer::isInterestingSymbol(MIRSymbol &symbol) {
  if (StringUtils::StartsWith(symbol.GetName(), "asan_")) {
    return false;
  }
  if (StringUtils::StartsWith(symbol.GetName(), "_temp_.shortcircuit.")) {
    return false;
  }
  if (ProcessedSymbols.find(&symbol) != ProcessedSymbols.end()) {
    return ProcessedSymbols[&symbol];
  }
  if (std::find(preAnalysis->usedInAddrof.begin(), preAnalysis->usedInAddrof.end(), &symbol) !=
      preAnalysis->usedInAddrof.end()) {
    return true;
  }
  bool isInteresting = true;

  MIRType *mirType = symbol.GetType();
  isInteresting &= isTypeSized(mirType);
  isInteresting &= mirType->GetSize() > 0;
  isInteresting &= !symbol.IsConst();

  if (mirType->GetKind() == kTypeScalar || mirType->GetKind() == kTypePointer || mirType->GetKind() == kTypeBitField) {
    isInteresting &= false;
  }

  ProcessedSymbols[&symbol] = isInteresting;
  return isInteresting;
}

bool AddressSanitizer::isInterestingAlloca(UnaryNode &unaryNode) {
  if (ProcessedAllocas.find(&unaryNode) != ProcessedAllocas.end()) {
    return ProcessedAllocas[&unaryNode];
  }
  bool isInteresting = true;

  ConstvalNode *constvalNode = dynamic_cast<ConstvalNode *>(unaryNode.Opnd(0));
  if (constvalNode) {
    MIRIntConst *mirConst = dynamic_cast<MIRIntConst *>(constvalNode->GetConstVal());
    isInteresting = mirConst->GetValue().GetExtValue() > 0;
  }
  ProcessedAllocas[&unaryNode] = isInteresting;
  return isInteresting;
}

}  // end namespace maple