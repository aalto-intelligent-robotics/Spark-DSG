/* -----------------------------------------------------------------------------
 * Copyright 2022 Massachusetts Institute of Technology.
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Research was sponsored by the United States Air Force Research Laboratory and
 * the United States Air Force Artificial Intelligence Accelerator and was
 * accomplished under Cooperative Agreement Number FA8750-19-2-1000. The views
 * and conclusions contained in this document are those of the authors and should
 * not be interpreted as representing the official policies, either expressed or
 * implied, of the United States Air Force or the U.S. Government. The U.S.
 * Government is authorized to reproduce and distribute reprints for Government
 * purposes notwithstanding any copyright notation herein.
 * -------------------------------------------------------------------------- */
#include "spark_dsg/edge_container.h"

#include "spark_dsg/edge_attributes.h"
#include "spark_dsg/node_symbol.h"

namespace spark_dsg {

using Edge = EdgeContainer::Edge;

SceneGraphEdge::SceneGraphEdge(NodeId source,
                               NodeId target,
                               std::unique_ptr<EdgeAttributes>&& info)
    : source(source), target(target), info(std::move(info)) {}

SceneGraphEdge::~SceneGraphEdge() = default;

void EdgeContainer::insert(NodeId source,
                           NodeId target,
                           std::unique_ptr<EdgeAttributes>&& edge_info) {
  auto attrs = (edge_info == nullptr) ? std::make_unique<EdgeAttributes>()
                                      : std::move(edge_info);

  edges.emplace(std::piecewise_construct,
                std::forward_as_tuple(source, target),
                std::forward_as_tuple(source, target, std::move(attrs)));
  edge_status[EdgeKey(source, target)] = EdgeStatus::NEW;
}

void EdgeContainer::remove(NodeId source, NodeId target) {
  const EdgeKey key(source, target);
  edge_status.at(key) = EdgeStatus::DELETED;
  edges.erase(key);
}

void EdgeContainer::rewire(NodeId source,
                           NodeId target,
                           NodeId new_source,
                           NodeId new_target) {
  EdgeKey key(source, target);
  const auto prev = find(key);
  if (!prev) {
    return;
  }

  auto attrs = prev->info->clone();
  edge_status.at(key) = EdgeStatus::MERGED;
  remove(source, target);
  insert(new_source, new_target, std::move(attrs));
}

bool EdgeContainer::contains(NodeId source, NodeId target) const {
  return edges.count(EdgeKey(source, target));
}

size_t EdgeContainer::size() const { return edges.size(); }

void EdgeContainer::reset() {
  edges.clear();
  edge_status.clear();
}

EdgeStatus EdgeContainer::getStatus(NodeId source, NodeId target) const {
  const auto iter = edge_status.find(EdgeKey(source, target));
  return (iter == edge_status.end()) ? EdgeStatus::NONEXISTENT : iter->second;
}

const Edge* EdgeContainer::find(NodeId source, NodeId target) const {
  const EdgeKey key(source, target);
  return find(key);
}

Edge* EdgeContainer::find(NodeId source, NodeId target) {
  const EdgeKey key(source, target);
  return find(key);
}

void EdgeContainer::getNew(std::vector<EdgeKey>& new_edges, bool clear_new) {
  auto iter = edge_status.begin();
  while (iter != edge_status.end()) {
    if (iter->second == EdgeStatus::NEW) {
      new_edges.push_back(iter->first);
      if (clear_new) {
        iter->second = EdgeStatus::VISIBLE;
      }
    }

    ++iter;
  }
}

void EdgeContainer::getRemoved(std::vector<EdgeKey>& removed_edges,
                               bool clear_removed) {
  auto iter = edge_status.begin();
  while (iter != edge_status.end()) {
    if (iter->second != EdgeStatus::DELETED) {
      ++iter;
      continue;
    }

    removed_edges.push_back(iter->first);
    if (clear_removed) {
      iter = edge_status.erase(iter);
    } else {
      ++iter;
    }
  }
}

void EdgeContainer::setStale() {
  stale_edges.clear();
  for (const auto& key_edge_pair : edges) {
    stale_edges[key_edge_pair.first] = true;
  }
}

NodeId getMergedId(NodeId node, const std::map<NodeId, NodeId>* merges) {
  if (!merges) {
    return node;
  }

  auto iter = merges->find(node);
  return iter == merges->end() ? node : iter->second;
}

void EdgeContainer::updateFrom(const EdgeContainer& other,
                               const std::map<NodeId, NodeId>* merges) {
  for (const auto& id_edge_pair : other_layer.edges_.edges) {
    const auto& edge = id_edge_pair.second;
    NodeId new_source = config.getMergedId(edge.source);
    NodeId new_target = config.getMergedId(edge.target);
    if (new_source == new_target) {
      continue;
    }

    // NOTE(nathan) not really necessary but saves the clone call
    if (hasEdge(new_source, new_target)) {
      // TODO(nathan) clone attributes
      continue;
    }

    insertEdge(new_source, new_target, edge.info->clone());
  }

  std::vector<EdgeKey> removed_edges;
  other_layer->edges_.getRemoved(removed_edges, config.clear_removed);
  for (const auto& removed_edge : removed_edges) {
    NodeId new_source = config.getMergedId(removed_edge.k1);
    NodeId new_target = config.getMergedId(removed_edge.k2);
    if (new_source == new_target) {
      continue;
    }

    if (other_layer->hasEdge(new_source, new_target)) {
      continue;  // edge still exists through merged node
    }

    layers_[l_id]->removeEdge(new_source, new_target);
  }

  for (const auto& id_edge_pair : other.interlayer_edges()) {
    const auto& edge = id_edge_pair.second;
    NodeId new_source = config.getMergedId(edge.source);
    NodeId new_target = config.getMergedId(edge.target);
    if (new_source == new_target) {
      continue;
    }

    if (config.enforce_parent_constraints) {
      insertParentEdge(new_source, new_target, edge.info->clone());
    } else {
      insertEdge(new_source, new_target, edge.info->clone());
    }
  }
}

Edge* EdgeContainer::find(const EdgeKey& key) const {
  auto iter = edges.find(key);
  if (iter == edges.end()) {
    return nullptr;
  }

  stale_edges[key] = false;
  return const_cast<SceneGraphEdge*>(&iter->second);
}

}  // namespace spark_dsg
