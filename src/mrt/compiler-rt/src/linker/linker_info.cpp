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
#include "linker/linker_info.h"

namespace maplert {
MUID GetHash(const std::vector<LinkerMFileInfo*> &mplInfos, bool getSolidHash) {
  std::stringstream sstream;
  for (auto mplInfo : mplInfos) {
    MUID &hash = getSolidHash ? mplInfo->hashOfDecouple : mplInfo->hash;
#ifdef USE_64BIT_MUID
    // 8 spaces to 64 bit
    sstream << std::setfill('0') << std::setw(8) << std::hex << hash.data.words[1] <<
        std::setfill('0') << std::setw(8) << std::hex << hash.data.words[0];
#else
    // 16 spaces to 32 bit
    sstream << std::setfill('0') << std::setw(16) << std::hex << hash.data.words[1] <<
        std::setfill('0') << std::setw(16) << std::hex << hash.data.words[0];
#endif // USE_64BIT_MUID
  }
  auto str = sstream.str();
  MUID muid;
  linkerutils::GenerateMUID(str.data(), str.size(), muid);
  return muid;
}
} // namespace maplert
