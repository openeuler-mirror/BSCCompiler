//
// Created by wchenbt on 9/5/2021.
//

#include "ubsan_bounds.h"
#include "me_function.h"
#include "mir_builder.h"
#include "san_common.h"


namespace maple {

  static unsigned log2_32(uint32_t value) {
    int n = 32;
    unsigned y;

    y = value >>16; if (y != 0) { n = n -16; value = y; }
    y = value >> 8; if (y != 0) { n = n - 8; value = y; }
    y = value >> 4; if (y != 0) { n = n - 4; value = y; }
    y = value >> 2; if (y != 0) { n = n - 2; value = y; }
    y = value >> 1; if (y != 0) { n = n - 1; }
    return 32 - n;
  }

  static void getTypeKind(MIRType *mirType, uint16_t *typeKind, uint16_t *typeInfo) {

    *typeKind = 0xffff;
    *typeInfo = 0;

    PrimitiveType primType = PrimitiveType(mirType->GetPrimType());

    if (primType.IsInteger()) {
      *typeKind = 0;
      *typeInfo = (log2_32(mirType->GetSize() << 3) << 1) | \
                  (primType.IsUnsigned() ? 0 : 1);
    } else if (primType.IsFloat()) {
      *typeKind = 1;
      *typeInfo = mirType->GetSize();
    } else {
      // Not implemented
      mirType->Dump(0, false);
      LogInfo::MapleLogger() << "The above mirType has not been implemented yet!\n";
    }
  }

  ArrayInfo::ArrayInfo (StmtNode *usedStmt, MIRArrayType *arrayType, ArrayNode *arrayNode)
          : usedStmt(usedStmt), arrayType(arrayType) {
    for (size_t i = 1; i < arrayNode->NumOpnds(); i++) {
      this->offset.push_back(arrayNode->Opnd(i));
    }
    for (uint16 i = 0; i < arrayType->GetDim(); i++) {
      this->dimensions.push_back(arrayType->GetSizeArrayItem(i));
    }
    MIRType *type;
    MIRArrayType *element = arrayType;
    while (element) {
        elemType.push_back(element);
        type = element->GetElemType();
        element = dynamic_cast<MIRArrayType *>(type);
    }
    elemType.push_back(type);
  }

  size_t ArrayInfo::GetElementSize() {
    return this->elemType.back()->GetSize();
  }

  void ArrayInfo::SetNeededSize(size_t size) {
    this->neededSize = size;
  }

  std::string ArrayInfo::GetArrayTypeName(size_t dim) {
    std::string ret = std::string(GetPrimTypeName(elemType.back()->GetPrimType())) + " ";
    for (size_t i = dim; i < this->dimensions.size(); i++) {
        ret += "[" + std::to_string(this->dimensions.at(i)) + "]";
    }
    ret = '\'' + ret + "\'";
    return ret.c_str();
  }

  BoundCheck::BoundCheck(MeFunction *func) : func(func) {
      mirModule = &(func->GetMIRModule());
      mirBuilder = mirModule->GetMIRBuilder();
      initializeCallbacks();
  }

  std::vector<ArrayInfo> BoundCheck::getArrayInfo(StmtNode *stmtNode) {
    std::vector<ArrayInfo> toBeChecked;

    std::stack<BaseNode *> baseNodeStack;
    baseNodeStack.push(stmtNode);
    while (!baseNodeStack.empty()) {
      BaseNode *baseNode = baseNodeStack.top();
      baseNodeStack.pop();
      if (baseNode->GetOpCode() == OP_iassign) {
        IassignNode *iassign = dynamic_cast<IassignNode *>(baseNode);       // iassign
        if (iassign->Opnd(0)->GetOpCode() != OP_array) {
          continue;
        }
        ArrayNode *arrayNode = dynamic_cast<ArrayNode *>(iassign->Opnd(0));
        MIRArrayType *tmpArrayType = dynamic_cast<MIRArrayType *>(arrayNode->
                GetArrayType(GlobalTables::GetTypeTable()));
        ArrayInfo arrayInfo(stmtNode, tmpArrayType, arrayNode);

        MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign->GetTyIdx());
        MIRPtrType *pointerType = static_cast<MIRPtrType *>(mirType);
        MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
        arrayInfo.SetNeededSize(pointedTy->GetSize());

        AddrofNode *addrofNode = dynamic_cast<AddrofNode *>(arrayNode->GetBase());
        // CHECK_FATAL(addrofNode != nullptr, "The base of arrayNode is not of type AddrofNode");
        if (addrofNode == nullptr) {
          continue;
        }

        toBeChecked.push_back(arrayInfo);
      }
      if (baseNode->GetOpCode() == OP_iread) {
        IreadNode *iread = dynamic_cast<IreadNode *>(baseNode);    // iread
        if (iread->Opnd(0)->GetOpCode() != OP_array) {
          continue;
        }
        ArrayNode *arrayNode = dynamic_cast<ArrayNode *>(iread->Opnd(0));
        MIRArrayType *tmpArrayType = dynamic_cast<MIRArrayType *>(arrayNode->
                GetArrayType(GlobalTables::GetTypeTable()));
        ArrayInfo arrayInfo(stmtNode, tmpArrayType, arrayNode);

        MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
        MIRPtrType *pointerType = static_cast<MIRPtrType *>(mirType);
        MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
        arrayInfo.SetNeededSize(pointedTy->GetSize());

        AddrofNode *addrofNode = dynamic_cast<AddrofNode *>(arrayNode->GetBase());
        if (addrofNode == nullptr) {
          continue;
        }

        toBeChecked.push_back(arrayInfo);
      }
      for (size_t j = 0; j < baseNode->NumOpnds(); ++j) {
        baseNodeStack.push(baseNode->Opnd(j));
      }
    }
    return toBeChecked;
  }

  void BoundCheck::getBoundsCheckCond(ArrayInfo *arrayInfo, BlockNode *body, size_t dim) {

    LogInfo::MapleLogger() << "\nInstrument for " << arrayInfo->neededSize << " bytes\n";

    MIRType *uint64 = GlobalTables::GetTypeTable().GetUInt64();
    CHECK_FATAL(arrayInfo->offset.size() == arrayInfo->dimensions.size(),
        "The offset.size and array dimension.size do not match");
    MIRSymbol *offsetSym = getOrCreateSymbol(mirBuilder, uint64->GetTypeIndex(),
                                             "ubsan_offset", kStVar, kScAuto,
                                             mirBuilder->GetMirModule().CurFunction(), kScopeLocal);
    ConstvalNode *size = mirBuilder->CreateIntConst(arrayInfo->GetElementSize() * arrayInfo->dimensions[dim], PTY_u64);

    BaseNode *offset = mirBuilder->CreateExprBinary(OP_mul, *uint64, arrayInfo->offset[dim],
                                                    mirBuilder->CreateIntConst(arrayInfo->GetElementSize(), PTY_u64));
    DassignNode *dassignNode = mirBuilder->CreateStmtDassign(offsetSym->GetStIdx(), 0, offset);
    body->InsertBefore(arrayInfo->usedStmt, dassignNode);
    DreadNode *dreadNode = mirBuilder->CreateDread(*offsetSym, PTY_u64);
    BinaryNode *ObjSize = mirBuilder->CreateExprBinary(OP_sub, *uint64, size, dreadNode);
    // Offset >= 0
    CompareNode *Cmp1 = mirBuilder->CreateExprCompare(OP_lt, *uint64, *uint64, dreadNode,
                                                      mirBuilder->CreateIntConst(0, PTY_u64));
    // Size >= Offset
    CompareNode *Cmp2 = mirBuilder->CreateExprCompare(OP_lt, *uint64, *uint64, size, dreadNode);
    // Size - Offset >= NeededSize
    CompareNode *Cmp3 = mirBuilder->CreateExprCompare(OP_lt, *uint64, *uint64, ObjSize,
                                                      mirBuilder->CreateIntConst(arrayInfo->neededSize, PTY_u64));
    arrayInfo->checks.push_back({Cmp1, Cmp2, Cmp3});
  }


  void BoundCheck::insertBoundsCheck(ArrayInfo *arrayInfo, size_t dim) {
    StmtNode *insertBefore = arrayInfo->usedStmt;
    MIRType *uint64 = GlobalTables::GetTypeTable().GetUInt64();

    // first check : offset >= 0
    LabelIdx labelIdx = mirBuilder->GetMirModule().CurFunction()->GetLabelTab()->CreateLabel();
    mirBuilder->GetMirModule().CurFunction()->GetLabelTab()->AddToStringLabelMap(labelIdx);

    DassignNode *dassignNode = mirBuilder->CreateStmtDassign(*symbol_1, 0, arrayInfo->checks[dim][0]);
    dassignNode->InsertAfterThis(*insertBefore);
    CondGotoNode *brStmt = mirBuilder->CreateStmtCondGoto(arrayInfo->checks[dim][0], OP_brtrue, labelIdx);
    brStmt->InsertAfterThis(*insertBefore);
    dassignNode = mirBuilder->CreateStmtDassign(*symbol_1, 0, arrayInfo->checks[dim][1]);
    dassignNode->InsertBeforeThis(*brStmt);

    brStmt->SetOffset(labelIdx);
    LabelNode *labelStmt = mirBuilder->GetMirModule().CurFuncCodeMemPool()->New<LabelNode>();
    labelStmt->SetLabelIdx(labelIdx);
    labelStmt->InsertAfterThis(*insertBefore);

    CompareNode *cmpNode = mirBuilder->CreateExprCompare(OP_ne, *uint64, *uint64,
                                                         mirBuilder->CreateDread(*symbol_1, PTY_u64),
                                                         mirBuilder->CreateIntConst(0, PTY_u64));
    dassignNode = mirBuilder->CreateStmtDassign(*symbol_2, 0, cmpNode);
    dassignNode->InsertAfterThis(*insertBefore);

    // second check: size >= offset
    labelIdx = mirBuilder->GetMirModule().CurFunction()->GetLabelTab()->CreateLabel();
    mirBuilder->GetMirModule().CurFunction()->GetLabelTab()->AddToStringLabelMap(labelIdx);

    brStmt = mirBuilder->CreateStmtCondGoto(cmpNode, OP_brtrue, labelIdx);
    brStmt->InsertBeforeThis(*dassignNode);
    dassignNode = mirBuilder->CreateStmtDassign(*symbol_2, 0, arrayInfo->checks[dim][2]);
    dassignNode->InsertBeforeThis(*brStmt);

    brStmt->SetOffset(labelIdx);
    labelStmt = mirBuilder->GetMirModule().CurFuncCodeMemPool()->New<LabelNode>();
    labelStmt->SetLabelIdx(labelIdx);
    labelStmt->InsertAfterThis(*insertBefore);

    // third check: size - offset >= neededsize
    labelIdx = mirBuilder->GetMirModule().CurFunction()->GetLabelTab()->CreateLabel();
    mirBuilder->GetMirModule().CurFunction()->GetLabelTab()->AddToStringLabelMap(labelIdx);

    cmpNode = mirBuilder->CreateExprCompare(OP_ne, *uint64, *uint64,
                                            mirBuilder->CreateDread(*symbol_2, PTY_u64),
                                            mirBuilder->CreateIntConst(0, PTY_u64));
    brStmt = mirBuilder->CreateStmtCondGoto(cmpNode, OP_brfalse, labelIdx);
    brStmt->InsertAfterThis(*insertBefore);


    brStmt->SetOffset(labelIdx);
    labelStmt = mirBuilder->GetMirModule().CurFuncCodeMemPool()->New<LabelNode>();
    labelStmt->SetLabelIdx(labelIdx);
    labelStmt->InsertAfterThis(*insertBefore);

    // Initialize the field sourceLoc
    std::string srcFileName = "";
    if (!mirModule->GetSrcFileInfo().empty()) {
      size_t size = mirModule->GetSrcFileInfo().size();
      size_t i = 0;
      for (auto infoElem : mirModule->GetSrcFileInfo()) {
        srcFileName += GlobalTables::GetStrTable().GetStringFromStrIdx(infoElem.first);
        if (i++ < size - 1) {
          srcFileName += ",\n";
        }
      }
    }


    std::string::size_type iPos = srcFileName.find_last_of('/') + 1;
    UStrIdx moduleName = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(srcFileName.substr(iPos, srcFileName.length() - iPos));
    ConststrNode *constNode = mirModule->CurFuncCodeMemPool()->New<ConststrNode>(PTY_a64, moduleName);
    IassignNode *iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*sourceLocType),
                                                            1,mirBuilder->CreateAddrof(*sourceLoc), constNode);
    iassignNode->InsertAfterThis(*labelStmt);

    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*sourceLocType),
                                                            2, mirBuilder->CreateAddrof(*sourceLoc),
                                                            mirBuilder->GetConstUInt32(arrayInfo->usedStmt->GetSrcPos().LineNum()));
    iassignNode->InsertAfterThis(*labelStmt);

    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*sourceLocType),
                                                3, mirBuilder->CreateAddrof(*sourceLoc),
                                                mirBuilder->GetConstUInt32(arrayInfo->usedStmt->GetSrcPos().Column()));
    iassignNode->InsertAfterThis(*labelStmt);

    // Initialize the field arrayType
    uint16_t typeKind, typeInfo;
    if (dim < arrayInfo->elemType.size()) {
      getTypeKind(arrayInfo->elemType[dim], &typeKind, &typeInfo);
    } else {
      getTypeKind(arrayInfo->elemType.back(), &typeKind, &typeInfo);
    }
    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*typeDescriptor),
                                                             1, mirBuilder->CreateAddrof(*arrayType),
                                                             mirBuilder->GetConstUInt32(typeKind));
    iassignNode->InsertAfterThis(*labelStmt);

    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*typeDescriptor),
                                                2, mirBuilder->CreateAddrof(*arrayType),
                                                mirBuilder->GetConstUInt32(typeInfo));
    iassignNode->InsertAfterThis(*labelStmt);

    MapleVector<BaseNode*> arguments(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());


    UStrIdx typeName = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(arrayInfo->GetArrayTypeName(dim));
    int arraySize = strlen(arrayInfo->GetArrayTypeName(dim).c_str());
    constNode = mirModule->CurFuncCodeMemPool()->New<ConststrNode>(PTY_a64, typeName);
    arguments.push_back(mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetAddr64(),
                                                     mirBuilder->CreateAddrof(*arrayType, PTY_u64),
                                                     mirBuilder->CreateIntConst(4, PTY_a64)));
    arguments.push_back(constNode);
    arguments.push_back(mirBuilder->GetConstUInt32(arraySize + 1));
    IntrinsiccallNode *intrinsiccallNode = mirBuilder->CreateStmtIntrinsicCall(INTRN_C_memcpy, arguments);
    intrinsiccallNode->InsertAfterThis(*labelStmt);

    arguments.clear();
    arguments.push_back(mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetAddr64(),
                                                     mirBuilder->CreateAddrof(*arrayType, PTY_u64),
                                                     mirBuilder->CreateIntConst(5 + arraySize, PTY_a64)));
    arguments.push_back(mirBuilder->CreateIntConst(0, PTY_u32));
    arguments.push_back(mirBuilder->GetConstUInt32(99 - arraySize));
    intrinsiccallNode = mirBuilder->CreateStmtIntrinsicCall(INTRN_C_memset, arguments);
    intrinsiccallNode->InsertAfterThis(*labelStmt);


    // Initialize the field indexType
    getTypeKind(GlobalTables::GetTypeTable().GetPrimType(arrayInfo->offset[dim]->GetPrimType()),
                &typeKind, &typeInfo);

    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*typeDescriptor),
                                                1, mirBuilder->CreateAddrof(*indexType),
                                                mirBuilder->GetConstUInt32(typeKind));
    iassignNode->InsertAfterThis(*labelStmt);

    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*typeDescriptor),
                                                2, mirBuilder->CreateAddrof(*indexType),
                                                mirBuilder->GetConstUInt32(typeInfo));
    iassignNode->InsertAfterThis(*labelStmt);

    typeName = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(GetPrimTypeName(arrayInfo->offset[dim]->GetPrimType()));
    constNode = mirModule->CurFuncCodeMemPool()->New<ConststrNode>(PTY_a64, typeName);

    arraySize = strlen(GetPrimTypeName(arrayInfo->offset[dim]->GetPrimType()));
    arguments.clear();
    arguments.push_back(mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetAddr64(),
                                                     mirBuilder->CreateAddrof(*indexType, PTY_u64),
                                                     mirBuilder->CreateIntConst(4, PTY_a64)));
    arguments.push_back(constNode);
    arguments.push_back(mirBuilder->GetConstUInt32( arraySize + 1));
    intrinsiccallNode = mirBuilder->CreateStmtIntrinsicCall(INTRN_C_memcpy, arguments);
    intrinsiccallNode->InsertAfterThis(*labelStmt);

    arguments.clear();
    arguments.push_back(mirBuilder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetAddr64(),
                                                     mirBuilder->CreateAddrof(*indexType, PTY_u64),
                                                     mirBuilder->CreateIntConst(5 + arraySize, PTY_a64)));
    arguments.push_back(mirBuilder->CreateIntConst(0, PTY_u32));
    arguments.push_back(mirBuilder->GetConstUInt32(99 - arraySize));
    intrinsiccallNode = mirBuilder->CreateStmtIntrinsicCall(INTRN_C_memset, arguments);
    intrinsiccallNode->InsertAfterThis(*labelStmt);


    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*outofBoundsData),
                                                1, mirBuilder->CreateAddrof(*outofBound),
                                                mirBuilder->CreateDread(*sourceLoc, PTY_agg));
    iassignNode->InsertAfterThis(*labelStmt);

    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*outofBoundsData),
                                                5, mirBuilder->CreateAddrof(*outofBound),
                                                mirBuilder->CreateAddrof(*arrayType));
    iassignNode->InsertAfterThis(*labelStmt);

    iassignNode = mirBuilder->CreateStmtIassign(*GlobalTables::GetTypeTable().GetOrCreatePointerType(*outofBoundsData),
                                                6,mirBuilder->CreateAddrof(*outofBound),
                                                mirBuilder->CreateAddrof(*indexType));
    iassignNode->InsertAfterThis(*labelStmt);

    MapleVector<BaseNode*> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.emplace_back(mirBuilder->CreateAddrof(*outofBound, PTY_a64));
    args.emplace_back(arrayInfo->offset[dim]);
    CallNode* callNode = mirBuilder->CreateStmtCall(ubsanHandler->GetPuidx(), args);
    callNode->InsertAfterThis(*labelStmt);
  }

  void BoundCheck::initializeCallbacks() {

    FieldVector fieldVector;
    FieldVector parentFileds;

    MIRPtrType *int8Ptr = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetOrCreatePointerType(
            GlobalTables::GetTypeTable().GetInt8()->GetTypeIndex()));

    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "Filename", *int8Ptr);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "Line", *GlobalTables::GetTypeTable().GetUInt32());
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "Column", *GlobalTables::GetTypeTable().GetUInt32());
    sourceLocType = static_cast<MIRStructType *>(
            GlobalTables::GetTypeTable().GetOrCreateStructType(
                    "SourceLocation", fieldVector, parentFileds, mirBuilder->GetMirModule()));
    fieldVector.clear();

    MIRArrayType *charArray = static_cast<MIRArrayType*>(
            GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetInt8(), 100));
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "TypeKind", *GlobalTables::GetTypeTable().GetUInt16());
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "TypeInfo", *GlobalTables::GetTypeTable().GetUInt16());
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "TypeName", *charArray);
    typeDescriptor = static_cast<MIRStructType *>(
            GlobalTables::GetTypeTable().GetOrCreateStructType(
                    "TypeDescriptor", fieldVector, parentFileds, mirBuilder->GetMirModule()));
    fieldVector.clear();

    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "Loc", *sourceLocType);
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "ArrayType", *GlobalTables::GetTypeTable().GetOrCreatePointerType(*typeDescriptor));
    GlobalTables::GetTypeTable().PushIntoFieldVector(
            fieldVector, "IndexType", *GlobalTables::GetTypeTable().GetOrCreatePointerType(*typeDescriptor));

    outofBoundsData = static_cast<MIRStructType *>(
            GlobalTables::GetTypeTable().GetOrCreateStructType(
                    "OutOfBoundsData", fieldVector, parentFileds, mirBuilder->GetMirModule()));

    ubsanHandler = getOrInsertFunction(mirBuilder, "__ubsan_handle_out_of_bounds",
                                                          GlobalTables::GetTypeTable().GetVoid(),
                                                          {GlobalTables::GetTypeTable().GetInt8(),
                                                           GlobalTables::GetTypeTable().GetUInt64()});

    symbol_1 = getOrCreateSymbol(mirBuilder, GlobalTables::GetTypeTable().GetUInt64()->GetTypeIndex(),
                                 "ubsan_cmp_1", kStVar,kScAuto,
                                 mirBuilder->GetMirModule().CurFunction(), kScopeLocal);
    symbol_2 = getOrCreateSymbol(mirBuilder, GlobalTables::GetTypeTable().GetUInt64()->GetTypeIndex(),
                                 "ubsan_cmp_2", kStVar,kScAuto,
                                 mirBuilder->GetMirModule().CurFunction(), kScopeLocal);

    outofBound = getOrCreateSymbol(mirBuilder, outofBoundsData->GetTypeIndex(),
                                                 "ubsan_outOfBound", kStVar, kScAuto, func->GetMirFunc(), kScopeLocal);

    sourceLoc = getOrCreateSymbol(mirBuilder, typeDescriptor->GetTypeIndex(),
                                             "ubsan_sourceLoc", kStVar, kScAuto, func->GetMirFunc(), kScopeLocal);

    arrayType = getOrCreateSymbol(mirBuilder, typeDescriptor->GetTypeIndex(),
                                             "ubsan_arrayType", kStVar, kScAuto, func->GetMirFunc(), kScopeLocal);

    indexType = getOrCreateSymbol(mirBuilder, typeDescriptor->GetTypeIndex(),
                                             "ubsan_indexType", kStVar, kScAuto, func->GetMirFunc(), kScopeLocal);

  }

  bool BoundCheck::addBoundsChecking() {
    for (auto &stmt : func->GetMirFunc()->GetBody()->GetStmtNodes()) {
      std::vector<ArrayInfo> toBeChecked = getArrayInfo(&stmt);
      if (toBeChecked.empty()) {
        continue;
      }
      for (ArrayInfo arrayInfo: toBeChecked) {
        if (arrayInfo.offset.size()) {
          for (size_t i = 0; i < arrayInfo.offset.size(); i++) {
            getBoundsCheckCond(&arrayInfo, func->GetMirFunc()->GetBody(), i);
            insertBoundsCheck(&arrayInfo, i);
          }
        }
      }
    }
    return true;
  }
} // namespace maple