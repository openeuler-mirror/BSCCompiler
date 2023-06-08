/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_DRIVER_INCLUDE_SAFE_EXE_H
#define MAPLE_DRIVER_INCLUDE_SAFE_EXE_H

/* To start a new process for dex2mpl/mplipa, we need sys/wait on unix-like systems to
 * make it complete. However, there is not a sys/wait.h for mingw, so we used createprocess
 * in windows.h instead
 */
#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#else
#include <sys/wait.h>
#endif

#include <unistd.h>
#include <cstdlib>
#include "error_code.h"
#include "mpl_logging.h"
#include "mpl_options.h"
#include "string_utils.h"
#include "securec.h"

namespace maple {
enum class Compilee {
  kGcc,
  kHir2mpl,
  kUnknown
};

class SafeExe {
 public:
#ifndef _WIN32
  static ErrorCode HandleCommand(const std::string &cmd, const std::string &args) {
    std::vector<std::string> vectorArgs = ParseArgsVector(cmd, args);
    // extra space for exe name and args
    char **argv = new char *[vectorArgs.size() + 1];
    // argv[0] is program name
    // copy args
    for (size_t j = 0; j < vectorArgs.size(); ++j) {
      size_t strLength = vectorArgs[j].size();
      argv[j] = new char[strLength + 1];
      strncpy_s(argv[j], strLength + 1, vectorArgs[j].c_str(), strLength);
      argv[j][strLength] = '\0';
    }
    // end of arguments sentinel is nullptr
    argv[vectorArgs.size()] = nullptr;
    fflush(nullptr);
    pid_t pid = fork();
    ErrorCode ret = kErrorNoError;
    if (pid == 0) {
      // child process
      fflush(nullptr);
      if (execv(cmd.c_str(), argv) < 0) {
        for (size_t j = 0; j < vectorArgs.size(); ++j) {
          delete [] argv[j];
        }
        delete [] argv;
        exit(1);
      }
    } else {
      // parent process
      int status = -1;
      waitpid(pid, &status, 0);
      auto exitStatus = static_cast<uint32>(status);
      if (!WIFEXITED(exitStatus)) {
        LogInfo::MapleLogger() << "Error while Exe, cmd: " << cmd << " args: " << args << '\n';
        ret = kErrorCompileFail;
      } else if (WEXITSTATUS(exitStatus) != 0) {
        LogInfo::MapleLogger() << "Error while Exe, cmd: " << cmd << " args: " << args << '\n';
        ret = kErrorCompileFail;
      }
    }

    for (size_t j = 0; j < vectorArgs.size(); ++j) {
      delete [] argv[j];
    }
    delete [] argv;
    return ret;
  }

  static ErrorCode HandleCommand(const std::string &cmd, const MplOptions &mplOptions,
                                 const std::vector<MplOption> &options) {
    size_t argIndex;
    char **argv;
    Compilee compileeFlag = Compilee::kUnknown;
    std::string ldLibPath = "";
    size_t index = cmd.find_last_of("-");
    if (index > 0 && index < cmd.size() && cmd.substr(index) == "-gcc") {
      compileeFlag = Compilee::kGcc;
      for (auto &opt : options) {
        if (opt.GetKey() == "-c") {
          compileeFlag = Compilee::kUnknown;
        }
      }
    } else if (StringUtils::GetStrAfterLast(cmd, kFileSeperatorStr) == "hir2mpl" ||
               StringUtils::GetStrAfterLast(cmd, kFileSeperatorStr) == "clang") {
      compileeFlag = Compilee::kHir2mpl;
      if (FileUtils::SafeGetenv(kMapleRoot) != "") {
        ldLibPath += FileUtils::SafeGetenv(kMapleRoot) + "/build/tools/hpk/:";
        ldLibPath += FileUtils::SafeGetenv(kMapleRoot) + \
					 "/tools/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-enhanced/lib";
      } else {
        ldLibPath += mplOptions.GetExeFolder().substr(0, mplOptions.GetExeFolder().length() - 4);
        ldLibPath += "thirdparty/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-enhanced/lib";
      }
    }
    std::tie(argv, argIndex) = GenerateUnixArguments(cmd, options, compileeFlag);
    if (opts::debug) {
      LogInfo::MapleLogger() << "Run: " << cmd;
      for (auto &opt : options) {
        LogInfo::MapleLogger() << " " << opt.GetKey() << " " << opt.GetValue();
      }
      if (compileeFlag == Compilee::kGcc) {
        for (auto &opt : maplecl::CommandLine::GetCommandLine().GetLinkOptions()) {
          LogInfo::MapleLogger() << " " << opt;
        }
      }
      LogInfo::MapleLogger() << "\n";
    }

    fflush(nullptr);
    pid_t pid = fork();
    ErrorCode ret = kErrorNoError;
    if (pid == 0) {
      // child process
      fflush(nullptr);
      if (compileeFlag == Compilee::kHir2mpl) {
        std::string ld_path = ":";
        if (FileUtils::SafeGetenv(kLdLibPath) != "") {
          ld_path += FileUtils::SafeGetenv(kLdLibPath);
          ldLibPath += ld_path;
        }
        setenv("LD_LIBRARY_PATH", ldLibPath.c_str(), 1);
      }

      if (execv(cmd.c_str(), argv) < 0) {
        /* last argv[argIndex] is nullptr, so it's j < argIndex (NOT j <= argIndex) */
        for (size_t j = 0; j < argIndex; ++j) {
          delete [] argv[j];
        }
        delete [] argv;
        exit(1);
      }
    } else {
      // parent process
      int status = -1;
      waitpid(pid, &status, 0);
      auto exitStatus = static_cast<uint32>(status);
      if (!WIFEXITED(exitStatus)) {
        ret = kErrorCompileFail;
      } else if (WEXITSTATUS(exitStatus) != 0) {
        ret = kErrorCompileFail;
      }

      if (ret != kErrorNoError && opts::debug.IsEnabledByUser()) {
        LogInfo::MapleLogger() << "Error while Exe, cmd: " << cmd << " args: ";
        for (auto &opt : options) {
          LogInfo::MapleLogger() << opt.GetKey() << " " << opt.GetValue() << " ";
        }
        if (compileeFlag == Compilee::kGcc) {
          for (auto &opt : maplecl::CommandLine::GetCommandLine().GetLinkOptions()) {
            LogInfo::MapleLogger() << opt << " ";
          }
        }
        LogInfo::MapleLogger() << "\n";
      }
    }

    /* last argv[argIndex] is nullptr, so it's j < argIndex (NOT j <= argIndex) */
    for (size_t j = 0; j < argIndex; ++j) {
      delete [] argv[j];
    }
    delete [] argv;
    return ret;
  }
#else
  static ErrorCode HandleCommand(const std::string &cmd, const std::string &args) {
    ErrorCode ret = ErrorCode::kErrorNoError;

    STARTUPINFO startInfo;
    PROCESS_INFORMATION pInfo;
    DWORD exitCode;

    errno_t retSafe = memset_s(&startInfo, sizeof(STARTUPINFO), 0, sizeof(STARTUPINFO));
    CHECK_FATAL(retSafe == EOK, "memset_s for StartUpInfo failed when HandleComand");

    startInfo.cb = sizeof(STARTUPINFO);

    char* appName = strdup(cmd.c_str());
    char* cmdLine = strdup(args.c_str());
    CHECK_FATAL(appName != nullptr, "strdup for appName failed");
    CHECK_FATAL(cmdLine != nullptr, "strdup for cmdLine failed");

    bool success = CreateProcess(appName, cmdLine, NULL, NULL, FALSE,
                                 NORMAL_PRIORITY_CLASS, NULL, NULL, &startInfo, &pInfo);
    CHECK_FATAL(success != 0, "CreateProcess failed when HandleCommond");

    WaitForSingleObject(pInfo.hProcess, INFINITE);
    GetExitCodeProcess(pInfo.hProcess, &exitCode);

    if (exitCode != 0) {
      LogInfo::MapleLogger() << "Error while Exe, cmd: " << cmd << " args: " << args
                             << " exitCode: " << exitCode << '\n';
      ret = ErrorCode::kErrorCompileFail;
    }

    free(appName);
    free(cmdLine);
    appName = nullptr;
    cmdLine = nullptr;
    return ret;
  }

  static ErrorCode HandleCommand(const std::string &cmd,
                                 const std::vector<MplOption> &options) {
    ErrorCode ret = ErrorCode::kErrorNoError;

    STARTUPINFO startInfo;
    PROCESS_INFORMATION pInfo;
    DWORD exitCode;

    errno_t retSafe = memset_s(&startInfo, sizeof(STARTUPINFO), 0, sizeof(STARTUPINFO));
    CHECK_FATAL(retSafe == EOK, "memset_s for StartUpInfo failed when HandleComand");

    startInfo.cb = sizeof(STARTUPINFO);\
    std::string argString;
    for (auto &opt : options) {
      argString += opt.GetKey() + " " + opt.GetValue() + " ";
    }

    char* appName = strdup(cmd.c_str());
    char* cmdLine = strdup(argString.c_str());
    CHECK_FATAL(appName != nullptr, "strdup for appName failed");
    CHECK_FATAL(cmdLine != nullptr, "strdup for cmdLine failed");

    bool success = CreateProcess(appName, cmdLine, NULL, NULL, FALSE,
                                 NORMAL_PRIORITY_CLASS, NULL, NULL, &startInfo, &pInfo);
    CHECK_FATAL(success != 0, "CreateProcess failed when HandleCommond");

    WaitForSingleObject(pInfo.hProcess, INFINITE);
    GetExitCodeProcess(pInfo.hProcess, &exitCode);

    if (exitCode != 0) {
      LogInfo::MapleLogger() << "Error while Exe, cmd: " << cmd << " args: " << argString
                             << " exitCode: " << exitCode << '\n';
      ret = ErrorCode::kErrorCompileFail;
    }

    free(appName);
    free(cmdLine);
    appName = nullptr;
    cmdLine = nullptr;
    return ret;
  }
#endif

  static ErrorCode Exe(const std::string &cmd, const std::string &args) {
    LogInfo::MapleLogger() << "Starting:" << cmd << args << '\n';
    if (StringUtils::HasCommandInjectionChar(cmd) || StringUtils::HasCommandInjectionChar(args)) {
      LogInfo::MapleLogger() << "Error while Exe, cmd: " << cmd << " args: " << args << '\n';
      return kErrorCompileFail;
    }
    ErrorCode ret = HandleCommand(cmd, args);
    return ret;
  }

  static ErrorCode Exe(const std::string &cmd, const MplOptions &mplOptions,
                       const std::vector<MplOption> &options) {
    if (StringUtils::HasCommandInjectionChar(cmd)) {
      LogInfo::MapleLogger() << "Error while Exe, cmd: " << cmd << '\n';
      return kErrorCompileFail;
    }
    ErrorCode ret = HandleCommand(cmd, mplOptions, options);
    return ret;
  }

 private:
  static std::vector<std::string> ParseArgsVector(const std::string &cmd, const std::string &args) {
    std::vector<std::string> tmpArgs;
    StringUtils::Split(args, tmpArgs, ' ');
    // remove ' ' in vector
    for (auto iter = tmpArgs.begin(); iter != tmpArgs.end();) {
      if (*iter == " " || *iter == "") {
        iter = tmpArgs.erase(iter);
      } else {
        ++iter;
      }
    }
    (void)tmpArgs.insert(tmpArgs.cbegin(), cmd);
    return tmpArgs;
  }

  static std::tuple<char **, size_t> GenerateUnixArguments(const std::string &cmd,
      const std::vector<MplOption> &options, Compilee compileeFlag) {
    /* argSize=2, because we reserve 1st arg as exe binary, and another arg as last nullptr arg */
    size_t argSize = 2;

    /* Calculate how many args are needed.
     * (* 2) is needed, because we have key and value arguments in each option
     */
    if (compileeFlag == Compilee::kGcc && maplecl::CommandLine::GetCommandLine().GetLinkOptions().size() > 0) {
      argSize += maplecl::CommandLine::GetCommandLine().GetLinkOptions().size();
    }
    argSize += options.size() * 2;

    /* extra space for exe name and args */
    char **argv = new char *[argSize];
    size_t argIndex = 1; // firts index is reserved for cmd, so it starts with 1
    // argv[0] is program name
    // copy args
    auto cmdSize = cmd.size() + 1; // +1 for NUL terminal
    argv[0] = new char[cmdSize];
    errno_t errSafe = strncpy_s(argv[0], cmdSize, cmd.c_str(), cmdSize); // c_str includes NUL terminal
    CHECK_FATAL(errSafe == EOK, "strncpy_s failed");
    /* Allocate and fill all arguments */
    for (auto &opt : options) {
      auto key = opt.GetKey();
      auto val = opt.GetValue();
      /* +1 for NUL terminal */
      auto keySize = key.size() + 1;
      auto valSize = val.size() + 1;

      if (keySize != 1) {
        argv[argIndex] = new char[keySize];
        errSafe = strncpy_s(argv[argIndex], keySize, key.c_str(), keySize);
        CHECK_FATAL(errSafe == EOK, "strncpy_s failed");
        ++argIndex;
      }

      if (valSize != 1) {
        argv[argIndex] = new char[valSize];
        errSafe = strncpy_s(argv[argIndex], valSize, val.c_str(), valSize);
        CHECK_FATAL(errSafe == EOK, "strncpy_s failed");
        ++argIndex;
      }
    }

    if (compileeFlag == Compilee::kGcc) {
      for (auto &opt : maplecl::CommandLine::GetCommandLine().GetLinkOptions()) {
        auto keySize = opt.size() + 1;
        if (keySize != 1) {
          argv[argIndex] = new char[keySize];
          errSafe = strncpy_s(argv[argIndex], keySize, opt.c_str(), keySize);
          CHECK_FATAL(errSafe == EOK, "strncpy_s failed");
          ++argIndex;
        }
      }
    }

    // end of arguments sentinel is nullptr
    argv[argIndex] = nullptr;
    return std::make_tuple(argv, argIndex);
  }
};
} // namespace maple
#endif // MAPLE_DRIVER_INCLUDE_SAFE_EXE_H
