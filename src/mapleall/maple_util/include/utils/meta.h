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
#ifndef MAPLE_UTIL_INCLUDE_UTILS_META_H
#define MAPLE_UTIL_INCLUDE_UTILS_META_H

namespace maple {
namespace utils {
template <typename T, typename U>
struct MetaAnd : public std::conditional_t<T::value, U, T> {};

template <typename T, typename U>
struct MetaOr : public std::conditional_t<T::value, T, U> {};

template <typename T>
struct MetaNot : public std::integral_constant<bool, !static_cast<bool>(T::value)>::type {};

template <typename ...>
struct IsSigned;

template <typename T>
struct IsSigned<T>
    : public std::is_signed<T>::type {};

template <typename T, typename U>
struct IsSigned<T, U>
    : public MetaAnd<std::is_signed<T>, std::is_signed<U>>::type {};

template <typename ...T>
constexpr bool kIsSignedV = IsSigned<T...>::value;

template <typename ...>
struct IsUnsigned;

template <typename T>
struct IsUnsigned<T>
    : public std::is_unsigned<T>::type {};

template <typename T, typename U>
struct IsUnsigned<T, U>
    : public MetaAnd<std::is_unsigned<T>, std::is_unsigned<U>>::type {};

template <typename ...T>
constexpr bool kIsUnsignedV = IsUnsigned<T...>::value;

template <typename T, typename U>
struct IsSameSign : public MetaOr<IsSigned<T, U>, IsUnsigned<T, U>>::type {};

template <typename T, typename U>
struct IsDiffSign : public MetaNot<IsSameSign<T, U>>::type {};

template <typename ...>
struct IsPointer;

template <typename T>
struct IsPointer<T>
    : public std::is_pointer<T>::type {};

template <typename T, typename U>
struct IsPointer<T, U>
    : public MetaAnd<IsPointer<T>, IsPointer<U>>::type {};

template <typename ...T>
constexpr bool kIsPointerV = IsPointer<T...>::value;

template <typename T, typename U>
struct ConstOf : public MetaAnd<std::is_const<U>, std::is_same<std::add_const_t<T>, U>>::type {};

template <typename T, typename U>
constexpr bool kConstOfV = ConstOf<T, U>::value;

template <typename T, typename U>
struct is_ncv_same
    : public std::is_same<std::remove_cv_t<T>, std::remove_cv_t<U>>::type {};

template <typename T, typename U>
constexpr bool kIsNcvSameV = is_ncv_same<T, U>::value;

namespace ptr {
template <typename T, typename U, typename = std::enable_if_t<kIsPointerV<T, U>>>
struct ConstOf : public utils::ConstOf<std::remove_pointer_t<T>, std::remove_pointer_t<U>>::type {};

template <typename T, typename U, typename = std::enable_if_t<kIsPointerV<T, U>>>
constexpr bool constOfV = ConstOf<T, U>::value;

template <typename T, typename U, typename = std::enable_if_t<kIsPointerV<T, U>>>
struct IsNcvSame : public utils::is_ncv_same<std::remove_pointer_t<T>, std::remove_pointer_t<U>>::type {};

template <typename T, typename U, typename = std::enable_if_t<kIsPointerV<T, U>>>
constexpr bool kIsNcvSameV = IsNcvSame<T, U>::value;
}
}}
#endif // MAPLE_UTIL_INCLUDE_UTILS_META_H
