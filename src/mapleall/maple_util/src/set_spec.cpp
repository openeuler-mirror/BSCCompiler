/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <cstring>

#include "set_spec.h"
#include "securec.h"
#include "driver_options.h"

namespace maple {

std::vector<SpecList *> SetSpec::specs;
std::deque<std::string> SetSpec::args;

static SpecList cc1({"cc1", 3, nullptr});
static SpecList cppOptions({"cpp_options", 11, nullptr});


// Nonzero means %s has been seen; the next arg to be terminated is the name of a library file and
// we should try the standard search dirs for it.
static int thisIsLibraryFile;

// Nonzero if an arg has been started and not yet terminated (with space, tab or newline).
static bool argGoing = false;

void SetSpec::SetUpSpecs(std::deque<std::string_view> &arg, const std::string &specsFile) {
  if (specsFile == "") {
    return;
  }
  InitSpec();
  ReadSpec(specsFile);
  args.clear();
  // 2 is speclist original length
  for (size_t i = 0; i < 2; i++) {
    if (specs[i]->value != nullptr) {
      DoSpecs(specs[i]->value);
    }
  }
  for (int index = static_cast<int>(args.size() - 1); index >= 0; index--) {
    std::string_view tmp(args[index]);
    arg.emplace_front(tmp);
  }
}

void SetSpec::InitSpec() {
    SetSpec::specs.push_back(&cc1);
    SetSpec::specs.push_back(&cppOptions);
}

char* SkipWhitespace(char *p) {
  for (;;) {
    // A fully-blank line is a delimiter in the SPEC file and shouldn't be considered whitespace.
    // 2 is index of one line
    if (p[0] == '\n' && p[1] == '\n' && p[2] == '\n') {
      return p + 1;
    } else if (*p == '\n' || *p == ' ' || *p == '\t') {
      p++;
    } else if (*p == '#') {
      while (*p != '\n') {
        p++;
      }
      p++;
    } else {
      break;
    }
  }
  return p;
}

bool StartsWith(const char *str, const char *prefix) {
  return strncmp(str, prefix, strlen (prefix)) == 0;
}

char* SaveString(const char *s, size_t len) {
  char *result = FileUtils::GetInstance().GetMemPool().NewArray<char>(len + 1);
  errno_t err = memcpy_s(result, len, s, len);
  CHECK_FATAL(err == EOK, "memcpy_s failed");
  result[len] = 0;
  return result;
}

void SetSpec::ReadSpec(const std::string &specsFile) {
  char *buffer;
  char *p;
  buffer = FileUtils::LoadFile(specsFile.c_str());
  p = buffer;
  for (;;) {
    char *suffix;
    char *spec;
    char *in, *out, *p1, *p2, *p3;
    p = SkipWhitespace(p);
    if (*p == 0) {
      break;
    }
    if (*p == '%') {
      p1 = p;
      while (*p != '\0' && *p != '\n') {
        p++;
      }
      // Skip '\n'.
      p++;
      if (StartsWith(p1, "%include") && (p1[sizeof "%include" - 1] == ' ' ||
          p1[sizeof "%include" - 1] == '\t')) {
        CHECK_FATAL(false, "Currently does not support parsing %include.\n");
      } else if (StartsWith(p1, "%include_noerr") && (p1[sizeof "%include_noerr" - 1] == ' ' ||
                 p1[sizeof "%include_noerr" - 1] == '\t')) {
        CHECK_FATAL(false, "Currently does not support parsing %include_noerr.\n");
      } else if (StartsWith(p1, "%rename") && (p1[sizeof "%rename" - 1] == ' ' ||
                 p1[sizeof "%rename" - 1] == '\t')) {
        int nameLen;
        struct SpecList *sl;
        // Get original name.
        p1 += sizeof "%rename";
        while (*p1 == ' ' || *p1 == '\t') {
          p1++;
        }
        if (!isalpha(static_cast<char>(*p1))) {
          CHECK_FATAL(false, "specs %%rename syntax malformed after %ld characters.\n", static_cast<long>(p1 - buffer));
        }
        p2 = p1;
        while (*p2 != '\0' && !isspace(static_cast<char>(*p2))) {
          p2++;
        }
        if (*p2 != ' ' && *p2 != '\t') {
          CHECK_FATAL(false, "specs %%rename syntax malformed after %ld characters.\n", static_cast<long>(p2 - buffer));
        }
        nameLen = static_cast<int>(p2 - p1);
        *p2++ = '\0';
        while (*p2 == ' ' || *p2 == '\t') {
          p2++;
        }
        if (!isalpha(static_cast<char>(*p2))) {
          CHECK_FATAL(false, "specs %%rename syntax malformed after %ld characters.\n", static_cast<long>(p2 - buffer));
        }
        /* Get new spec name.  */
        p3 = p2;
        while (*p3 != '\0' && !isspace(static_cast<char>(*p3))) {
          p3++;
        }
        if (p3 != p - 1) {
          CHECK_FATAL(false, "specs %%rename syntax malformed after %ld characters.\n", static_cast<long>(p3 - buffer));
        }
        *p3 = '\0';
        for (auto sl1 : specs) {
          if (nameLen == sl1->nameLen && strcmp(sl1->name, p1) == 0) {
            sl = sl1;
            break;
          }
        }
        if (!sl) {
          CHECK_FATAL(false, "specs %s spec was not found to be renamed.\n", p1);
        }
        if (strcmp(p1, p2) == 0) {
          continue;
        }
        for (auto newsl : specs) {
          if (strcmp(newsl->name, p2) == 0) {
            CHECK_FATAL(false, "Attempt to rename spec %qs to  already defined spec %qs.\n", p1, p2);
          }
        }
        SetSpecs(p2, sl->value);
        continue;
      }
    }
    p1 = p;
    while (*p1 != '\0' && *p1 != ':' && *p1 != '\n') {
      p1++;
    }
    if (*p1 != ':') {
      CHECK_FATAL(false, "specs file malformed after %ld characters.\n", static_cast<long>(p1 - buffer));
    }
    p2 = p1;
    while (p2 > buffer && (p2[-1] == ' ' || p2[-1] == '\t')) {
        p2--;
    }
    suffix = SaveString(p, static_cast<size_t>(p2 - p));
    std::string tmpSuffix(suffix);
    p = SkipWhitespace(p1 + 1);
    if (p[1] == 0) {
      CHECK_FATAL(false, "specs file malformed after %ld characters.\n", static_cast<long>(p - buffer));
    }
    p1 = p;
    while (*p1 != '\0' && !(*p1 == '\n' && (p1[1] == '\n' || p1[1] == '\0'))) {
      p1++;
    }
    if ((p1 - p) > 0) {
      spec = SaveString(p, static_cast<size_t>(p1 - p));
      in = spec;
      out = spec;
      while (*in != 0) {
        if (in[0] == '\\' && in[1] == '\n') {
          in += 2; // 2 is skip "\\" or "\n"
        } else if (in[0] == '#') {
          while (*in != '\0' && *in != '\n') {
            in++;
          }
        } else {
          *out++ = *in++;
        }
      }
      *out = 0;
    }
    p = p1;
    if (tmpSuffix == "*cpp_options" || tmpSuffix == "*cc1") {
      SetSpecs(suffix + 1, spec);
    }
  }
}

void SetSpec::SetSpecs(const char *name, const char *spec) {
  struct SpecList *sl;
  int nameLen = static_cast<int>(strlen(name));

  // See if the spec already exists.
  for (auto sl1 : specs) {
    if (nameLen == sl1->nameLen && strcmp(sl1->name, name) == 0) {
      sl = sl1;
      break;
    }
  }

  if (strcmp(sl->name, "") == 0) {
    // Not found - make it.
    sl->name = name;
    sl->nameLen = nameLen;
    sl->value = spec;
    specs.push_back(sl);
  }

  sl->value = spec;
}

std::string SetSpec::FindPath(const std::string tmpArg) {
  std::string result = "";
  std::string cmd = "";
  std::vector<std::string> libPathVec;
  std::vector<std::string> resVec;

  if (FileUtils::SafeGetenv(kGccLibPath) != "") {
    StringUtils::Split(FileUtils::SafeGetenv(kGccLibPath), libPathVec, ':');
  }
  if (opts::sysRoot.IsEnabledByUser()) {
    libPathVec.push_back(opts::sysRoot.GetValue() + "/lib/");
    libPathVec.push_back(opts::sysRoot.GetValue() + "/usr/lib/");
  }
  for (auto libPath : libPathVec) {
    cmd = "ls " + libPath + " | grep " + tmpArg;
    result = FileUtils::ExecuteShell(cmd.c_str());
    if (result != "") {
      StringUtils::Split(result, resVec, '\n');
      return libPath + resVec[0];
    }
  }
  return tmpArg;
}

void SetSpec::EndGoingArg(const std::string tmpArg) {
  if (argGoing != 0) {
    if (thisIsLibraryFile == 1) {
      std::string path = FindPath(tmpArg);
      args.push_back(path);
    } else {
      args.push_back(tmpArg);
    }
  }
}

void SetSpec::DoSpecs(const char *spec) {
  const char *p = spec;
  int c;
  std::string tmpArg = "";

  while ((c = *p++)) {
    switch (c) {
      case '\n':
      case '\t':
      case ' ':
        EndGoingArg(tmpArg);
        tmpArg = "";
        argGoing = false;
        thisIsLibraryFile = 0;
        break;
      case '%':
        switch (c = *p++) {
          case 's':
            thisIsLibraryFile = 1;
            break;
          case '(':
          {
            const char *name = p;
            size_t len;
            while (*p != '\0' && *p != ')') {
              p++;
            }
            len = static_cast<size_t>(p - name);
            for (auto sl1 : specs) {
              if (sl1->nameLen == static_cast<int>(len) && strncmp(sl1->name, name, len) == 0) {
                name = sl1->value;
                break;
              }
            }
            if (*p) {
              p++;
            }
            break;
          }
          default:
            CHECK_FATAL(false, "spec failure: unrecognized spec option %qc", c);
        }
        break;
      default:
        std::string tmp(1, c);
        tmpArg += tmp;
        argGoing = true;
    }
  }
  if (tmpArg != "") {
    EndGoingArg(tmpArg);
    tmpArg = "";
    argGoing = false;
    thisIsLibraryFile = 0;
  }
}

}  // namespace maple