/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLEBE_INCLUDE_CG_VISITOR_COMMON_H
#define MAPLEBE_INCLUDE_CG_VISITOR_COMMON_H
namespace maplebe {
class OperandVisitorBase {
 public:
  virtual ~OperandVisitorBase() = default;
};

template<typename Visitable>
class OperandVisitor {
 public:
  virtual ~OperandVisitor() = default;
  virtual void Visit(Visitable *v) = 0;
};

template<typename ... V>
class OperandVisitors {
 public:
  virtual ~OperandVisitors() = default;
};

template<typename OpV1, typename OpV2, typename ... OpV3>
class OperandVisitors<OpV1, OpV2, OpV3 ...> :
    public OperandVisitor<OpV1>,
    public OperandVisitor<OpV2>,
    public OperandVisitor<OpV3 ...>
{};
}  /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_VISITOR_COMMON_H */
