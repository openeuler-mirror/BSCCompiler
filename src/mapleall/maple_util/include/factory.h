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
#ifndef MAPLE_UTIL_INCLUDE_FACTORY_H
#define MAPLE_UTIL_INCLUDE_FACTORY_H
#include <map>
#include <functional>
#include <mutex>
#include "thread_env.h"

namespace maple {
template <typename TKey, typename TObject, typename... TArgs>
class ObjectFactory final {
 public:
  using KeyType = TKey;
  using CreatorType = std::function<std::unique_ptr<TObject>(TArgs...)>;

  void Register(const KeyType &key, CreatorType func) {
    if (creator.find(key) == creator.end()) {
      creator[key] = func;
    }
  }

  std::unique_ptr<TObject> Create(const KeyType &key, TArgs ...args) const {
    auto it = creator.find(key);
    return it == creator.end() ? std::unique_ptr<TObject>() : (it->second)(std::forward<TArgs>(args)...);
  }

  template <typename TObjectImpl>
  static std::unique_ptr<TObject> DefaultCreator(TArgs ...args) {
    return std::make_unique<TObjectImpl>(std::forward<TArgs>(args)...);
  }

  static ObjectFactory &Ins() {
    static ObjectFactory factory;
    return factory;
  }

  ObjectFactory(const ObjectFactory&) = delete;
  ObjectFactory &operator=(const ObjectFactory&) = delete;
  ObjectFactory(const ObjectFactory&&) = delete;
  ObjectFactory &operator=(const ObjectFactory&&) = delete;

 private:
  ObjectFactory() = default;
  ~ObjectFactory() = default;

 private:
  using CreatorHolder = std::map<KeyType, CreatorType>;
  CreatorHolder creator;
};

template <typename TFactory, typename TFactory::KeyType Key, typename TObjectImpl>
inline void RegisterFactoryObject() {
  TFactory::Ins().Register(Key, TFactory::template DefaultCreator<TObjectImpl>);
}

template <typename TFactory, typename... TArgs>
inline auto CreateProductObject(const typename TFactory::KeyType &key, TArgs &&...args) {
  return TFactory::Ins().Create(key, std::forward<TArgs>(args)...);
}

template <typename TKey, typename TRet, typename... TArgs>
class FunctionFactory final {
 public:
  using KeyType = TKey;
  using CreatorType = std::function<TRet(TArgs...)>;

  void Register(const KeyType &key, CreatorType func) {
    static std::mutex mtx;
    ParallelGuard guard(mtx, ThreadEnv::IsMeParallel());
    if (creator.find(key) == creator.end()) {
      creator[key] = func;
    }
  }

  CreatorType Create(const KeyType &key) const {
    static std::mutex mtx;
    ParallelGuard guard(mtx, ThreadEnv::IsMeParallel());
    auto it = creator.find(key);
    return it == creator.end() ? nullptr : it->second;
  }

  static FunctionFactory &Ins() {
    static FunctionFactory factory;
    return factory;
  }

  FunctionFactory(const FunctionFactory&) = delete;
  FunctionFactory &operator=(const FunctionFactory&) = delete;
  FunctionFactory(const FunctionFactory&&) = delete;
  FunctionFactory &operator=(const FunctionFactory&&) = delete;

 private:
  FunctionFactory() = default;
  ~FunctionFactory() = default;

 private:
  using CreatorHolder = std::map<KeyType, CreatorType>;
  CreatorHolder creator;
};

template <typename TFactory>
inline void RegisterFactoryFunction(const typename TFactory::KeyType &key, typename TFactory::CreatorType func) {
  TFactory::Ins().Register(key, func);
}

template <typename TFactory>
inline auto CreateProductFunction(const typename TFactory::KeyType &key) {
  return TFactory::Ins().Create(key);
}
}  // namespace maple

#endif  // MAPLE_UTIL_INCLUDE_FACTORY_H
