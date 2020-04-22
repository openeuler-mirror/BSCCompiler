/*
* Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
*
* OpenArkFE is licensed under the Mulan PSL v1.
* You can use this software according to the terms and conditions of the Mulan PSL v1.
* You may obtain a copy of Mulan PSL v1 at:
*
*  http://license.coscl.org.cn/MulanPSL
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
* FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v1 for more details.
*/

///////////////////////////////////////////////////////////////////////////////
// This file contains the popular container for most data structures.
//
// [NOTE] For containers, there is one basic rule that is THE CONTAINTER ITSELF
//        SHOULD BE SELF CONTAINED. In another word, all the memory should be
//        contained in the container itself. Once the destructor or Release()
//        is called, all memory should be freed.
//
// The user of containers need explictly call Release() to free
// the memory, if he won't trigger the destructor of the containers. This
// is actually common case, like tree nodes which is maintained by a memory
// pool and won't call destructor, so any memory allocated at runtime won't
// be released.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include "mempool.h"
#include "massert.h"

// We define a ContainerMemPool, which is slightly different than the other MemPool.
// There are two major differences.
// (1) Size of each allocation will be the same.
// (2) Support indexing.

class ContainerMemPool : public MemPool {
public:
  unsigned mElemSize;
public:
  char* AddrOfIndex(unsigned index);
  void  SetElemSize(unsigned i) {mElemSize = i;}
  char* AllocElem() {return Alloc(mElemSize);}
};

// SmallVector is widely used in the tree nodes to save children nodes, and
// they should call Release() explicitly, if the destructor of SmallVector
// won't be called.

// NOTE: When we locate an element in the memory pool, we don't check if it's
//       out of boundary. It's the user of SmallVector to ensure element index
//       is valid.

template <class T> class SmallVector {
private:
  ContainerMemPool mMemPool;
  unsigned         mNum;     // element number

public:
  SmallVector() {
    mNum = 0;
    SetBlockSize(128);
    mMemPool.SetElemSize(sizeof(T));
  }
  ~SmallVector() {Release();}

  void SetBlockSize(unsigned i) {mMemPool.SetBlockSize(i);}
  void Release() {mMemPool.Release();}

  void PushBack(T t) {
    char *addr = mMemPool.AllocElem();
    *(T*)addr = t;
    mNum++;
  }

  void PopBack(T);

  unsigned GetNum() {return mNum;}

  T Back();

  T ValueAtIndex(unsigned i) {
    char *addr = mMemPool.AddrOfIndex(i);
    return *(T*)addr;
  }

  T* RefAtIndex(unsigned i) {
    char *addr = mMemPool.AddrOfIndex(i);
    return (T*)addr;
  }

  // It's the caller's duty to make sure ith element is valid.
  void SetElem(unsigned i, T t) {
    char *addr = mMemPool.AddrOfIndex(i);
    *(T*)addr = t;
  }

  bool Find(T t) {
    for (unsigned i = 0; i < mNum; i++) {
      T temp = ValueAtIndex(i);
      if (temp == t)
        return true;
    }
    return false;
  }

  // clear the data, but keep the memory, no free.
  void Clear(){
    mNum = 0;
    mMemPool.Clear();
  }
};

// The reason we need a SmallList is trying to provide a structure
// easy for delete and insert element, and also easy for access
// through index. The indexing is currently through traversing element
// from the head (index 0). This is acceptable since it's a 'Small'
// list.
//
// [NOTE] Again, as SmallVector it's the user's duty to assure the
//        element indexing is not out of boundary.
//
// SmallList provides a mechanism of insertion, the usage pattern is as
// below.
//    SmallList list;
//    list.LocateValue(T existingvalue), or LocateIndex(index);
//    list.InsertBefore(T newvalue)
//    list.InsertAfter(T newvalue)
//
// No matter what the operations behind list.LocateXXX are, the position
// always at the 'located' element. Please make sure
// there are no duplicated value, or else it always locate the first
// one.

template <class T> class SmallList {
private:
  struct Elem {
    T mData;
    Elem *mNext;
    Elem *mPrev;
  };

private:
  ContainerMemPool mMemPool;
  unsigned mNum;     // element number
  Elem    *mHead;    // index = 0
  Elem    *mTail;    // index = mNum - 1
  Elem    *mPointer; // temp pointer
private:
  Elem* NewElem(T t) {
    char *addr = mMemPool.AllocElem();
    Elem *elem = (Elem*)addr;
    elem->mData = t;
    elem->mNext = NULL;
    elem->mPrev = NULL;
    mNum++;
    return elem;
  }

  Elem* ElemAtIndex(unsigned i) {
    unsigned idx = 0;
    Elem *temp_elem = mHead;
    while (temp_elem) {
      if (idx == i)
        break;
      idx++;
      temp_elem = temp_elem->mNext;
    }
    return temp_elem;
  }

public:
  SmallList() {
    mHead = NULL;
    mTail = NULL;
    mPointer = NULL;
    mNum = 0;
    SetBlockSize(256);
    mMemPool.SetElemSize(sizeof(Elem));
  }
  ~SmallList() {Release();}

  void SetBlockSize(unsigned i) {mMemPool.SetBlockSize(i);}
  void Release() {mMemPool.Release();}

  void PushBack(T t) {
    Elem *e = NewElem(t);
    if (mTail) {
      mTail->mNext = e;
      e->mPrev = mTail;
    } else {
      MASSERT(!mHead);
      mHead = e;
    }
    mTail = e;
  }

  void PushFront(T t) {
    Elem *e = NewElem(t);
    e->mNext = mHead;
    if (mHead) {
      mHead->mPrev = e;
    } else {
      MASSERT(!mTail);
      mTail = e;
    }
    mHead = e;
  }

  unsigned GetNum() {return mNum;}

  // Caller's duty to assure Back() has existing element.
  T Back() {return mTail->mData;}

  // Caller's duty to assure Front() has existing element.
  T Front() {return mHead->mData;}

  T ValueAtIndex(unsigned i) {
    Elem *e = ElemAtIndex(i);
    return e->mData;
  }

  T* RefAtIndex(unsigned i) {
    Elem *e = ElemAtIndex(i);
    return &(e->mData);
  }

  bool Find(T t) {
    Elem *temp = mHead;
    while (temp) {
      if (temp->mData == t)
        return true;
      temp = temp->mNext;
    }
    return false;
  }

  // The following three functions are used together, with a leading
  // Locate() followed by any number of InsertBefore() or InsertAfter().
  // Keep in mind, the pointer always point to the one by Locate().

  // It's caller's duty to make sure 't' really exists.
  void LocateValue(T t) {
    Elem *temp = mHead;
    while (temp) {
      if (temp->mData == t) {
        mPointer = temp;
        return;
      }
      temp = temp->mNext;
    }
  }

  void LocateIndex(unsigned idx) {
    mPointer = ElemAtIndex(idx);
  }

  void InsertAfter(T t) {
    Elem *new_elem = NewElem(t);
    MASSERT(mPointer);
    Elem *old_next = mPointer->mNext;
    mPointer->mNext = new_elem;
    new_elem->mNext = old_next;
    new_elem->mPrev = mPointer;
    if (old_next)
      old_next->mPrev = new_elem;
    else {
      MASSERT(mPointer == mTail);
      mTail = new_elem;
    }
  }

  void InsertBefore(T t) {
    Elem *new_elem = NewElem(t);
    MASSERT(mPointer);
    Elem *old_prev = mPointer->mPrev;
    mPointer->mPrev = new_elem;
    new_elem->mNext = mPointer;
    new_elem->mPrev = old_prev;
    if (old_prev)
      old_prev->mNext = new_elem;
    else {
      MASSERT(mPointer = mHead);
      mHead = new_elem;
    }
  }

  // Swap the position of A and B.
  // [NOTE] We assume that A is before B in the list.
  void Swap(Elem *A, Elem *B) {
    MASSERT(A && B);
    MASSERT(A->mNext && B->mPrev);
    Elem *A_prev = A->mPrev;
    Elem *A_next = A->mNext;
    Elem *B_prev = B->mPrev;
    Elem *B_next = B->mNext;

    if (A_prev)
      A_prev->mNext = B;
    if (B_next)
      B_next->mPrev = A;
    B->mPrev = A_prev;
    A->mNext = B_next;

    // If the two nodes are connected, it's easier to handle.
    if (B_prev == A) {
      A->mPrev = B;
      B->mNext = A;
    } else {
      B_prev->mNext = A;
      A->mPrev = B_prev;
      A_next->mPrev = B;
      B->mNext = A_next;
    }

    if (mHead == A)
      mHead = B;
    if (mTail == B)
      mTail = A;
  }

  // Sort all elements in descending order, the largest element
  // having 0th index. [NOTE] The user needs provide the operator <
  // of T.
  void SortDescending() {
    Elem *target = mHead;
    while (target) {
      Elem *peer = target->mNext;
      while (peer) {
        if (target->mData < peer->mData) {
          Swap(target, peer);
          Elem *temp = target;
          target = peer;
          peer = temp;
        }
        peer = peer->mNext;
      }
      target = target->mNext;
    }
  }

  // clear the data, but keep the memory, no free.
  void Clear(){
    mNum = 0;
    mHead = NULL;
    mTail = NULL;
    mMemPool.Clear();
  }
};

/////////////////////////////////////////////////////////////////////////
//                      Guamian
// Guamian, aka Hanging Noodle, represents a 2-D data structure shown below.
//
//  --K--->K--->K--->K-->
//    |    |    |    |
//    E    E    E    E
//    |    |         |
//    E    E         E
//         |
//         E
// The horizontal bar is a one directional linked list. It's like the stick
// of Guamian. Each vertical list is like one noodle string, which is also
// one direction linked list. The node on the stick is called (K)nob, the node
// on the noodle string is called (E)lement.
//
// None of the two lists are sorted since our target scenarios are usually
// at small scope.
//
// Duplication of knobs or elements is not supported in Guamian.
/////////////////////////////////////////////////////////////////////////

template <class K = unsigned, class D = unsigned, class E = unsigned> class Guamian {
private:
  struct Elem{
    E     mData;
    Elem *mNext;
  };

  // Sometimes people need save certain additional information to
  // each knob. So we define mData.
  struct Knob{
    K        mKey;
    D        mData;
    Knob    *mNext;
    Elem    *mChildren; // pointing to the first child
  };

private:
  MemPool mMemPool;
  Knob   *mHeader;

  // allocate a new knob
  Knob* NewKnob() {
    Knob *knob = (Knob*)mMemPool.Alloc(sizeof(Knob));
    knob->mKey = 0;
    knob->mData = 0;
    knob->mNext = NULL;
    knob->mChildren = NULL;
    return knob;
  }

  // allocate a new element
  Elem* NewElem() {
    Elem *elem = (Elem*)mMemPool.Alloc(sizeof(Elem));
    elem->mNext = NULL;
    elem->mData = 0;
    return elem;
  }

  // Sometimes people want to have a sequence of operations like,
  //   Get the knob,
  //   Add one element, on the knob
  //   Add more elements, on the knob.
  // This is common scenario. To implement, it requires a temporary
  // pointer to the located knob. This temp knob is used ONLY when
  // paired operations, PairedFindOrCreateKnob() and PairedAddElem() 
  Knob *mTempKnob;

private:
  // Just try to find the Knob.
  // return NULL if fails.
  Knob* FindKnob(K key) {
    Knob *result = NULL;
    Knob *knob = mHeader;
    while (knob) {
      if (knob->mKey == key) {
        result = knob;
        break;
      }
      knob = knob->mNext;
    }
    return result;
  }

  // Try to find the Knob. Create one if failed
  // and add it to the list.
  Knob* FindOrCreateKnob(K key) {
    // Find the knob. If cannot find, create a new one
    // We always put the new one as the new header.
    Knob *knob = FindKnob(key);
    if (!knob) {
      knob = NewKnob();
      knob->mNext = mHeader;
      knob->mKey = key;
      mHeader = knob;
    }
    return knob;
  }

  // Add an element to knob. It's the caller's duty to assure
  // knob is not NULL.
  void AddElem(Knob *knob, E data) {
    Elem *elem = knob->mChildren;
    Elem *found = NULL;
    while (elem) {
      if (elem->mData == data) {
        found = elem;
        break;
      }
      elem = elem->mNext;
    }

    if (!found) {
      Elem *e = NewElem();
      e->mData = data;
      e->mNext = knob->mChildren;
      knob->mChildren = e;
    }
  }

  // return true : if find the element
  //       false : if fail
  bool FindElem(Knob *knob, E data) {
    Elem *elem = knob->mChildren;
    while (elem) {
      if (elem->mData == data)
        return true;
      elem = elem->mNext;
    }
    return false;
  }

  // Remove elem from the list. If elem doesn't exist, exit quietly.
  // It's caller's duty to assure elem exists.
  void RemoveElem(Knob *knob, E data) {
    Elem *elem = knob->mChildren;
    Elem *elem_prev = NULL;
    Elem *target = NULL;
    while (elem) {
      if (elem->mData == data) {
        target = elem;
        break;
      }
      elem_prev = elem;
      elem = elem->mNext;
    }

    if (target) {
      if (target == knob->mChildren)
        knob->mChildren = target->mNext;
      else
        elem_prev->mNext = target->mNext;
    }
  }

  // Move the element to be the first child  of knob.
  // It's the caller's duty to make sure 'data' does exist
  // in knob's children.
  void MoveElemToHead(Knob *knob, E data) {
    Elem *target_elem = NULL;
    Elem *elem = knob->mChildren;
    Elem *elem_prev = NULL;
    while (elem) {
      if (elem->mData == data) {
        target_elem = elem;
        break;
      }
      elem_prev = elem;
      elem = elem->mNext;
    }

    if (target_elem && (target_elem != knob->mChildren)) {
      elem_prev->mNext = target_elem->mNext;
      target_elem->mNext = knob->mChildren;
      knob->mChildren = target_elem;
    }
  }

  // Try to find the first child of Knob k. Return the data.
  // found is set to false if fails, or true.
  // [NOTE] It's the user's responsibilty to make sure the Knob
  //        of 'key' exists.
  E FindFirstElem(Knob *knob, bool &found) {
    Elem *e = knob->mChildren;
    if (!e) {
      found = false;
      return 0;
    }
    found = true;
    return e->mData;
  }

  // return num of elements in knob.
  // It's caller's duty to assure knob is not NULL.
  unsigned NumOfElem(Knob *knob) {
    Elem *e = knob->mChildren;
    unsigned c = 0;
    while(e) {
      c++;
      e = e->mNext;
    }
    return c;
  }

  // Return the idx-th element in knob.
  // It's caller's duty to assure the validity of return value.
  // It doesn't check validity here.
  // Index starts from 0.
  E GetElemAtIndex(Knob *knob, unsigned idx) {
    Elem *e = knob->mChildren;
    unsigned c = 0;
    E data;
    while(e) {
      if (c == idx) {
        data = e->mData;
        break;
      }
      c++;
      e = e->mNext;
    }
    return data;
  }

public:
  Guamian() {mHeader = NULL;}
  ~Guamian(){Release();}

  void AddElem(K key, E data) {
    Knob *knob = FindOrCreateKnob(key);
    AddElem(knob, data);
  }

  // If 'data' doesn't exist, it ends quietly
  void RemoveElem(K key, E data) {
    Knob *knob = FindOrCreateKnob(key);
    RemoveElem(knob, data);
  }

  // Try to find the first child of Knob k. Return the data.
  // found is set to false if fails, or true.
  // [NOTE] It's the user's responsibilty to make sure the Knob
  //        of 'key' exists.
  E FindFirstElem(K key, bool &found) {
    Knob *knob = FindKnob(key);
    if (!knob) {
      found = false;
      return 0;   // return value doesn't matter when fails.
    }
    E data = FindFirstElem(knob, found);
    return data;
  }

  // return true : if find the element
  //       false : if fail
  bool FindElem(K key, E data) {
    Knob *knob = FindKnob(key);
    if (!knob)
      return false;
    return FindElem(knob, data);
  }

  // Move element to be the header
  // If 'data' doesn't exist, it ends quietly.
  void MoveElemToHead(K key, E data) {
    Knob *knob = FindKnob(key);
    if (!knob)
      return;
    MoveElemToHead(knob, data);
  }

  /////////////////////////////////////////////////////////
  // Paired operations start with finding a knob. It can
  // be either PairedFindKnob() or PairedFindOrCreateKnob()
  // Following that, there could be any number of operations
  // like searching, adding, moving an element.
  /////////////////////////////////////////////////////////

  void PairedFindOrCreateKnob(K key) {
    mTempKnob = FindOrCreateKnob(key);
  }

  bool PairedFindKnob(K key) {
    mTempKnob = FindKnob(key);
    if (mTempKnob)
      return true;
    else
      return false;
  }

  void PairedAddElem(E data) {
    AddElem(mTempKnob, data);
  }

  // If 'data' doesn't exist, it ends quietly
  void PairedRemoveElem(E data) {
    RemoveElem(mTempKnob, data);
  }

  bool PairedFindElem(E data) {
    return FindElem(mTempKnob, data);
  }

  // If 'data' doesn't exist, it ends quietly.
  void PairedMoveElemToHead(E data) {
    MoveElemToHead(mTempKnob, data);
  }

  E PairedFindFirstElem(bool &found) {
    return FindFirstElem(mTempKnob, found);
  }

  // return num of elements in current temp knob.
  // It's caller's duty to assure knob is not NULL.
  unsigned PairedNumOfElem() {
    return NumOfElem(mTempKnob);
  }

  // Return the idx-th element in knob.
  // It's caller's duty to assure the validity of return value.
  // It doesn't check validity here.
  // Index starts from 0.
  E PairedGetElemAtIndex(unsigned idx) {
    return GetElemAtIndex(mTempKnob, idx);
  }

  // Reduce the element at index exc_idx.
  // It's caller's duty to assure the element exists.
  void PairedReduceElems(unsigned exc_idx) {
    ReduceElems(mTempKnob, exc_idx);
  }

  void PairedSetKnobData(D d) {
    mTempKnob->mData = d;
  }

  D PairedGetKnobData() {
    return mTempKnob->mData;
  }

  /////////////////////////////////////////////////////////
  //                 Other functions
  /////////////////////////////////////////////////////////

  void Release() {mMemPool.Release();}
};

#endif
