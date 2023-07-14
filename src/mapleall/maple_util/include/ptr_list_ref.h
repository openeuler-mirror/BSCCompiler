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
#ifndef MAPLE_UTIL_INCLUDE_PTR_LIST_REF_H
#define MAPLE_UTIL_INCLUDE_PTR_LIST_REF_H
#include <iterator>

#include "mpl_logging.h"

namespace maple {
template <typename T>
class PtrListNodeBase {
 public:
  PtrListNodeBase() = default;
  ~PtrListNodeBase() = default;
  T *GetPrev() const {
    return prev;
  }

  T *GetNext() const {
    return next;
  }

  void SetPrev(T *ptr) {
    prev = ptr;
  }

  void SetNext(T *ptr) {
    next = ptr;
  }

 private:
  T *prev = nullptr;
  T *next = nullptr;
};

// wrap iterator to run it backwards
template <typename T>
class ReversePtrListRefIterator {
 public:
  using iterator_category = typename std::iterator_traits<T>::iterator_category;
  using value_type = typename std::iterator_traits<T>::value_type;
  using difference_type = typename std::iterator_traits<T>::difference_type;
  using pointer = typename std::iterator_traits<T>::pointer;
  using reference = typename std::iterator_traits<T>::reference;

  using iterator_type = T;

  ReversePtrListRefIterator() : current() {}

  explicit ReversePtrListRefIterator(T right) : current(right) {}

  template <class Other>
  explicit ReversePtrListRefIterator(const ReversePtrListRefIterator<Other> &right) : current(right.base()) {}

  template <class Other>
  ReversePtrListRefIterator &operator=(const ReversePtrListRefIterator<Other> &right) {
    current = right.base();
    return (*this);
  }

  ~ReversePtrListRefIterator() = default;

  T base() const {
    return current;
  }

  reference operator*() const {
    return *current;
  }

  pointer operator->() const {
    return &(operator*());
  }

  ReversePtrListRefIterator &operator++() {
    --current;
    return (*this);
  }

  ReversePtrListRefIterator operator++(int) {
    ReversePtrListRefIterator tmp = *this;
    --current;
    return (tmp);
  }

  ReversePtrListRefIterator &operator--() {
    ++current;
    return (*this);
  }

  ReversePtrListRefIterator operator--(int) {
    ReversePtrListRefIterator tmp = *this;
    ++current;
    return (tmp);
  }

  bool operator==(const ReversePtrListRefIterator &Iterator) const {
    return this->base() == Iterator.base();
  }

  bool operator!=(const ReversePtrListRefIterator &Iterator) const {
    return !(*this == Iterator);
  }

 protected:
  T current;
};

template <typename T>
class PtrListRefIterator {
 public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  using const_pointer = const T*;
  using const_reference = const T&;

  PtrListRefIterator() = default;

  explicit PtrListRefIterator(pointer pointr) : ptr(pointr) {}

  template <typename U, typename = std::enable_if_t<std::is_same<U, std::remove_const_t<T>>::value>>
  PtrListRefIterator(const PtrListRefIterator<U> &iter) : ptr(iter.d()) {}

  ~PtrListRefIterator() = default;

  pointer d() const {
    return ptr;
  }

  reference operator*() const {
    return *ptr;
  }

  pointer operator->() const {
    return ptr;
  }

  PtrListRefIterator &operator++() {
    this->ptr = this->ptr->GetNext();
    return *this;
  }

  PtrListRefIterator &operator--() {
    this->ptr = this->ptr->GetPrev();
    return *this;
  }

  PtrListRefIterator operator++(int) {
    PtrListRefIterator it = *this;
    ++(*this);
    return it;
  }

  PtrListRefIterator operator--(int) {
    PtrListRefIterator it = *this;
    --(*this);
    return it;
  }

  bool operator==(const PtrListRefIterator &Iterator) const {
    return this->ptr == Iterator.ptr;
  }

  bool operator!=(const PtrListRefIterator &Iterator) const {
    return !(*this == Iterator);
  }

 private:
  pointer ptr = nullptr;
};

template <typename T>
class PtrListRef {
 public:
  using value_type = T;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  using iterator = PtrListRefIterator<T>;
  using const_iterator = PtrListRefIterator<const T>;
  using reverse_iterator = ReversePtrListRefIterator<iterator>;
  using const_reverse_iterator = ReversePtrListRefIterator<const_iterator>;

  PtrListRef() = default;
  explicit PtrListRef(pointer value) : first(value), last(value) {}

  PtrListRef(pointer firster, pointer laster) : first(firster), last(laster == nullptr ? firster : laster) {}

  ~PtrListRef() = default;

  iterator begin() {
    return iterator(this->first);
  }

  const_iterator begin() const {
    return const_iterator(this->first);
  }

  const_iterator cbegin() const {
    return const_iterator(this->first);
  }

  iterator end() {
    return iterator(this->last == nullptr ? nullptr : this->last->GetNext());
  }

  const_iterator end() const {
    return const_iterator(this->last == nullptr ? nullptr : this->last->GetNext());
  }

  const_iterator cend() const {
    return const_iterator(this->last == nullptr ? nullptr : this->last->GetNext());
  }

  reverse_iterator rbegin() {
    return reverse_iterator(iterator(this->last));
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(const_iterator(this->last));
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(const_iterator(this->last));
  }

  reverse_iterator rend() {
    return reverse_iterator(iterator(this->first == nullptr ? nullptr : this->first->GetPrev()));
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(const_iterator(this->first == nullptr ? nullptr : this->first->GetPrev()));
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(const_iterator(this->first == nullptr ? nullptr : this->first->GetPrev()));
  }

  reference front() {
    return *(this->first);
  }

  reference back() {
    return *(this->last);
  }

  const_reference front() const {
    return *(this->first);
  }

  const_reference back() const {
    return *(this->last);
  }

  bool empty() const {
    return first == nullptr;
  }

  void update_front(pointer value) {
    if (value != nullptr) {
      value->SetPrev(nullptr);
    }
    this->first = value;
  }

  void push_front(pointer val) {
    if (this->last == nullptr) {
      this->first = val;
      this->last = val;
      ASSERT(val != nullptr, "null ptr check");
      val->SetPrev(nullptr);
      val->SetNext(nullptr);
    } else {
      ASSERT(this->first != nullptr, "null ptr check");
      this->first->SetPrev(val);
      ASSERT(val != nullptr, "null ptr check");
      val->SetPrev(nullptr);
      val->SetNext(this->first);
      this->first = val;
    }
  }

  void pop_front() {
    if (this->first == nullptr) {
      return;
    }

    this->first = this->first->GetNext();
    if (this->first != nullptr) {
      this->first->SetPrev(nullptr);
    }
  }

  void update_back(pointer val) {
    if (val != nullptr) {
      val->SetNext(nullptr);
    }
    this->last = val;
  }

  void push_back(pointer val) {
    if (this->last == nullptr) {
      this->first = val;
      this->last = val;
      val->SetPrev(nullptr);
    } else {
      this->last->SetNext(val);
      val->SetPrev(this->last);
      this->last = val;
    }
    val->SetNext(nullptr);
  }

  void pop_back() {
    if (this->last == nullptr) {
      return;
    }

    if (this->last->GetPrev() == nullptr) {
      this->first = nullptr;
      this->last = nullptr;
    } else {
      this->last = this->last->GetPrev();
      this->last->SetNext(nullptr);
    }
  }

  void insert(const_iterator where, pointer value) {
    if (where == const_iterator(this->first)) {
      this->push_front(value);
    } else if (where == this->cend()) {
      this->push_back(value);
    } else {
      // `_Where` stands for the position, however we made the data and node combined, so a const_cast is needed.
      auto *ptr = const_cast<T*>(&*where);
      value->SetPrev(ptr->GetPrev());
      value->SetNext(ptr);
      value->GetPrev()->SetNext(value);
      ptr->SetPrev(value);
    }
  }

  void insert(const_pointer where, pointer val) {
    this->insert(const_iterator(where), val);
  }

  void insertAfter(const_iterator where, pointer val) {
    if (where == const_iterator(nullptr)) {
      this->push_front(val);
    } else if (where == const_iterator(this->last)) {
      this->push_back(val);
    } else {
      // `_Where` stands for the position, however we made the data and node combined, so a const_cast is needed.
      auto *ptr = const_cast<T*>(&*where);
      ASSERT(val != nullptr, "null ptr check");
      val->SetPrev(ptr);
      val->SetNext(ptr->GetNext());
      val->GetNext()->SetPrev(val);
      ptr->SetNext(val);
    }
  }

  void insertAfter(const_pointer where, pointer val) {
    this->insertAfter(const_iterator(where), val);
  }

  void splice(const_iterator where, PtrListRef &other) {
    ASSERT(!other.empty(), "NYI");
    if (this->empty()) {
      this->first = &(other.front());
      this->last = &(other.back());
    } else if (where == this->cend() || where == const_iterator(this->last)) {
      ASSERT(this->last != nullptr, "null ptr check");
      this->last->SetNext(&(other.front()));
      other.front().SetPrev(this->last);
      this->last = &(other.back());
    } else {
      ASSERT(to_ptr(where) != nullptr, "null ptr check");
      ASSERT(where->GetNext() != nullptr, "null ptr check");
      // `_Where` stands for the position, however we made the data and node combined, so a const_cast is needed.
      auto *ptr = const_cast<T*>(&*where);
      other.front().SetPrev(ptr);
      other.back().SetNext(ptr->GetNext());
      ptr->GetNext()->SetPrev(&(other.back()));
      ptr->SetNext(&(other.front()));
    }
  }

  void splice(const_pointer where, PtrListRef &other) {
    splice(const_iterator(where), other);
  }

  void clear() {
    this->first = nullptr;
    this->last = nullptr;
  }

  iterator erase(const_iterator where) {
    if (where == this->cbegin() && where == this->rbegin().base()) {
      this->first = nullptr;
      this->last = nullptr;
    } else if (where == this->cbegin()) {
      // `_Where` stands for the position, however we made the data and node combined, so a const_cast is needed.
      auto *ptr = const_cast<T*>(&*where);
      this->first = ptr->GetNext();
      ASSERT(this->first != nullptr, "null ptr check");
      this->first->SetPrev(nullptr);
    } else if (where == this->rbegin().base()) {
      pop_back();
    } else {
      ASSERT(where->GetPrev() != nullptr, "null ptr check");
      // `_Where` stands for the position, however we made the data and node combined, so a const_cast is needed.
      auto *ptr = const_cast<T*>(&*where);
      ptr->GetPrev()->SetNext(ptr->GetNext());
      if (ptr->GetNext()) {
        ptr->GetNext()->SetPrev(ptr->GetPrev());
      }
    }
    return iterator(nullptr);
  }

  iterator erase(const_pointer where) {
    return this->erase(const_iterator(where));
  }

  void set_first(T *f) {
    this->first = f;
  }

  void set_last(T *f) {
    this->last = f;
  }

 private:
  T *first = nullptr;
  T *last = nullptr;
};

template <typename Iterator>
auto to_ptr(Iterator it) -> typename std::iterator_traits<Iterator>::pointer {
  return it.d();
}

template <typename Iterator>
auto to_ptr(ReversePtrListRefIterator<Iterator> it) -> typename std::iterator_traits<Iterator>::pointer {
  return it.base().d();
}
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_PTR_LIST_REF_H
