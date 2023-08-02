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
#include "mempool.h"
#include "thread_env.h"
#include "securec.h"
#include "mpl_logging.h"

namespace maple {
MemPoolCtrler memPoolCtrler;
bool MemPoolCtrler::freeMemInTime = false;

size_t BitsAlign(size_t size) {
  size_t kAlign8FillSize = 7;
  return ((size) + kAlign8FillSize) & (0xFFFFFFF8);
}

void MemPoolCtrler::FreeMemBlocks(const MemPool &pool, MemBlock *fixedMemHead, MemBlock *bigMemHead) {
  (void)(pool);

  MemBlock *fixedTail = nullptr;

  if (fixedMemHead != nullptr) {
    fixedTail = fixedMemHead;
    while (fixedTail->nextMemBlock != nullptr) {
      fixedTail = fixedTail->nextMemBlock;
    }
  }

  while (bigMemHead != nullptr) {
    auto *cur = bigMemHead;
    bigMemHead = bigMemHead->nextMemBlock;
    cur->~MemBlock();
    free(cur);
  }

  ParallelGuard guard(ctrlerMutex, HaveRace());
  if (fixedTail != nullptr) {
    fixedTail->nextMemBlock = fixedFreeMemBlocks;
    ASSERT(fixedTail->nextMemBlock != fixedTail, "error");
    fixedFreeMemBlocks = fixedMemHead;
  }
}

// Destructor, free all allocated memories
MemPoolCtrler::~MemPoolCtrler() {
  FreeMem();
}

// Allocate a new memory pool and register it in controller
MemPool *MemPoolCtrler::NewMemPool(const std::string &name, bool isLocalPool) {
  MemPool *memPool = nullptr;

  if (isLocalPool) {
    memPool = new ThreadLocalMemPool(*this, name);
  } else {
    memPool = new ThreadShareMemPool(*this, name);
  }

  return memPool;
}

// This function will be removed soon, DO NOT call it, just use delete memPool
void MemPoolCtrler::DeleteMemPool(MemPool *memPool) const {
  delete memPool;
}

void MemPoolCtrler::FreeMem() {
  ParallelGuard guard(ctrlerMutex, HaveRace());

  while (fixedFreeMemBlocks != nullptr) {
    MemBlock *arena = fixedFreeMemBlocks;
    fixedFreeMemBlocks = fixedFreeMemBlocks->nextMemBlock;
    arena->~MemBlock();
  }
}

MemBlock *MemPoolCtrler::AllocMemBlock(const MemPool &pool, size_t size) {
  if (size <= kMemBlockSizeMin) {
    return AllocFixMemBlock(pool);
  } else {
    return AllocBigMemBlock(pool, size);
  }
}

MemBlock *MemPoolCtrler::AllocFixMemBlock(const MemPool &pool) {
  (void)(pool);
  ParallelGuard guard(ctrlerMutex, HaveRace());
  if (fixedFreeMemBlocks == nullptr) {
    uint8_t *ptr = sysMemoryMgr->RealAllocMemory(kMemBlockRealMallocSize);
    // leave one MemBlock to return
    for (size_t i = 0; i < kMemBlockRealMallocSize / kMemBlockMallocSize; ++i) {
      auto *block = new (ptr) MemBlock(ptr + kMemBlockStructSize, kMemBlockSizeMin);
      ptr += kMemBlockMallocSize;
      block->nextMemBlock = fixedFreeMemBlocks;
      fixedFreeMemBlocks = block;
    }
  }

  MemBlock *ret = fixedFreeMemBlocks;
  fixedFreeMemBlocks = fixedFreeMemBlocks->nextMemBlock;
  return ret;
}

MemBlock *MemPoolCtrler::AllocBigMemBlock(const MemPool &pool, size_t size) const {
  ASSERT(size > kMemBlockSizeMin, "Big memory block must be bigger than fixed memory block");
  (void)(pool);

  uint8_t *block = reinterpret_cast<uint8_t*>(malloc(size + kMemBlockStructSize));
  CHECK_FATAL(block != nullptr, "malloc failed");
  return new (block) MemBlock(block + kMemBlockStructSize, size);
}

MemPool::~MemPool() {
  ctrler.FreeMemBlocks(*this, fixedMemHead, bigMemHead);
}

void *MemPool::Malloc(size_t size) {
  size = BitsAlign(size);
  ASSERT(endPtr >= curPtr, "endPtr should >= curPtr");
  if (size > static_cast<size_t>(endPtr - curPtr)) {
    return AllocNewMemBlock(size);
  }
  uint8_t *retPtr = curPtr;
  curPtr += size;
  return retPtr;
}

void MemPool::ReleaseContainingMem() {
  ctrler.FreeMemBlocks(*this, fixedMemHead, bigMemHead);

  fixedMemHead = nullptr;
  bigMemHead = nullptr;
  endPtr = nullptr;
  curPtr = nullptr;
}

// Malloc size of memory from memory pool, then set 0
void *MemPool::Calloc(size_t size) {
  void *p = Malloc(BitsAlign(size));
  ASSERT(p != nullptr, "ERROR: Calloc error");
  errno_t eNum = memset_s(p, BitsAlign(size), 0, BitsAlign(size));
  CHECK_FATAL(eNum == EOK, "memset_s failed");
  return p;
}

// Realloc new size of memory
void *MemPool::Realloc(const void *ptr, size_t oldSize, size_t newSize) {
  void *result = Malloc(newSize);
  ASSERT(result != nullptr, "ERROR: Realloc error");
  size_t copySize = ((newSize > oldSize) ? oldSize : newSize);
  if (copySize != 0 && ptr != nullptr) {
    errno_t eNum = memcpy_s(result, copySize, ptr, copySize);
    CHECK_FATAL(eNum == EOK, "memcpy_s failed");
  }
  return result;
}

uint8_t *MemPool::AllocNewMemBlock(size_t byteSize) {
  MemBlock **head = nullptr;
  MemBlock *newMemBlock = ctrler.AllocMemBlock(*this, byteSize);
  if (newMemBlock->memSize <= kMemBlockSizeMin) {
    head = &fixedMemHead;
  } else {
    head = &bigMemHead;
  }

  newMemBlock->nextMemBlock = *head;
  *head = newMemBlock;
  CHECK_FATAL(newMemBlock->nextMemBlock != newMemBlock, "error");

  curPtr = newMemBlock->startPtr + byteSize;
  endPtr = newMemBlock->startPtr + newMemBlock->memSize;
  ASSERT(curPtr <= endPtr, "must be");

  return newMemBlock->startPtr;
}

void *StackMemPool::Malloc(size_t size) {
  size = BitsAlign(size);
  uint8_t **curPtrPtr = nullptr;
  uint8_t *curEndPtr = nullptr;
  if (size <= kMemBlockSizeMin) {
    curPtrPtr = &curPtr;
    curEndPtr = endPtr;
  } else {
    curPtrPtr = &bigCurPtr;
    curEndPtr = bigEndPtr;
  }
  uint8_t *retPtr = *curPtrPtr;
  ASSERT(curEndPtr >= *curPtrPtr, "endPtr should >= curPtr");
  if (size > static_cast<size_t>(curEndPtr - *curPtrPtr)) {
    retPtr = AllocTailMemBlock(size);
  }
  *curPtrPtr = retPtr + size;
  return retPtr;
}

// scoped mem pool don't use big mem block for small size, different with normal mempool
MemBlock *StackMemPool::AllocMemBlockBySize(size_t size) {
  if (size <= kMemBlockSizeMin) {
    return ctrler.AllocFixMemBlock(*this);
  } else {
    return ctrler.AllocBigMemBlock(*this, size);
  }
}

void StackMemPool::ResetStackTop(const LocalMapleAllocator *alloc, uint8_t *fixedCurPtrMark,
                                 MemBlock *fixedStackTopMark, uint8_t *bigCurPtrMark,
                                 MemBlock *bigStackTopMark) noexcept {
  CheckTopAllocator(alloc);
  PopAllocator();

  if (fixedStackTopMark != nullptr) {
    fixedMemStackTop = fixedStackTopMark;
    curPtr = fixedCurPtrMark;
    endPtr = fixedMemStackTop->EndPtr();
  } else if (fixedMemHead != nullptr) {
    fixedMemStackTop = fixedMemHead;
    curPtr = fixedMemStackTop->startPtr;
    endPtr = fixedMemStackTop->EndPtr();
  }

  if (bigStackTopMark != nullptr) {
    bigMemStackTop = bigStackTopMark;
    bigCurPtr = bigCurPtrMark;
    bigEndPtr = bigMemStackTop->EndPtr();
  } else if (bigMemHead != nullptr) {
    bigMemStackTop = bigMemHead;
    bigCurPtr = bigMemStackTop->startPtr;
    bigEndPtr = bigMemStackTop->EndPtr();
  }
}

uint8_t *StackMemPool::AllocTailMemBlock(size_t size) {
  MemBlock **head = nullptr;
  MemBlock **stackTop = nullptr;
  uint8_t **endPtrPtr = nullptr;

  if (size <= kMemBlockSizeMin) {
    head = &fixedMemHead;
    stackTop = &fixedMemStackTop;
    endPtrPtr = &endPtr;
  } else {
    head = &bigMemHead;
    stackTop = &bigMemStackTop;
    endPtrPtr = &bigEndPtr;
  }

  if (*stackTop == nullptr) {
    MemBlock *newMemBlock = AllocMemBlockBySize(size);
    *stackTop = newMemBlock;
    *head = newMemBlock;
    (*stackTop)->nextMemBlock = nullptr;
  } else {
    if ((*stackTop)->nextMemBlock != nullptr && (*stackTop)->nextMemBlock->memSize >= size) {
      *stackTop = (*stackTop)->nextMemBlock;
    } else {
      MemBlock *newMemBlock = AllocMemBlockBySize(size);
      auto *tmp = (*stackTop)->nextMemBlock;
      (*stackTop)->nextMemBlock = newMemBlock;
      *stackTop = newMemBlock;
      newMemBlock->nextMemBlock = tmp;
    }
  }
  *endPtrPtr = (*stackTop)->EndPtr();
  return (*stackTop)->startPtr;
}
}  // namespace maple
