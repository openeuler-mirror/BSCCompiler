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

#include "driver_options.h"

#include <stdint.h>
#include <string>

namespace opts::hir2mpl {

maplecl::Option<bool> help({"--help", "-h"},
                      "  -h, -help              : print usage and exit",
                      {hir2mplCategory});

maplecl::Option<bool> version({"--version", "-v"},
                         "  -v, -version           : print version and exit",
                         {hir2mplCategory});

maplecl::Option<std::string> mpltSys({"--mplt-sys", "-mplt-sys"},
                                "  -mplt-sys sys1.mplt,sys2.mplt\n"
                                "                         : input sys mplt files",
                                {hir2mplCategory});

maplecl::Option<std::string> mpltApk({"--mplt-apk", "-mplt-apk"},
                                "  -mplt-apk apk1.mplt,apk2.mplt\n"
                                "                         : input apk mplt files",
                                {hir2mplCategory});

maplecl::Option<std::string> mplt({"--mplt", "-mplt"},
                             "  -mplt lib1.mplt,lib2.mplt\n"
                             "                         : input mplt files",
                             {hir2mplCategory});

maplecl::Option<std::string> inClass({"--in-class", "-in-class"},
                                "  -in-class file1.jar,file2.jar\n"
                                "                         : input class files",
                                {hir2mplCategory});

maplecl::Option<std::string> inJar({"--in-jar", "-in-jar"},
                              "  -in-jar file1.jar,file2.jar\n"
                              "                         : input jar files",
                              {hir2mplCategory});

maplecl::Option<std::string> inDex({"--in-dex", "-in-dex"},
                              "  -in-dex file1.dex,file2.dex\n"
                              "                         : input dex files",
                              {hir2mplCategory});

maplecl::Option<std::string> inAst({"--in-ast", "-in-ast"},
                              "  -in-ast file1.ast,file2.ast\n"
                              "                         : input ast files",
                              {hir2mplCategory});

maplecl::Option<std::string> inMast({"--in-mast", "-in-mast"},
                               "  -in-mast file1.mast,file2.mast\n"
                               "                         : input mast files",
                               {hir2mplCategory});

maplecl::Option<std::string> output({"--output", "-p"},
                               "  -p, -output            : output path",
                               {hir2mplCategory});

maplecl::Option<std::string> outputName({"--output-name", "-o"},
                                   "  -o, -output-name       : output name",
                                   {hir2mplCategory});

maplecl::Option<bool> mpltOnly({"--t", "-t"},
                          "  -t                     : generate mplt only",
                          {hir2mplCategory});

maplecl::Option<bool> asciimplt({"--asciimplt", "-asciimplt"},
                           "  -asciimplt             : generate mplt in ascii format",
                           {hir2mplCategory});

maplecl::Option<bool> dumpInstComment({"--dump-inst-comment", "-dump-inst-comment"},
                                 "  -dump-inst-comment     : dump instruction comment",
                                 {hir2mplCategory});

maplecl::Option<bool> noMplFile({"--no-mpl-file", "-no-mpl-file"},
                           "  -no-mpl-file           : disable dump mpl file",
                           {hir2mplCategory});

maplecl::Option<uint32_t> dumpLevel({"--dump-level", "-d"},
                               "  -d, -dump-level xx     : debug info dump level\n"
                               "                           [0] disable\n"
                               "                           [1] dump simple info\n"
                               "                           [2] dump detail info\n"
                               "                           [3] dump debug info",
                               {hir2mplCategory});

maplecl::Option<bool> dumpTime({"--dump-time", "-dump-time"},
                          "  -dump-time             : dump time",
                          {hir2mplCategory});

maplecl::Option<bool> dumpComment({"--dump-comment", "-dump-comment"},
                             "  -dump-comment          : gen comment stmt",
                             {hir2mplCategory});

maplecl::Option<bool> dumpLOC({"--dump-LOC", "-dump-LOC"},
                         "  -dump-LOC              : gen LOC",
                         {hir2mplCategory});

maplecl::Option<bool> dbgFriendly({"--g", "-g"},
                             "  -g                     : emit debug friendly mpl, including\n"
                             "                           no variable renaming\n"
                             "                           gen LOC",
                             {hir2mplCategory});

maplecl::Option<bool> dumpPhaseTime({"--dump-phase-time", "-dump-phase-time"},
                               "  -dump-phase-time       : dump total phase time",
                               {hir2mplCategory});

maplecl::Option<bool> dumpPhaseTimeDetail({"-dump-phase-time-detail", "--dump-phase-time-detail"},
                                     "  -dump-phase-time-detail\n"      \
                                     "                         : dump phase time for each method",
                                     {hir2mplCategory});

maplecl::Option<bool> rc({"-rc", "--rc"},
                    "  -rc                    : enable rc",
                    {hir2mplCategory});

maplecl::Option<bool> nobarrier({"-nobarrier", "--nobarrier"},
                           "  -nobarrier             : no barrier",
                           {hir2mplCategory});

maplecl::Option<bool> usesignedchar({"-usesignedchar", "--usesignedchar"},
                               "  -usesignedchar         : use signed char",
                               {hir2mplCategory});

maplecl::Option<bool> be({"-be", "--be"},
                    "  -be                    : enable big endian",
                    {hir2mplCategory});

maplecl::Option<bool> o2({"-O2", "--O2"},
                    "  -O2                    : enable hir2mpl O2 optimize",
                    {hir2mplCategory});

maplecl::Option<bool> simplifyShortCircuit({"-simplify-short-circuit", "--simplify-short-circuit"},
                                      "  -simplify-short-circuit\n"     \
                                      "                         : enable simplify short circuit",
                                      {hir2mplCategory});

maplecl::Option<bool> enableVariableArray({"-enable-variable-array", "--enable-variable-array"},
                                     "  -enable-variable-array\n" \
                                     "                         : enable variable array",
                                     {hir2mplCategory});

maplecl::Option<uint32_t> funcInliceSize({"-func-inline-size", "--func-inline-size"},
                                    "  -func-inline-size      : set func inline size",
                                    {hir2mplCategory});

maplecl::Option<uint32_t> np({"-np", "--np"},
                        "  -np num                : number of threads",
                        {hir2mplCategory});

maplecl::Option<bool> dumpThreadTime({"-dump-thread-time", "--dump-thread-time"},
                                "  -dump-thread-time      : dump thread time in mpl schedular",
                                {hir2mplCategory});

maplecl::Option<std::string> xbootclasspath({"-Xbootclasspath", "--Xbootclasspath"},
                                       "  -Xbootclasspath=bootclasspath\n" \
                                       "                         : boot class path list",
                                       {hir2mplCategory});

maplecl::Option<std::string> classloadercontext({"-classloadercontext", "--classloadercontext"},
                                           "  -classloadercontext=pcl\n" \
                                           "                         : class loader context \n" \
                                           "                         : path class loader",
                                           {hir2mplCategory});

maplecl::Option<std::string> dep({"-dep", "--dep"},
                            "  -dep=all or func\n"                      \
                            "                         : [all]  collect all dependent types\n" \
                            "                         : [func] collect dependent types in function",
                            {hir2mplCategory});

maplecl::Option<std::string> depsamename({"-depsamename", "--depsamename"},
                                    "  -DepSameNamePolicy=sys or src\n" \
                                    "               : [sys] load type from sys when on-demand load same name type\n" \
                                    "               : [src] load type from src when on-demand load same name type",
                                    {hir2mplCategory});

maplecl::Option<bool> npeCheckDynamic({"-npe-check-dynamic", "--npe-check-dynamic"},
                                 "  -npe-check-dynamic     : Nonnull pointr dynamic checking",
                                 {hir2mplCategory});

maplecl::Option<bool> boundaryCheckDynamic({"-boundary-check-dynamic", "--boundary-check-dynamic"},
                                      "  -boundary-check-dynamic\n"     \
                                      "                         : Boundary dynamic checking",
                                      {hir2mplCategory});

maplecl::Option<bool> safeRegion({"-safe-region", "--safe-region"},
                            "  -boundary-check-dynamic\n"               \
                            "  -safe-region           : Enable safe region",
                            {hir2mplCategory});

maplecl::Option<bool> dumpFEIRBB({"-dump-bb", "--dump-bb"},
                            "  -dump-bb               : dump basic blocks info",
                            {hir2mplCategory});

maplecl::Option<std::string> dumpFEIRCFGGraph({"-dump-cfg", "--dump-cfg"},
                                         "  -dump-cfg funcname1,funcname2\n" \
                                         "                         : dump cfg graph to dot file",
                                         {hir2mplCategory});

maplecl::Option<bool> wpaa({"-wpaa", "--wpaa"},
                      "  -dump-cfg funcname1,funcname2\n"               \
                      "  -wpaa                  : enable whole program ailas analysis",
                      {hir2mplCategory});

maplecl::Option<bool> debug({"-debug", "--debug"},
                      "  -debug                 : dump enabled options",
                      {hir2mplCategory});

}
