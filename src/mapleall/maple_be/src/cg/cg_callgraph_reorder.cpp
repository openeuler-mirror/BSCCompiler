/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "cg_callgraph_reorder.h"

#include <fstream>
#include <numeric>

namespace maple {
struct Edge {
  uint32 src;
  uint64 weight;
};

class Cluster {
 public:
  Cluster(uint32 i, uint64 s, uint64 weight)
      : leader(i), next(i), prev(i), size(s), weight(weight), mostLikelyEdge{i, 0} {}

  double getDensity() const {
    if (size == 0) {
      return 0;
    }
    return double(weight) / double(size);
  }

  uint32 leader;
  uint32 next;
  uint32 prev;
  uint64 size;
  uint64 weight;
  Edge mostLikelyEdge;
};

class FuncSection {
 public:
  std::string funcName;
  uint64 weight;
  uint64 size;
};

static std::vector<Cluster> clusters = {};
static std::vector<FuncSection> funcs;

// reading function profile of the following format:
// calleeName calleeWeight calleeSize callerName callerWeight callerSize edgeWeight
// ...
static void ReadProfile(const std::string &path) {
  std::ifstream fs(path);
  if (!fs) {
    LogInfo::MapleLogger() << "WARN: failed to open " << path << '\n';
    return;
  }
  std::string line;
  std::map<std::string, uint32> funcName2Cluster;
  auto getOrCreateNode = [&funcName2Cluster](const FuncSection &f) {
    auto res = funcName2Cluster.emplace(f.funcName, clusters.size());
    if (res.second) {
      clusters.emplace_back(clusters.size(), f.size, f.weight);
      funcs.push_back(f);
    }
    return res.first->second;
  };

  while (std::getline(fs, line)) {
    std::istringstream ss(line);
    std::string calleeName, callerName;
    uint64 calleeSize, callerSize, calleeWeight, callerWeight, edgeWeight;
    ss >> calleeName >> calleeWeight >> calleeSize >> callerName >> callerWeight >> callerSize >> edgeWeight;
    if (!ss) {
      LogInfo::MapleLogger() << "WARN: unexpected format in Function Priority File" << '\n';
      return;
    }
    FuncSection callee{calleeName, calleeWeight, calleeSize};
    FuncSection caller{callerName, callerWeight, callerSize};
    uint32 src = getOrCreateNode(caller);
    uint32 dst = getOrCreateNode(callee);
    // recursive call
    if (src == dst) {
      continue;
    }
    auto &c = clusters[dst];
    if (c.mostLikelyEdge.weight < edgeWeight) {
      c.mostLikelyEdge.src = src;
      c.mostLikelyEdge.weight = edgeWeight;
    }
  }
}

// union-find with path-halving
static uint32 GetLeader(uint32 src) {
  uint32 v = src;
  while (clusters[v].leader != v) {
    v = clusters[v].leader;
  }
  clusters[src].leader = v;
  return v;
}

static void MergeClusters(Cluster &dst, uint32 dstIdx, Cluster &src, uint32 srcIdx) {
  uint32 tail1 = dst.prev, tail2 = src.prev;
  dst.prev = tail2;
  clusters[tail2].next = dstIdx;
  src.prev = tail1;
  clusters[tail1].next = srcIdx;
  dst.size += src.size;
  dst.weight += src.weight;
  src.size = 0;
  src.weight = 0;
}

std::map<std::string, uint32> ReorderAccordingProfile(const std::string &path) {
  ReadProfile(path);
  constexpr uint32 kMaxDensityDegradation = 8;
  constexpr uint64 kMaxClusterSize = 1024 * 1024;
  constexpr uint32 kUnlikelyThreshold = 10;
  std::vector<uint32> sortedIdx(clusters.size());
  std::iota(sortedIdx.begin(), sortedIdx.end(), 0);
  // sort the cluster's idx by density decreasing
  std::stable_sort(sortedIdx.begin(), sortedIdx.end(),
                   [](uint32 a, uint32 b) { return clusters[a].getDensity() > clusters[b].getDensity(); });
  for (auto idx : sortedIdx) {
    Cluster &c = clusters[idx];
    // skip if c is root of callgraph or the edge is not likely
    if (c.mostLikelyEdge.src == idx || c.mostLikelyEdge.weight * kUnlikelyThreshold <= funcs[idx].weight) {
      continue;
    }
    uint32 leader = GetLeader(c.mostLikelyEdge.src);
    if (leader == idx) {
      continue;
    }
    auto &dst = clusters[leader];
    if (c.size + dst.size > kMaxClusterSize) {
      continue;
    }
    auto newDensity = double(dst.weight + c.weight) / double(dst.size + c.size);
    // if the Cluster density degradate too much after merge, don't merge;
    if (newDensity * kMaxDensityDegradation < c.getDensity()) {
      continue;
    }
    c.leader = leader;
    MergeClusters(dst, leader, c, idx);
  }
  auto iter = std::remove_if(sortedIdx.begin(), sortedIdx.end(), [](uint32 idx) { return clusters[idx].size == 0; });
  sortedIdx.erase(iter, sortedIdx.end());

  std::stable_sort(sortedIdx.begin(), sortedIdx.end(),
                   [](uint32 a, uint32 b) { return clusters[a].getDensity() > clusters[b].getDensity(); });
  std::map<std::string, uint32> result;
  uint32 order = 1;
  for (auto idx : sortedIdx) {
    for (uint32 i = idx;;) {
      result[funcs[i].funcName] = order++;
      i = clusters[i].next;
      if (i == idx) {
        break;
      }
    }
  }
  return result;
}

}  // namespace maple
