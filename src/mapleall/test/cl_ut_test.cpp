/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <climits>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "cl_option.h"
#include "cl_parser.h"
#include "gtest/gtest.h"

/* ##################### own OptionType for the test #####################
 * ####################################################################### */

static bool utCLTypeChecker = false;

/* You can create own OptionType and Set own Option Parser for him, like this: */
class UTCLType {
 public:
  UTCLType() = default;
  UTCLType(const std::string &data) : data(data) {};

  std::string data;
};
bool operator==(const UTCLType &opt, const std::string &str) {
  bool ret = (opt.data == str) ? true : false;
  return ret;
}
bool operator==(const std::string &str, const UTCLType &opt) {
  bool ret = (opt.data == str) ? true : false;
  return ret;
}

template <>
void maplecl::Option<UTCLType>::Clear() {
  value.data = "";
}

template <>
std::vector<std::string> maplecl::Option<UTCLType>::GetRawValues() {
  return std::vector<std::string>();
}

template <>
maplecl::RetCode maplecl::Option<UTCLType>::Parse(size_t &argsIndex,
    const std::deque<std::string_view> &args, KeyArg &keyArg) {
  utCLTypeChecker = true;
  RetCode err = maplecl::RetCode::kNoError;

  if (args[argsIndex] != "--uttype") {
    return maplecl::RetCode::kParsingErr;
  }

  size_t localArgsIndex = argsIndex + 1;
  /* Second command line argument does not exist */
  if (localArgsIndex >= args.size() || args[localArgsIndex].empty()) {
    return RetCode::kValueEmpty;
  }

  /* In this example, the value of UTCLType must be --UTCLTypeOption */
  if (args[localArgsIndex] == "--UTCLTypeOption") {
    argsIndex += 2; /* 1 for Option Key, 1 for Value */
    err = maplecl::RetCode::kNoError;
    SetValue(UTCLType("--UTCLTypeOption"));
  } else {
    err = maplecl::RetCode::kValueEmpty;
  }

  return err;
}

/* ########################## Test Options ###############################
 * ####################################################################### */

namespace testopts {

  maplecl::OptionCategory defaultCategory;
  maplecl::OptionCategory testCategory1;
  maplecl::OptionCategory testCategory2;

  maplecl::Option<bool> booloptEnabled({"--boole"}, "");
  maplecl::Option<bool> booloptDisabled({"--boold"}, "");

  maplecl::Option<std::string> testStr({"--str"}, "");

  maplecl::Option<uint8_t> testUint8({"--uint8"}, "");
  maplecl::Option<uint16_t> testUint16({"--uint16"}, "");

  maplecl::Option<int32_t> testInt32({"--int32"}, "");
  maplecl::Option<uint32_t> testUint32({"--uint32"}, "");

  maplecl::Option<int64_t> testInt64({"--int64"}, "");
  maplecl::Option<uint64_t> testUint64({"--uint64"}, "");

  maplecl::Option<bool> doubleDefinedOpt({"--tst", "-t"}, "");

  maplecl::Option<bool> defaultBool({"--defbool"}, "", maplecl::Init(true));
  maplecl::Option<std::string> defaultString({"--defstring"}, "", maplecl::Init("Default String"));
  maplecl::Option<int32_t> defaultDigit({"--defdigit"}, "", maplecl::Init(-42));

  maplecl::Option<bool> enable({"--enable"}, "", maplecl::Init(true), maplecl::DisableWith("--no-enable"));

  maplecl::Option<std::string> macro({"-JOIN"}, "", maplecl::kJoinedValue);
  maplecl::Option<int32_t> joindig({"--joindig"}, "", maplecl::kJoinedValue);

  maplecl::Option<std::string> equalStr({"--eqstr"}, "");
  maplecl::Option<int32_t> equalDig({"--eqdig"}, "");

  maplecl::Option<int32_t> reqVal({"--reqval"}, "", maplecl::kRequiredValue, maplecl::Init(-42));
  maplecl::Option<int32_t> optVal({"--optval"}, "", maplecl::kOptionalValue, maplecl::Init(-42));
  maplecl::Option<int32_t> woVal({"--woval"}, "", maplecl::kDisallowedValue, maplecl::Init(-42));

  maplecl::Option<bool> cat1Opt1({"--c1opt1"}, "", {testCategory1});
  maplecl::Option<bool> cat12Opt({"--c12opt"}, "", {testCategory1, testCategory2});

  maplecl::Option<UTCLType> uttype({"--uttype"}, "");

  maplecl::Option<bool> desc({"--desc"}, "It's test description");

  maplecl::Option<bool> dup({"--dup"}, "");

  maplecl::List<std::string> vecString({"--vecstr"}, "");
  maplecl::List<uint32_t> vecDig({"--vecdig"}, "");
  maplecl::List<bool> vecBool({"--vecbool"}, "", maplecl::DisableWith("--no-vecbool"));

  maplecl::List<std::string> vecStringDef({"--vecstrdef"}, "", maplecl::Init("Default"));
  maplecl::List<uint32_t> vecDigDef({"--vecdigdef"}, "", maplecl::Init(100));
  maplecl::List<bool> vecBoolDef({"--vecbooldef"}, "", maplecl::DisableWith("--no-vecbooldef"), maplecl::Init(true));

  maplecl::Option<std::string> common({"--datacommon"}, "");
} // namespace testopts

/* ################# "Enable/Disable Boolean Options" Test ###############
 * ####################################################################### */

TEST(clOptions, boolOpt) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--boole",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool isSet = testopts::booloptEnabled;
  ASSERT_EQ(isSet, true);

  isSet = testopts::booloptDisabled;
  ASSERT_EQ(isSet, false);
}

/* ################# "Set and Compare Options" Test #####################
 * ####################################################################### */

TEST(clOptions, comparableOpt1) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--boole",
    "--str", "DATA",
    "--int32", "-42",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  std::string lStr = "data";
  int32_t lDig = 42;
  bool lBool = false;

  bool isSet = (testopts::booloptEnabled == true);
  ASSERT_EQ(isSet, true);
  isSet = (testopts::booloptEnabled == lBool);
  ASSERT_EQ(isSet, false);

  isSet = (testopts::testStr == "DATA");
  ASSERT_EQ(isSet, true);
  isSet = ("DATA" == testopts::testStr);
  ASSERT_EQ(isSet, true);
  isSet = (testopts::testStr == lStr);
  ASSERT_EQ(isSet, false);

  isSet = (testopts::testInt32 == -42);
  ASSERT_EQ(isSet, true);
  isSet = (-42 == testopts::testInt32);
  ASSERT_EQ(isSet, true);
  isSet = (testopts::testInt32 == lDig);
  ASSERT_EQ(isSet, false);
}

TEST(clOptions, IncorrectVal) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--str", "--boole",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--str");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kValueEmpty);

  bool isSet = (testopts::booloptEnabled == true);
  ASSERT_EQ(isSet, true);
}

/* ################# "Set Digital Options" Test ##########################
 * ####################################################################### */

TEST(clOptions, digitTestMaxVal) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint8", "255",
    "--uint16", "65535",
    "--int32", "2147483647",
    "--uint32", "4294967295",
    "--int64", "9223372036854775807",
    "--uint64", "18446744073709551615",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  uint8_t uint8Dig = testopts::testUint8;
  ASSERT_EQ(uint8Dig, (1 << 8) - 1);

  uint16_t uint16Dig = testopts::testUint16;
  ASSERT_EQ(uint16Dig, (1 << 16) - 1);

  int32_t int32Dig = testopts::testInt32;
  ASSERT_EQ(int32Dig, INT_MAX);

  uint32_t uint32Dig = testopts::testUint32;
  ASSERT_EQ(uint32Dig, UINT_MAX);

  int64_t int64Dig = testopts::testInt64;
  ASSERT_EQ(int64Dig, LLONG_MAX);

  uint64_t uint64Dig = testopts::testUint64;
  ASSERT_EQ(uint64Dig, ULLONG_MAX);
}

TEST(clOptions, digitTestNegativeVal1) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint32", "-10",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uint32");
  ASSERT_EQ(badArgs[1].first, "-10");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kIncorrectValue);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, digitTestNegativeVal2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint64", "-10",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uint64");
  ASSERT_EQ(badArgs[1].first, "-10");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kIncorrectValue);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, digitTestNegativeVal3) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int32", "-2147483648",
    "--int64", "-9223372036854775808",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  int64_t int64Dig = testopts::testInt64;
  ASSERT_EQ(int64Dig, LLONG_MIN);
}

TEST(clOptions, digitIncorrectPrefix) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int32", "--10",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int32");
  ASSERT_EQ(badArgs[1].first, "--10");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kIncorrectValue);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, digitIncorrectVal) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int32", "INCORRECT",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int32");
  ASSERT_EQ(badArgs[1].first, "INCORRECT");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kIncorrectValue);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

/* ################# "Set out of range Value in Option" Test #############
 * ####################################################################### */

TEST(clOptions, digitTestOutOfRange1) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int32", "-2147483649",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int32");
  ASSERT_EQ(badArgs[1].first, "-2147483649");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kOutOfRange);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, digitTestOutOfRange2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint32", "4294967296",
    "--uint16", "65536",
    "--uint8", "256",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 6);
  ASSERT_EQ(badArgs[0].first, "--uint32");
  ASSERT_EQ(badArgs[1].first, "4294967296");
  ASSERT_EQ(badArgs[2].first, "--uint16");
  ASSERT_EQ(badArgs[3].first, "65536");
  ASSERT_EQ(badArgs[4].first, "--uint8");
  ASSERT_EQ(badArgs[5].first, "256");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kOutOfRange);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
  ASSERT_EQ(badArgs[2].second, maplecl::RetCode::kOutOfRange);
  ASSERT_EQ(badArgs[3].second, maplecl::RetCode::kNotRegistered);
  ASSERT_EQ(badArgs[4].second, maplecl::RetCode::kOutOfRange);
  ASSERT_EQ(badArgs[5].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, digitTestOutOfRange3) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int64", "-9223372036854775809",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int64");
  ASSERT_EQ(badArgs[1].first, "-9223372036854775809");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kOutOfRange);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, digitTestOutOfRange4) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint64", "18446744073709551616",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uint64");
  ASSERT_EQ(badArgs[1].first, "18446744073709551616");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kOutOfRange);
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

/* ################# Check double option name definition #################
 * ####################################################################### */

TEST(clOptions, doubleDef1) {
    // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--tst",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool isSet = testopts::doubleDefinedOpt;
  ASSERT_EQ(isSet, true);
}

TEST(clOptions, doubleDef2) {
    // create command line
  const char *argv[] = {
    "CLTest", // program name
    "-t",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool isSet = testopts::doubleDefinedOpt;
  ASSERT_EQ(isSet, true);
}

/* ################# Check default option initialization #################
 * ####################################################################### */

TEST(clOptions, defaultVal) {
    // create command line
  const char *argv[] = {
    "CLTest", // program name
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  /* Default Options are not set in command line but initialized with default value */
  bool isSet = testopts::defaultBool;
  ASSERT_EQ(isSet, true);

  std::string defStr = testopts::defaultString;
  ASSERT_EQ(defStr, "Default String");

  int32_t defaultDigit = testopts::defaultDigit;
  ASSERT_EQ(defaultDigit, -42);
}

/* ########## Check Option disabling with additional no-opt name #########
 * ####################################################################### */

/* check that testopts::enable is enabled by default.
 * It's needed to be sure that --no-enable disables this option */
TEST(clOptions, disableOpt1) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool isSet = testopts::enable;
  ASSERT_EQ(isSet, true);
}

TEST(clOptions, disableOpt2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--no-enable",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool isSet = testopts::enable;
  ASSERT_EQ(isSet, false);
}

/* ################# Check Joined Options ################################
 * ####################################################################### */

TEST(clOptions, joinedOpt) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "-JOINMACRO",
    "--joindig-42",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  std::string joinOpt = testopts::macro;
  ASSERT_EQ(joinOpt, "MACRO");

  int32_t joinDig = testopts::joindig;
  ASSERT_EQ(joinDig, -42);
}

/* ################# Check Options like --opt=value ######################
 * ####################################################################### */

TEST(clOptions, equalOpt) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--eqstr=EQUALSTRING",
    "--eqdig=-42",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  std::string equalStr = testopts::equalStr;
  ASSERT_EQ(equalStr, "EQUALSTRING");

  int32_t equalDig = testopts::equalDig;
  ASSERT_EQ(equalDig, -42);
}

TEST(clOptions, equalOptErr) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--eqstr=",
    "--eqdig=",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  /* --woval must not contain any key values, so 20 will be handled as second key */
  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--eqstr=");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kValueEmpty);

  ASSERT_EQ(badArgs[1].first, "--eqdig=");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kValueEmpty);
}

TEST(clOptions, joinedEqualOpt) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "-JOINTEST=20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  std::string joinOpt = testopts::macro;
  ASSERT_EQ(joinOpt, "MACRO -JOIN TEST=20");
}

/* ################# Check Options with required Value ###################
 * ####################################################################### */

TEST(clOptions, expectedVal1) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--reqval",
    "--boole",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--reqval");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kIncorrectValue);

  bool isSet = testopts::booloptEnabled;
  ASSERT_EQ(isSet, true);
}

TEST(clOptions, expectedVal2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--reqval", "20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  int32_t equalDig = testopts::reqVal;
  ASSERT_EQ(equalDig, 20);
}

TEST(clOptions, expectedVal3) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--optval",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  int32_t equalDig = testopts::optVal;
  ASSERT_EQ(equalDig, -42);
}

TEST(clOptions, expectedVal4) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--optval=20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  int32_t equalDig = testopts::optVal;
  ASSERT_EQ(equalDig, 20);
}

TEST(clOptions, expectedVal5) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--optval", "20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);
}

TEST(clOptions, expectedVal6) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--optval", "--boole",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  testopts::booloptEnabled.Clear();
  testopts::optVal.Clear();
  ASSERT_EQ(testopts::booloptEnabled.GetValue(), false);
  ASSERT_EQ(testopts::optVal.IsEnabledByUser(), false);

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  ASSERT_EQ(testopts::booloptEnabled.GetValue(), true);
  ASSERT_EQ(testopts::optVal.IsEnabledByUser(), true);
  ASSERT_EQ(testopts::optVal.GetValue(), -42);
}

TEST(clOptions, expectedVal7) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--woval", "20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  /* --woval must not contain any key values, so 20 will be handled as second key */
  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "20");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, expectedVal8) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--woval=20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--woval=20");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kUnnecessaryValue);
}

TEST(clOptions, expectedVal9) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--woval",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  int32_t equalDig = testopts::woVal;
  ASSERT_EQ(equalDig, -42);
}

/* ################# Options Category Test ###############################
 * ####################################################################### */

TEST(clOptions, optCategory1) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--c1opt1",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv, testopts::defaultCategory);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--c1opt1");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, optCategory2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--c1opt1",
    "--c12opt",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv, testopts::testCategory1);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool isSet = testopts::cat1Opt1;
  ASSERT_EQ(isSet, true);

  isSet = testopts::cat12Opt;
  ASSERT_EQ(isSet, true);
}

TEST(clOptions, optCategory3) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--c12opt",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv, testopts::testCategory2);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool isSet = testopts::cat12Opt;
  ASSERT_EQ(isSet, true);
}

/* ################# Own OptionType Test #################################
 * ####################################################################### */

TEST(clOptions, ownOptionType1) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uttype",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  ASSERT_EQ(utCLTypeChecker, false);

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  ASSERT_EQ(utCLTypeChecker, true);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--uttype");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kValueEmpty);
}

TEST(clOptions, ownOptionType2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uttype", "TEST",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  utCLTypeChecker = false;
  ASSERT_EQ(utCLTypeChecker, false);

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kParsingErr);

  ASSERT_EQ(utCLTypeChecker, true);

  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uttype");
  ASSERT_EQ(badArgs[0].second, maplecl::RetCode::kValueEmpty);
  ASSERT_EQ(badArgs[1].first, "TEST");
  ASSERT_EQ(badArgs[1].second, maplecl::RetCode::kNotRegistered);
}

TEST(clOptions, ownOptionType3) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uttype", "--UTCLTypeOption",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  utCLTypeChecker = false;
  ASSERT_EQ(utCLTypeChecker, false);

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  ASSERT_EQ(utCLTypeChecker, true);
  UTCLType opt = testopts::uttype;

  std::string tst = "--UTCLTypeOption";
  bool isSet = (tst == opt);

  ASSERT_EQ(isSet, isSet);
  ASSERT_EQ(opt, "--UTCLTypeOption");
}

/* ################# Duplicated Option ###################################
 * ####################################################################### */

TEST(clOptions, duplicatedOptions) {
  EXPECT_DEATH(maplecl::Option<bool> dup2({"--dup"}, ""), "");
}

/* ##################### Check Description ###############################
 * ####################################################################### */

TEST(clOptions, description) {
  ASSERT_EQ(testopts::desc.GetDescription(), "It's test description");
}

/* ##################### Check maplecl::list ##################################
 * ####################################################################### */

TEST(clOptions, optList1) {
  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--vecstr", "data1",
    "--vecstr", "data2",
    "--vecstr", "data3",
    "--vecdig", "10",
    "--vecdig", "20",
    "--vecbool", "--no-vecbool", "--vecbool",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  auto strVals = testopts::vecString.GetValues();
  ASSERT_EQ(strVals.size(), 3);
  ASSERT_EQ(strVals[0], "data1");
  ASSERT_EQ(strVals[1], "data2");
  ASSERT_EQ(strVals[2], "data3");

  auto digVals = testopts::vecDig.GetValues();
  ASSERT_EQ(digVals.size(), 2);
  ASSERT_EQ(digVals[0], 10);
  ASSERT_EQ(digVals[1], 20);

  auto boolVals = testopts::vecBool.GetValues();
  ASSERT_EQ(boolVals.size(), 3);
  ASSERT_EQ(boolVals[0], true);
  ASSERT_EQ(boolVals[1], false);
  ASSERT_EQ(boolVals[2], true);

  maplecl::CommandLine::GetCommandLine().Clear(); // to clear previous test data

  ASSERT_EQ(testopts::vecBool.GetValues().size(), 0);
  ASSERT_EQ(testopts::vecDig.GetValues().size(), 0);
  ASSERT_EQ(testopts::vecString.GetValues().size(), 0);
}

TEST(clOptions, optList2) {
  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--vecstrdef", "data1",
    "--vecdigdef", "10",
    "--vecbooldef", "--no-vecbooldef",
    nullptr };
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  ASSERT_EQ(testopts::vecBoolDef.GetValues().size(), 1);
  ASSERT_EQ(testopts::vecDigDef.GetValues().size(), 1);
  ASSERT_EQ(testopts::vecStringDef.GetValues().size(), 1);

  ASSERT_EQ(testopts::vecDigDef.GetValues()[0], 100);
  ASSERT_EQ(testopts::vecBoolDef.GetValues()[0], true);
  ASSERT_EQ(testopts::vecStringDef.GetValues()[0], "Default");

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  ASSERT_EQ(testopts::vecStringDef.GetValues().size(), 2);
  ASSERT_EQ(testopts::vecDigDef.GetValues().size(), 2);
  ASSERT_EQ(testopts::vecBoolDef.GetValues().size(), 3);

  ASSERT_EQ(testopts::vecStringDef.GetValues()[1], "data1");
  ASSERT_EQ(testopts::vecDigDef.GetValues()[1], 10);
  ASSERT_EQ(testopts::vecBoolDef.GetValues()[1], true);
  ASSERT_EQ(testopts::vecBoolDef.GetValues()[2], false);

  maplecl::CommandLine::GetCommandLine().Clear(); // to clear previous test data
  ASSERT_EQ(testopts::vecStringDef.GetValues().size(), 1);
  ASSERT_EQ(testopts::vecDigDef.GetValues().size(), 1);
  ASSERT_EQ(testopts::vecBoolDef.GetValues().size(), 1);
}

/* ##################### "GetCommonValue from interface" check ###########
 * ####################################################################### */
TEST(clOptions, common) {
  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--datacommon", "datacommon",
    nullptr };
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  maplecl::OptionInterface *commonOpt = &testopts::common;
  std::string val = commonOpt->GetCommonValue();
  ASSERT_EQ(val, "datacommon");
}

/* ##################### "CopyIfEnabled" check ###########
 * ####################################################################### */
TEST(clOptions, copyifenabled) {
  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--boole",
    "--str", "TEST",
    "--uint8", "10",
    nullptr };
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  maplecl::CommandLine::GetCommandLine().Clear(); // to clear previous test data
  /* Check that options are cleared */
  ASSERT_EQ(testopts::booloptEnabled == false, true);
  ASSERT_EQ(testopts::booloptEnabled.IsEnabledByUser(), false);
  ASSERT_EQ(testopts::booloptDisabled == false, true);
  ASSERT_EQ(testopts::booloptDisabled.IsEnabledByUser(), false);
  ASSERT_EQ(testopts::testStr == "", true);
  ASSERT_EQ(testopts::testStr.IsEnabledByUser(), false);
  ASSERT_EQ(testopts::testUint8 == (uint8_t)0, true);
  ASSERT_EQ(testopts::testUint8.IsEnabledByUser(), false);

  auto err = maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, maplecl::RetCode::kNoError);

  bool boole = false, boold = false;
  std::string strTmp = "";
  uint8_t u8Tmp = 0;
  int tstEnabled = -1, tstDisabled = -1;

  maplecl::CopyIfEnabled(boole, testopts::booloptEnabled);
  maplecl::CopyIfEnabled(boold, testopts::booloptDisabled);
  maplecl::CopyIfEnabled(tstEnabled, 10, testopts::booloptEnabled);
  maplecl::CopyIfEnabled(tstDisabled, 10, testopts::booloptDisabled);
  maplecl::CopyIfEnabled(strTmp, testopts::testStr);
  maplecl::CopyIfEnabled(u8Tmp, testopts::testUint8);

  ASSERT_EQ(boole, true);
  ASSERT_EQ(boold, false);
  ASSERT_EQ(tstEnabled, 10);
  ASSERT_EQ(tstDisabled, -1);
  ASSERT_EQ(strTmp, "TEST");
  ASSERT_EQ(u8Tmp, 10);
}