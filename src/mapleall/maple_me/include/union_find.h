/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_UNION_FIND_H
#define MAPLE_ME_INCLUDE_UNION_FIND_H
#include "mempool.h"
#include "mempool_allocator.h"
#include "mpl_logging.h"
#include "types_def.h"

namespace maple {
// This uses the Weighted Quick Union with Path Compression algorithm.
// Build a flattened tree to represent each class, where all its class members
// are linked together via the tree. The tree is represented by child pointing
// to its parent only. The members of the same class are identified at any time
// by having the same root, and the id of its root member can be used as class
// id.
class UnionFind {
 public:
  explicit UnionFind(MemPool &memPool)
      : ufAlloc(&memPool), rootIDs(ufAlloc.Adapter()), sizeOfClass(ufAlloc.Adapter()) {}

  UnionFind(MemPool &memPool, uint32 siz)
      : ufAlloc(&memPool), num(siz), rootIDs(siz, 0, ufAlloc.Adapter()), sizeOfClass(siz, 0, ufAlloc.Adapter()) {
    Reinit();
  }

  ~UnionFind() {
    // for the root id's, the sum of their size should be population size
#if defined(DEBUG) && DEBUG
    size_t sum = 0;
    for (size_t i = 0; i < num; ++i)
      if (rootIDs[i] == i) {
        // it is a root
        sum += sizeOfClass[i];
      }
    ASSERT(sum == num, "Something wrong in UnionFind");
#endif
  }

  void Reinit() {
    for (size_t i = 0; i < num; ++i) {
      rootIDs[i] = i;
      sizeOfClass[i] = 1;
    }
  }

  unsigned int NewMember() {
    rootIDs.push_back(num);  // new member is its own root
    sizeOfClass.push_back(1);
    return ++num;
  }

  unsigned int NewMember(size_t id) {
    if (id + 1 >= rootIDs.size()) {
      size_t oldSize = rootIDs.size();
      uint incNum = static_cast<uint>(id + 2 - rootIDs.size());
      num += incNum;
      rootIDs.insert(rootIDs.end(), incNum, 0);
      sizeOfClass.insert(sizeOfClass.end(), incNum, 1);
      for (size_t i = oldSize; i < rootIDs.size(); ++i) {
        rootIDs[i] = static_cast<uint32>(i);  // new member is its own root
      }
    }
    return num;
  }

  unsigned int Root(size_t i) {
    while (rootIDs[i] != i) {
      rootIDs[i] = rootIDs[rootIDs[i]];  // this compresses the path
      i = rootIDs[i];
    }
    return i;
  }

  bool Find(size_t p, unsigned int q) {
    return Root(p) == Root(q);
  }

  void Union(size_t p, size_t q) {
    unsigned int i = Root(p);
    unsigned int j = Root(q);
    if (i == j) {
      return;
    }
    // construct a balanced tree
    if (sizeOfClass[i] < sizeOfClass[j]) {
      rootIDs[i] = j;
      sizeOfClass[j] += sizeOfClass[i];
    } else {
      rootIDs[j] = i;
      sizeOfClass.at(i) += sizeOfClass.at(j);
    }
  }

  unsigned int GetElementsNumber(size_t i) const {
    ASSERT(i < sizeOfClass.size(), "index out of range");
    return sizeOfClass[i];
  }

  bool SingleMemberClass(unsigned int p) {
    unsigned int i = Root(p);
    ASSERT(i < sizeOfClass.size(), "index out of range");
    return sizeOfClass[i] == 1;
  }

  inline bool Find(size_t id) const {
    return rootIDs.size() > id;
  }

 private:
  MapleAllocator ufAlloc;
  unsigned int num = 0;                     // the population size; can continue to increase
  MapleVector<unsigned int> rootIDs;  // array index is id of each population member;
  // value is id of the root member of its class;
  // the member is a root if its value is itself;
  // as its root changes, will keep updating so as to
  // maintain a flat tree
  MapleVector<unsigned int> sizeOfClass;  // gives number of elements in the tree rooted there
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_UNION_FIND_H
