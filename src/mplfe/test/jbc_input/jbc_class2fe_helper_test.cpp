/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <climits>
#define protected public
#define private public
#include "jbc_class2fe_helper.h"
#undef protected
#undef private
#include "redirect_buffer.h"
#include "fe_options.h"

namespace maple {
class JBCClass2FEHelperTest : public testing::Test, public RedirectBuffer {
 public:
  JBCClass2FEHelperTest()
      : allocator(mp) {}
  ~JBCClass2FEHelperTest() = default;

  static void SetUpTestCase() {
    mp = FEUtils::NewMempool("MemPool for JBCClass2FEHelperTest", false /* isLcalPool */);
  }

  static void TearDownTestCase() {
    delete mp;
    mp = nullptr;
  }

 protected:
  static MemPool *mp;
  MapleAllocator allocator;
};
MemPool *JBCClass2FEHelperTest::mp = nullptr;

class JBCClassField2FEHelperTest : public testing::Test, public RedirectBuffer {
 public:
  JBCClassField2FEHelperTest()
      : allocator(mp) {}
  ~JBCClassField2FEHelperTest() = default;

  static void SetUpTestCase() {
    mp = FEUtils::NewMempool("MemPool for JBCClassField2FEHelperTest", false /* isLocalPool */);
  }

  static void TearDownTestCase() {
    delete mp;
    mp = nullptr;
  }

 protected:
  static MemPool *mp;
  MapleAllocator allocator;
};
MemPool *JBCClassField2FEHelperTest::mp = nullptr;

class MockJBCClass : public jbc::JBCClass {
 public:
  explicit MockJBCClass(MapleAllocator &allocator) : jbc::JBCClass(allocator) {}
  ~MockJBCClass() = default;
  MOCK_CONST_METHOD0(GetClassNameMpl, std::string());
  MOCK_CONST_METHOD0(GetClassNameOrin, std::string());
  MOCK_CONST_METHOD0(GetSuperClassName, std::string());
  MOCK_CONST_METHOD0(GetInterfaceNames, std::vector<std::string>());
};

class MockJBCClassField : public jbc::JBCClassField {
 public:
  MockJBCClassField(MapleAllocator &allocator, const jbc::JBCClass &klass)
      : jbc::JBCClassField(allocator, klass) {}
  ~MockJBCClassField() = default;
  MOCK_CONST_METHOD0(GetAccessFlag, uint16());
  MOCK_CONST_METHOD0(IsStatic, bool());
  MOCK_CONST_METHOD0(GetName, std::string());
  MOCK_CONST_METHOD0(GetDescription, std::string());
};

class MockJBCClass2FEHelper : public JBCClass2FEHelper {
 public:
  MockJBCClass2FEHelper(MapleAllocator &allocator, jbc::JBCClass &klassIn)
      : JBCClass2FEHelper(allocator, klassIn) {}
  ~MockJBCClass2FEHelper() = default;
  MOCK_CONST_METHOD0(IsStaticFieldProguard, bool());
};

TEST_F(JBCClass2FEHelperTest, PreProcessDecl_SameName) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("Ljava_2Flang_2FObject_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("Ljava/lang/Object;"));
  helper.PreProcessDecl();
  EXPECT_EQ(helper.IsSkipped(), true);
}

TEST_F(JBCClass2FEHelperTest, PreProcessDecl_NotSameName_Class) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewClass1InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewClass1InJBCClass2FEHelperTest;"));
  helper.PreProcessDecl();
  EXPECT_EQ(helper.IsSkipped(), false);
}

TEST_F(JBCClass2FEHelperTest, PreProcessDecl_NotSameName_Interface) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewInterface1InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewInterface1InJBCClass2FEHelperTest;"));
  klass.header.accessFlag = jbc::kAccClassInterface;
  helper.PreProcessDecl();
  EXPECT_EQ(helper.IsSkipped(), false);
}

TEST_F(JBCClass2FEHelperTest, ProcessDeclSuperClassForClass) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewClass2InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewClass2InJBCClass2FEHelperTest;"));
  ON_CALL(klass, GetSuperClassName())
      .WillByDefault(::testing::Return("Ljava/lang/Object;"));
  helper.PreProcessDecl();
  helper.ProcessDeclSuperClass();
  MIRStructType *structType = helper.GetContainer();
  EXPECT_NE(structType, nullptr);
  EXPECT_EQ(structType->GetKind(), kTypeClass);
  MIRClassType *classType = static_cast<MIRClassType*>(structType);
  MIRType *superType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(classType->GetParentTyIdx());
  EXPECT_EQ(superType->GetKind(), kTypeClass);
  EXPECT_EQ(superType->GetCompactMplTypeName(), "Ljava_2Flang_2FObject_3B");
}

TEST_F(JBCClass2FEHelperTest, ProcessDeclImplementsForClass) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewClass3InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewClass3InJBCClass2FEHelperTest;"));
  ON_CALL(klass, GetInterfaceNames())
      .WillByDefault(::testing::Return(std::vector<std::string>({ "LTestInterface1;", "LTestInterface2;" })));
  helper.PreProcessDecl();
  helper.ProcessDeclImplements();
  MIRStructType *structType = helper.GetContainer();
  EXPECT_NE(structType, nullptr);
  EXPECT_EQ(structType->GetKind(), kTypeClass);
  MIRClassType *classType = static_cast<MIRClassType*>(structType);
  const std::vector<TyIdx> &interfaces = classType->GetInterfaceImplemented();
  EXPECT_EQ(interfaces.size(), 2);
  MIRType *interfaceType0 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(interfaces[0]);
  EXPECT_EQ(interfaceType0->GetKind(), kTypeInterfaceIncomplete);
  EXPECT_EQ(interfaceType0->GetCompactMplTypeName(), "LTestInterface1_3B");
  MIRType *interfaceType1 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(interfaces[1]);
  EXPECT_EQ(interfaceType1->GetKind(), kTypeInterfaceIncomplete);
  EXPECT_EQ(interfaceType1->GetCompactMplTypeName(), "LTestInterface2_3B");
}

TEST_F(JBCClass2FEHelperTest, ProcessDeclSuperClassForInterface) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  klass.header.accessFlag = jbc::kAccClassInterface;
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewInterface2InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewInterface2InJBCClass2FEHelperTest;"));
  ON_CALL(klass, GetSuperClassName())
      .WillByDefault(::testing::Return("Ljava/io/Serializable;"));
  helper.PreProcessDecl();
  helper.ProcessDeclSuperClass();
  MIRStructType *structType = helper.GetContainer();
  EXPECT_NE(structType, nullptr);
  EXPECT_EQ(structType->GetKind(), kTypeInterface);
  MIRInterfaceType *interfaceType = static_cast<MIRInterfaceType*>(structType);
  const std::vector<TyIdx> &parents = interfaceType->GetParentsTyIdx();
  ASSERT_EQ(parents.size(), 1);
  MIRType *interfaceType0 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(parents[0]);
  EXPECT_EQ(interfaceType0->GetKind(), kTypeInterface);
  EXPECT_EQ(interfaceType0->GetCompactMplTypeName(), "Ljava_2Fio_2FSerializable_3B");
}

TEST_F(JBCClass2FEHelperTest, ProcessDeclImplementsForInterface) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  klass.header.accessFlag = jbc::kAccClassInterface;
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewInterface3InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewInterface3InJBCClass2FEHelperTest;"));
  ON_CALL(klass, GetInterfaceNames())
      .WillByDefault(::testing::Return(std::vector<std::string>({ "LTestInterface3;", "LTestInterface4;" })));
  helper.PreProcessDecl();
  helper.ProcessDeclImplements();
  MIRStructType *structType = helper.GetContainer();
  EXPECT_NE(structType, nullptr);
  EXPECT_EQ(structType->GetKind(), kTypeInterface);
  MIRInterfaceType *interfaceType = static_cast<MIRInterfaceType*>(structType);
  const std::vector<TyIdx> &parents = interfaceType->GetParentsTyIdx();
  EXPECT_EQ(parents.size(), 2);
  MIRType *interfaceType0 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(parents[0]);
  EXPECT_EQ(interfaceType0->GetKind(), kTypeInterfaceIncomplete);
  EXPECT_EQ(interfaceType0->GetCompactMplTypeName(), "LTestInterface3_3B");
  MIRType *interfaceType1 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(parents[1]);
  EXPECT_EQ(interfaceType1->GetKind(), kTypeInterfaceIncomplete);
  EXPECT_EQ(interfaceType1->GetCompactMplTypeName(), "LTestInterface4_3B");
}

TEST_F(JBCClass2FEHelperTest, CreateSymbol_Class) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewClass4InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewClass4InJBCClass2FEHelperTest;"));
  helper.PreProcessDecl();
  helper.CreateSymbol();
  EXPECT_NE(helper.mirSymbol, nullptr);
  RedirectCout();
  helper.mirSymbol->Dump(false, 0);
  EXPECT_EQ(GetBufferString(),
      "javaclass $LNewClass4InJBCClass2FEHelperTest_3B <$LNewClass4InJBCClass2FEHelperTest_3B>\n");
  RestoreCout();
}

TEST_F(JBCClass2FEHelperTest, CreateSymbol_Interface) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewInterface4InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewInterface4InJBCClass2FEHelperTest;"));
  klass.header.accessFlag = jbc::kAccClassInterface;
  helper.PreProcessDecl();
  helper.CreateSymbol();
  EXPECT_NE(helper.mirSymbol, nullptr);
  RedirectCout();
  helper.mirSymbol->Dump(false, 0);
  EXPECT_EQ(GetBufferString(),
      "javainterface $LNewInterface4InJBCClass2FEHelperTest_3B <$LNewInterface4InJBCClass2FEHelperTest_3B>\n");
  RestoreCout();
}

TEST_F(JBCClass2FEHelperTest, ProcessDecl_Class) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  klass.header.accessFlag = jbc::kAccClassAbstract;
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewClass5InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewClass5InJBCClass2FEHelperTest;"));
  ON_CALL(klass, GetSuperClassName())
      .WillByDefault(::testing::Return("Ljava/lang/Object;"));
  ON_CALL(klass, GetInterfaceNames())
      .WillByDefault(::testing::Return(std::vector<std::string>()));
  helper.PreProcessDecl();
  helper.ProcessDecl();
  EXPECT_NE(helper.mirSymbol, nullptr);
  RedirectCout();
  helper.mirSymbol->Dump(false, 0);
  EXPECT_EQ(GetBufferString(),
      "javaclass $LNewClass5InJBCClass2FEHelperTest_3B <$LNewClass5InJBCClass2FEHelperTest_3B> abstract\n");
  RestoreCout();
}

TEST_F(JBCClass2FEHelperTest, ProcessDecl_Interface) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  klass.header.accessFlag = jbc::kAccClassInterface | jbc::kAccClassPublic;
  JBCClass2FEHelper helper(allocator, klass);
  ON_CALL(klass, GetClassNameMpl())
      .WillByDefault(::testing::Return("LNewInterface5InJBCClass2FEHelperTest_3B"));
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LNewInterface5InJBCClass2FEHelperTest;"));
  ON_CALL(klass, GetSuperClassName())
      .WillByDefault(::testing::Return("Ljava/lang/Object;"));
  ON_CALL(klass, GetInterfaceNames())
      .WillByDefault(::testing::Return(std::vector<std::string>()));
  helper.PreProcessDecl();
  helper.ProcessDecl();
  EXPECT_NE(helper.mirSymbol, nullptr);
  RedirectCout();
  helper.mirSymbol->Dump(false, 0);
  EXPECT_EQ(GetBufferString(),
      "javainterface $LNewInterface5InJBCClass2FEHelperTest_3B <$LNewInterface5InJBCClass2FEHelperTest_3B> public\n");
  RestoreCout();
}

TEST_F(JBCClassField2FEHelperTest, ProcessDeclWithContainer_Instance) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LTestPack/TestClass;"));
  ::testing::NiceMock<MockJBCClassField> field(allocator, klass);
  ON_CALL(field, GetAccessFlag()).WillByDefault(::testing::Return(jbc::kAccFieldPublic));
  ON_CALL(field, IsStatic()).WillByDefault(::testing::Return(false));
  ON_CALL(field, GetName()).WillByDefault(::testing::Return("field"));
  ON_CALL(field, GetDescription()).WillByDefault(::testing::Return("Ljava/lang/Object;"));
  JBCClassField2FEHelper fieldHelper(allocator, field);
  ASSERT_EQ(fieldHelper.ProcessDeclWithContainer(allocator), true);
  // check field name
  std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fieldHelper.mirFieldPair.first);
  EXPECT_EQ(fieldName, "field");
  // check field type
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldHelper.mirFieldPair.second.first);
  EXPECT_EQ(type->GetKind(), kTypePointer);
  EXPECT_EQ(type->GetCompactMplTypeName(), "Ljava_2Flang_2FObject_3B");
  // check field attr
  EXPECT_EQ(fieldHelper.mirFieldPair.second.second.GetAttr(FLDATTR_public), true);
}

TEST_F(JBCClassField2FEHelperTest, ProcessDeclWithContainer_Static) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LTestPack/TestClass;"));
  ::testing::NiceMock<MockJBCClassField> field(allocator, klass);
  ON_CALL(field, GetAccessFlag()).WillByDefault(::testing::Return(jbc::kAccFieldProtected));
  ON_CALL(field, IsStatic()).WillByDefault(::testing::Return(true));
  ON_CALL(field, GetName()).WillByDefault(::testing::Return("field_static"));
  ON_CALL(field, GetDescription()).WillByDefault(::testing::Return("Ljava/lang/Object;"));
  JBCClassField2FEHelper fieldHelper(allocator, field);
  ASSERT_EQ(fieldHelper.ProcessDeclWithContainer(allocator), true);
  // check field name
  std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fieldHelper.mirFieldPair.first);
  EXPECT_EQ(fieldName, "LTestPack_2FTestClass_3B_7Cfield__static");
  // check field type
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldHelper.mirFieldPair.second.first);
  EXPECT_EQ(type->GetKind(), kTypePointer);
  EXPECT_EQ(type->GetCompactMplTypeName(), "Ljava_2Flang_2FObject_3B");
  // check field attr
  EXPECT_EQ(fieldHelper.mirFieldPair.second.second.GetAttr(FLDATTR_protected), true);
}

TEST_F(JBCClassField2FEHelperTest, ProcessDeclWithContainer_Static_AllType) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LTestPack/TestClass;"));
  ::testing::NiceMock<MockJBCClassField> field(allocator, klass);
  ON_CALL(field, GetAccessFlag()).WillByDefault(::testing::Return(jbc::kAccFieldPrivate));
  ON_CALL(field, IsStatic()).WillByDefault(::testing::Return(true));
  ON_CALL(field, GetName()).WillByDefault(::testing::Return("field_static"));
  ON_CALL(field, GetDescription()).WillByDefault(::testing::Return("Ljava/lang/Object;"));
  FEOptions::GetInstance().SetModeJavaStaticFieldName(FEOptions::kAllType);
  JBCClassField2FEHelper fieldHelper(allocator, field);
  ASSERT_EQ(fieldHelper.ProcessDeclWithContainer(allocator), true);
  // check field name
  std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fieldHelper.mirFieldPair.first);
  EXPECT_EQ(fieldName, "LTestPack_2FTestClass_3B_7Cfield__static_7CLjava_2Flang_2FObject_3B");
  // check field type
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldHelper.mirFieldPair.second.first);
  EXPECT_EQ(type->GetKind(), kTypePointer);
  EXPECT_EQ(type->GetCompactMplTypeName(), "Ljava_2Flang_2FObject_3B");
  // check field attr
  EXPECT_EQ(fieldHelper.mirFieldPair.second.second.GetAttr(FLDATTR_private), true);
}

TEST_F(JBCClassField2FEHelperTest, ProcessDeclWithContainer_Static_Smart) {
  ::testing::NiceMock<MockJBCClass> klass(allocator);
  ON_CALL(klass, GetClassNameOrin())
      .WillByDefault(::testing::Return("LTestPack/TestClass;"));
  ::testing::NiceMock<MockJBCClassField> field(allocator, klass);
  ON_CALL(field, GetAccessFlag()).WillByDefault(::testing::Return(jbc::kAccFieldPrivate));
  ON_CALL(field, IsStatic()).WillByDefault(::testing::Return(true));
  ON_CALL(field, GetName()).WillByDefault(::testing::Return("field_static"));
  ON_CALL(field, GetDescription()).WillByDefault(::testing::Return("Ljava/lang/Object;"));
  ::testing::NiceMock<MockJBCClass2FEHelper> klassHelper(allocator, klass);
  ON_CALL(klassHelper, IsStaticFieldProguard()).WillByDefault(::testing::Return(true));
  FEOptions::GetInstance().SetModeJavaStaticFieldName(FEOptions::kSmart);
  JBCClassField2FEHelper fieldHelper(allocator, field);
  ASSERT_EQ(fieldHelper.ProcessDeclWithContainer(allocator), true);
  // check field name
  std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fieldHelper.mirFieldPair.first);
  EXPECT_EQ(fieldName, "LTestPack_2FTestClass_3B_7Cfield__static_7CLjava_2Flang_2FObject_3B");
  // check field type
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldHelper.mirFieldPair.second.first);
  EXPECT_EQ(type->GetKind(), kTypePointer);
  EXPECT_EQ(type->GetCompactMplTypeName(), "Ljava_2Flang_2FObject_3B");
  // check field attr
  EXPECT_EQ(fieldHelper.mirFieldPair.second.second.GetAttr(FLDATTR_private), true);
  FEOptions::GetInstance().SetModeJavaStaticFieldName(FEOptions::kNoType);
}
}  // namespace maple
