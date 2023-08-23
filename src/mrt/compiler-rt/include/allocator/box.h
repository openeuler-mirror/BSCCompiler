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
#ifndef MAPLE_RUNTIME_BOX_H
#define MAPLE_RUNTIME_BOX_H

#include <cstddef>
#include <memory>

// Port of Rust Box
namespace maplert {
// Port of std::boxed::Box from the Rust programming language.
//
// Basically equivalent to C++ std::unique_ptr, but is supposed to be used with
// a custom allocator, and is not supposed to customize the deleter.  Basically,
//    - If you care more about where the object is allocated, use Box.
//    - If you care more about where the object is freed, use unique_ptr.
// Note that you can always convert a Box to and from Box::unique_ptr, a
// std::unique_ptr specialization, using IntoUniquePtr and FromUniquePtr.
//
// The user should allocate object using Box<T>::New, and let the Box<T> go out
// of scope so that it gets naturally deleted by the allocator.  We can also
// convert a Box<T> to T* using IntoRaw, and back from T* to Box<T> using
// FromRaw, so that the object remains alive when pointed by T*, and gets
// recycled after converting back to Box<T> and going out of scope.
//
// Requirements:
//   The allocator must be stateless.  The default allocator (std::allocator) is
//   stateless.  maplert::StdContainerAllocator is also statelss.
template<class T, class Allocator = std::allocator<T>>
class Box final {
 public:
  // Deleter that supports std::unique_ptr
  class Deleter {
   public:
    void operator()(T *ptr) {
      DestructAndDeallocate(ptr);
    }
  };

  // Our std::unique_ptr alias.
  using unique_ptr = std::unique_ptr<T, Deleter>;

  // Make an empty box.
  Box() : rawPointer(nullptr) {}

  // Move from another box, take the responsibility to free the object.
  Box(Box &&other) { // not explicit
    MoveFrom(other);
  };

  // Move from another box, take the responsibility to free the object.
  Box &operator=(Box &&other) {
    MoveFrom(other);
    return *this;
  }

  // Destruct the contained object and deallocate it.
  ~Box() {
    Drop();
  }

  // @return true if the box is empty (initialized as empty, moved, destructed,
  // or explicitly discarded).
  bool IsEmpty() const {
    return rawPointer == nullptr;
  }

  // Same as IsEmpty(), enabling the `box == nullptr` expression.
  bool operator==(std::nullptr_t) const {
    return IsEmpty();
  }

  // Same as !IsEmpty(), enabling the `box != nullptr` expression.
  bool operator!=(std::nullptr_t) const {
    return !IsEmpty();
  }

  // Get the reference to the contained object.  Similar to
  // std::unique_ptr::operator*()
  T &operator*() const {
    return *rawPointer;
  }

  // Allow the box->member syntax so that the box can be used as a pointer.
  // Similar to std::unique_ptr::operator->()
  T *operator->() const {
    return rawPointer;
  }

  // Convert into a raw pointer, and give up the responsibility to free the
  // object. Similar to std::unique_ptr::release()
  T *IntoRaw() {
    return TakePointer();
  }

  // Get the underlying raw pointer. Similar to std::unique_ptr::get().
  //
  // Use with care! It breaks the "This Box is the only pointer of the object"
  // rule.
  T *UnsafeGetRaw() const {
    return rawPointer;
  }

  // Convert into an std::unique_ptr.  The unique_ptr is now responsible for
  // freeing the object.
  unique_ptr IntoUniquePtr() {
    T *ptr = TakePointer();
    return unique_ptr(ptr);
  }

  // Destruct the contained object and deallocate the memory using the
  // allocator.  Do nothing if this Box has already been moved or destroyed.
  void Drop() noexcept {
    if (rawPointer != nullptr) {
      T *ptr = TakePointer();
      DestructAndDeallocate(ptr);
    }
  }

  // Create an empty, with a nullptr as its content. Same as the default
  // constructor, but more explicit.
  static Box Empty() {
    return Box();
  }

  // Create a box using the constructor of a type. Similar to std::make_unique.
  template<class... Args>
  static Box New(Args &&...args) {
    T *ptr = AllocateAndConstruct(std::forward<Args>(args)...);
    return Box(ptr);
  }

  // Convert a raw pointer back into a Box, taking the reponsibility to free the
  // object.  The object must be allocated by the same allocator specified for
  // the Box<T, Allocator> type.  Similar to the constructor of std::unique_ptr
  static Box FromRaw(T *ptr) {
    return Box(ptr);
  }

  // Convert a unique_ptr back into a Box, taking the reponsibility to free the
  // object.  The object must be allocated by the same allocator specified for
  // the Box<T, Allocator> type.
  static Box FromUniquePtr(unique_ptr uptr) {
    return FromRaw(uptr.release());
  }

 private:
  // Construct a box from a raw pointer.  Internal use, only.
  Box(T *ptr) : rawPointer(ptr) {}

  Box(const Box &other) = delete; // Cannot be copied.
  Box &operator=(const Box &other) = delete; // Cannot be copy-assigned.

  // Get the rawPointer, ensuring we do not forget to clear the rawPointer field.
  T *TakePointer() {
    T *ptr = rawPointer;
    rawPointer = nullptr;
    return ptr;
  }

  // Take the pointer (and the reponsibility to free the object) from another
  // Box
  void MoveFrom(Box &other) {
    rawPointer = other.TakePointer();
  }

  // Use the allocator to allocate and construct the object.
  template<class... Args>
  static T *AllocateAndConstruct(Args &&...args) {
    Allocator allocator;
    T *ptr = std::allocator_traits<Allocator>::allocate(allocator, 1);
    std::allocator_traits<Allocator>::construct(allocator, ptr, std::forward<Args>(args)...);
    return ptr;
  }

  // Use the allocator to destruct and deallocate the object.
  static void DestructAndDeallocate(T *ptr) {
    Allocator allocator;
    std::allocator_traits<Allocator>::destroy(allocator, ptr);
    std::allocator_traits<Allocator>::deallocate(allocator, ptr, 1);
  }

  // The raw pointer
  T *rawPointer;
};
} // namespace maplert

#endif // MAPLE_RUNTIME_BOX_H
