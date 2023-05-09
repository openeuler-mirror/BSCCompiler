/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_UTIL_INCLUDE_BASE_GRAPH_NODE_H
#define MAPLE_UTIL_INCLUDE_BASE_GRAPH_NODE_H
#include "types_def.h"
#include "mempool_allocator.h"
namespace maple {
class BaseGraphNode {
 public:
  explicit BaseGraphNode(uint32 index) : id(index) {}
  virtual ~BaseGraphNode() = default;
  virtual void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) = 0;
  virtual void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) const = 0;
  virtual void GetInNodes(std::vector<BaseGraphNode*> &outNodes) = 0;
  virtual void GetInNodes(std::vector<BaseGraphNode*> &outNodes) const = 0;
  virtual const std::string GetIdentity() = 0;

  virtual FreqType GetNodeFrequency() const { return 0; }
  virtual FreqType GetEdgeFrequency(const BaseGraphNode &node) const { return 0; }
  virtual FreqType GetEdgeFrequency(size_t idx) const { return 0; }

  uint32 GetID() const {
    return id;
  }
 protected:
  void SetID(uint32 newIdx) {
    id = newIdx;
  }
 private:
  uint32 id;
};

// BaseGraphNode needs to be the base class of T
template<typename T, std::enable_if_t<std::is_base_of<BaseGraphNode, T>::value, bool> = true>
void ConvertToVectorOfBasePtr(const MapleVector<T*> &originVec, MapleVector<BaseGraphNode*> &targetVec) {
  for (auto &item : originVec) {
    targetVec.emplace_back(item);
  }
}
}
#endif /* MAPLE_UTIL_INCLUDE_BASE_GRAPH_NODE_H */
