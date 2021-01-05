/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mempool.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "securec.h"
#include "mpl_logging.h"

namespace maple {
MemPoolCtrler memPoolCtrler;
bool MemPoolCtrler::freeMemInTime = false;

// Destructor, free all allocated memory pool and blocks
MemPoolCtrler::~MemPoolCtrler() {
  // Delete Memory Pool
  for (MemPool *memPool : memPools) {
    delete memPool;
  }
  // Delete all free_memory_block
  for (MemBlock *block : freeMemBlocks) {
    free(block);
  }
  for (auto it = largeFreeMemBlocks.begin(); it != largeFreeMemBlocks.end(); ++it) {
    for (auto itr = (*it).second.begin(); itr != (*it).second.end(); ++itr) {
      free(*itr);
    }
  }
}

// Allocate a new memory pool and register it in controller
MemPool *MemPoolCtrler::NewMemPool(const std::string &name) {
  auto memPool = new (std::nothrow) MemPool(*this, name);
  if (memPool == nullptr) {
    CHECK_FATAL(false, "ERROR: Can't allocate new memory pool");
    return nullptr;
  }
  memPools.insert(memPool);
  return memPool;
}

// Re-cycle all memory blocks allocated on a memory pool to free list
void MemPoolCtrler::DeleteMemPool(MemPool *memPool) {
  CHECK_NULL_FATAL(memPool);
  // Transfer memory blocks to ctrler->freeMemBlocks stack
  while (!memPool->memBlockStack.empty()) {
    MemBlock *mb = memPool->memBlockStack.top();
    mb->available = mb->origSize;
    mb->ptr = MEM_BLOCK_FIRST_PTR(mb);
    freeMemBlocks.push_back(mb);
    memPool->memBlockStack.pop();
  }
  // Transfer large memory blocks to ctrler->freeMemBlocks stack
  while (!memPool->largeMemBlockStack.empty()) {
    MemBlock *mb = memPool->largeMemBlockStack.top();
    mb->available = mb->origSize;
    mb->ptr = MEM_BLOCK_FIRST_PTR(mb);
    size_t key = (mb->available / 0x800) + 1; // Minimum BlockSize is 2K.
    if (largeFreeMemBlocks.find(key) == largeFreeMemBlocks.end()) {
      largeFreeMemBlocks[key] = std::set<MemBlock*, MemBlockCmp>();
    }
    largeFreeMemBlocks[key].insert(mb);
    memPool->largeMemBlockStack.pop();
  }
  // Delete entry of this mempool in memPools and delete the mempool node
  memPools.erase(memPool);
  delete memPool;
  if (freeMemInTime) {
    FreeMem();
  }
}

void MemPoolCtrler::FreeMem() {
  for (MemBlock *block : freeMemBlocks) {
    free(block);
  }
  for (auto it = largeFreeMemBlocks.begin(); it != largeFreeMemBlocks.end(); ++it) {
    for (auto itr = (*it).second.begin(); itr != (*it).second.end(); ++itr) {
      free(*itr);
    }
  }
  freeMemBlocks.clear();
  largeFreeMemBlocks.clear();
}

MemPool::~MemPool() {
  while (!memBlockStack.empty()) {
    free(memBlockStack.top());
    memBlockStack.pop();
  }
  while (!largeMemBlockStack.empty()) {
    free(largeMemBlockStack.top());
    largeMemBlockStack.pop();
  }
}

// Return a pointer that points to size of memory from memory block
void *MemPool::Malloc(size_t size) {
  void *result = nullptr;
  MemPoolCtrler::MemBlock *b = nullptr;
  // If size is smaller than 2K, fetch size of memory from the last memory block
  if (size <= kMinBlockSize) {
    // Get the top memory block from the top
    b = (memBlockStack.empty() ? nullptr : memBlockStack.top());
    // the last operation was a push
    if (b == nullptr || b->available < size) {
      b = GetMemBlock();
    }
    // Return the pointer that points to the starting point + 8;
  } else {
    b = GetLargeMemBlock(size);
  }
  CHECK_FATAL(b != nullptr, "ERROR: Malloc error");
  result = static_cast<void*>(b->ptr);
  b->ptr = static_cast<void*>(static_cast<char*>(b->ptr) + size);
  // available size decrease
  int tmp = b->available - size;
  CHECK_FATAL(tmp >= 0, "ERROR: Malloc error");
  b->available = tmp;
  return result;
}

// Malloc size of memory from memory pool, then set 0
void *MemPool::Calloc(size_t size) {
  void *p = Malloc(BITS_ALIGN(size));
  if (p == nullptr) {
    CHECK_FATAL(false, "ERROR: Calloc error\n");
    return nullptr;
  }
  errno_t eNum = memset_s(p, BITS_ALIGN(size), 0, BITS_ALIGN(size));
  if (eNum != EOK) {
    CHECK_FATAL(false, "memset_s failed");
  }
  return p;
}

// Realloc new size of memory
void *MemPool::Realloc(const void *ptr, size_t oldSize, size_t newSize) {
  void *result = Malloc(newSize);
  if (result != nullptr) {
    size_t copySize = ((newSize > oldSize) ? oldSize : newSize);
    if (copySize != 0 && ptr != nullptr) {
      errno_t eNum = memcpy_s(result, copySize, ptr, copySize);
      if (eNum != EOK) {
        FATAL(kLncFatal, "memcpy_s failed");
      }
    }
  } else {
    CHECK_FATAL(false, "Error Realloc\n");
  }
  return result;
}

void MemPool::ReleaseContainingMem() {
  while (!memBlockStack.empty()) {
    MemPoolCtrler::MemBlock *block1 = memBlockStack.top();
    block1->available = block1->origSize;
    block1->ptr = MEM_BLOCK_FIRST_PTR(block1);
    ctrler.freeMemBlocks.push_back(block1);
    memBlockStack.pop();
  }
  while (!largeMemBlockStack.empty()) {
    MemPoolCtrler::MemBlock *block2 = largeMemBlockStack.top();
    block2->available = block2->origSize;
    block2->ptr = MEM_BLOCK_FIRST_PTR(block2);
    size_t key = (block2->available / 0x800) + 1; // Minimum BlockSize is 2K.
    if (ctrler.largeFreeMemBlocks.find(key) == ctrler.largeFreeMemBlocks.end()) {
      std::set<MemPoolCtrler::MemBlock*, MemPoolCtrler::MemBlockCmp> tmp;
      ctrler.largeFreeMemBlocks[key] = tmp;
    }
    ctrler.largeFreeMemBlocks[key].insert(block2);
    largeMemBlockStack.pop();
  }
}

// Allocate a new memory block
MemPoolCtrler::MemBlock *MemPool::GetMemBlock() {
  MemPoolCtrler::MemBlock *block = nullptr;
  // Try to fetch one from free_memory_list
  if (!ctrler.freeMemBlocks.empty()) {
    block = ctrler.freeMemBlocks.front();
    ctrler.freeMemBlocks.erase(ctrler.freeMemBlocks.begin());
  } else {
    // Allocate a block of memory, 2K + MemBlock head size
    size_t total = kMinBlockSize + kMemBlockOverhead;
    block = static_cast<MemPoolCtrler::MemBlock*>(malloc(total));
    if (block == nullptr) {
      CHECK_FATAL(false, "ERROR: Allocate memory block failed\n");
      return nullptr;
    }
    // Available memory size is BlockSize
    block->available = kMinBlockSize;
    block->origSize = kMinBlockSize;
    // Set the pointer points to the first available byte
    block->ptr = MEM_BLOCK_FIRST_PTR(block);
  }
  memBlockStack.push(block);
  return block;
}

// Allocate a large memory block when user allocates memory size > 2K
MemPoolCtrler::MemBlock *MemPool::GetLargeMemBlock(size_t size) {
  MemPoolCtrler::MemBlock *block = nullptr;
  size_t key = (size / kMinBlockSize) + 1;
  if (ctrler.largeFreeMemBlocks.find(key) != ctrler.largeFreeMemBlocks.end() &&
      ctrler.largeFreeMemBlocks[key].size()) {
    block = *(ctrler.largeFreeMemBlocks[key].begin());
    if (block->origSize >= size) {
      ctrler.largeFreeMemBlocks[key].erase(ctrler.largeFreeMemBlocks[key].begin());
    } else {
      block = nullptr;
    }
  }
  if (block == nullptr) {
    // Allocate a large memory block
    block = static_cast<MemPoolCtrler::MemBlock*>(malloc(size + kMemBlockOverhead));
    if (block == nullptr) {
      CHECK_FATAL(false, "ERROR: Fail to allocate large memory block\n");
      return nullptr;
    }
    block->origSize = size;
    block->available = size;
    block->ptr = MEM_BLOCK_FIRST_PTR(block);
  }
  largeMemBlockStack.push(block);
  return block;
}

}  // namespace maple
