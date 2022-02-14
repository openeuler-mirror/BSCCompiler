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
cl::RetCode cl::Option<UTCLType>::Parse(ssize_t &argsIndex,
                                        const std::vector<std::string_view> &args,
                                        KeyArg &keyArg) {
  utCLTypeChecker = true;
  RetCode err = cl::RetCode::noError;

  if (args[argsIndex] != "--uttype") {
    return cl::RetCode::parsingErr;
  }

  ssize_t localArgsIndex = argsIndex + 1;
  /* Second command line argument does not exist */
  if (localArgsIndex >= args.size() || args[localArgsIndex].empty()) {
    return RetCode::valueEmpty;
  }

  /* In this example, the value of UTCLType must be --UTCLTypeOption */
  if (args[localArgsIndex] == "--UTCLTypeOption") {
    argsIndex += 2; /* 1 for Option Key, 1 for Value */
    err = cl::RetCode::noError;
    SetValue(UTCLType("--UTCLTypeOption"));
  } else {
    err = cl::RetCode::valueEmpty;
  }

  return err;
}

/* ########################## Test Options ###############################
 * ####################################################################### */

namespace testopts {

  cl::OptionCategory defaultCategory;
  cl::OptionCategory testCategory1;
  cl::OptionCategory testCategory2;

  cl::Option<bool> booloptEnabled({"--boole"}, "");
  cl::Option<bool> booloptDisabled({"--boold"}, "");

  cl::Option<std::string> testStr({"--str"}, "");

  cl::Option<int32_t> testInt32({"--int32"}, "");
  cl::Option<uint32_t> testUint32({"--uint32"}, "");

  cl::Option<int64_t> testInt64({"--int64"}, "");
  cl::Option<uint64_t> testUint64({"--uint64"}, "");

  cl::Option<bool> doubleDefinedOpt({"--tst", "-t"}, "");

  cl::Option<bool> defaultBool({"--defbool"}, "", cl::Init(true));
  cl::Option<std::string> defaultString({"--defstring"}, "", cl::Init("Default String"));
  cl::Option<int32_t> defaultDigit({"--defdigit"}, "", cl::Init(-42));

  cl::Option<bool> enable({"--enable"}, "", cl::Init(true), cl::DisableWith("--no-enable"));

  cl::Option<std::string> macro({"-D"}, "", cl::joinedValue);
  cl::Option<int32_t> joindig({"--joindig"}, "", cl::joinedValue);

  cl::Option<std::string> equalStr({"--eqstr"}, "");
  cl::Option<int32_t> equalDig({"--eqdig"}, "");

  cl::Option<int32_t> reqVal({"--reqval"}, "", cl::requiredValue, cl::Init(-42));
  cl::Option<int32_t> optVal({"--optval"}, "", cl::optionalValue, cl::Init(-42));
  cl::Option<int32_t> woVal({"--woval"}, "", cl::disallowedValue, cl::Init(-42));

  cl::Option<bool> cat1Opt1({"--c1opt1"}, "", {&testCategory1});
  cl::Option<bool> cat12Opt({"--c12opt"}, "", {&testCategory1, &testCategory2});

  cl::Option<UTCLType> uttype({"--uttype"}, "");

  cl::Option<bool> desc({"--desc"}, "It's test description");
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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

  bool isSet = testopts::booloptEnabled;
  ASSERT_EQ(isSet, true);

  isSet = testopts::booloptDisabled;
  ASSERT_EQ(isSet, false);
}

/* ################# "Set and Comapare Options" Test #####################
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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--str");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::valueEmpty);

  bool isSet = (testopts::booloptEnabled == true);
  ASSERT_EQ(isSet, true);
}

/* ################# "Set Digital Options" Test ##########################
 * ####################################################################### */

TEST(clOptions, digitTestMaxVal) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int32", "2147483647",
    "--uint32", "4294967295",
    "--int64", "9223372036854775807",
    "--uint64", "18446744073709551615",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uint32");
  ASSERT_EQ(badArgs[1].first, "-10");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::incorrectValue);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
}

TEST(clOptions, digitTestNegativeVal2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint64", "-10",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uint64");
  ASSERT_EQ(badArgs[1].first, "-10");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::incorrectValue);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
}

TEST(clOptions, digitTestNegativeVal3) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int32", "-2147483648",
    "--int64", "-9223372036854775808",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int32");
  ASSERT_EQ(badArgs[1].first, "--10");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::incorrectValue);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
}

TEST(clOptions, digitIncorrectVal) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int32", "INCORRECT",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int32");
  ASSERT_EQ(badArgs[1].first, "INCORRECT");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::incorrectValue);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int32");
  ASSERT_EQ(badArgs[1].first, "-2147483649");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::outOfRange);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
}

TEST(clOptions, digitTestOutOfRange2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint32", "4294967296",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uint32");
  ASSERT_EQ(badArgs[1].first, "4294967296");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::outOfRange);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
}

TEST(clOptions, digitTestOutOfRange3) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--int64", "-9223372036854775809",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--int64");
  ASSERT_EQ(badArgs[1].first, "-9223372036854775809");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::outOfRange);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
}

TEST(clOptions, digitTestOutOfRange4) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--uint64", "18446744073709551616",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uint64");
  ASSERT_EQ(badArgs[1].first, "18446744073709551616");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::outOfRange);
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

  bool isSet = testopts::enable;
  ASSERT_EQ(isSet, false);
}

/* ################# Check Joined Options ################################
 * ####################################################################### */

TEST(clOptions, joinedOpt) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "-DMACRO",
    "--joindig-42",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  /* --woval must not contain any key values, so 20 will be handled as second key */
  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--eqstr=");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::valueEmpty);

  ASSERT_EQ(badArgs[1].first, "--eqdig=");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::valueEmpty);
}

TEST(clOptions, joinedEqualOpt) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "-DTEST=20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

  std::string joinOpt = testopts::macro;
  ASSERT_EQ(joinOpt, "TEST=20");
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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--reqval");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::incorrectValue);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

  int32_t equalDig = testopts::optVal;
  ASSERT_EQ(equalDig, -42);
}

TEST(clOptions, expectedVal4) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--optval", "20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

  int32_t equalDig = testopts::optVal;
  ASSERT_EQ(equalDig, 20);
}

TEST(clOptions, expectedVal5) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--woval", "20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  /* --woval must not contain any key values, so 20 will be handled as second key */
  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "20");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::notRegistered);
}

TEST(clOptions, expectedVal6) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--woval=20",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--woval=20");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::unnecessaryValue);
}

TEST(clOptions, expectedVal7) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--woval",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv, testopts::defaultCategory);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--c1opt1");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::notRegistered);
}

TEST(clOptions, optCategory2) {

  // create command line
  const char *argv[] = {
    "CLTest", // program name
    "--c1opt1",
    "--c12opt",
    nullptr};
  int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv, testopts::testCategory1);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv, testopts::testCategory2);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  ASSERT_EQ(utCLTypeChecker, true);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 1);
  ASSERT_EQ(badArgs[0].first, "--uttype");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::valueEmpty);
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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::parsingErr);

  ASSERT_EQ(utCLTypeChecker, true);

  auto &badArgs = cl::CommandLine::GetCommandLine().badCLArgs;
  ASSERT_EQ(badArgs.size(), 2);
  ASSERT_EQ(badArgs[0].first, "--uttype");
  ASSERT_EQ(badArgs[0].second, cl::RetCode::valueEmpty);
  ASSERT_EQ(badArgs[1].first, "TEST");
  ASSERT_EQ(badArgs[1].second, cl::RetCode::notRegistered);
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

  auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
  ASSERT_EQ(err, cl::RetCode::noError);

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

  cl::Option<bool> dup({"--dup"}, "");
  EXPECT_DEATH(cl::Option<bool> dup2({"--dup"}, ""), "");
}

/* ##################### Check Description ###############################
 * ####################################################################### */

TEST(clOptions, description) {
  ASSERT_EQ(testopts::desc.GetDescription(), "It's test description");
}
