#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>

#include "core_diagram.h"
#include "core_id_value.h"
#include "cpp_assert.h"
#include "imgui_node_editor.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "core_i_node.h"
#include "coreui_diagram.h"
#include "coreui_node_mover.h"
#include "flow_tree.h"
#include "flow_tree_traversal.h"
#include "imgui.h"

namespace vh::ponc::coreui {
///
NodeMover::NodeMover(cpp::SafePtr<Diagram> parent_diagram)
    : parent_diagram_{std::move(parent_diagram)} {}

///
void NodeMover::OnFrame() {
  MarkNewNodesToMove();
  ApplyMoves();

  nodes_to_move_.clear();
  node_sizes_.clear();
  pin_poses_.clear();
}

///
void NodeMover::MoveNodeTo(ne::NodeId node_id, const ImVec2& pos) {
  const auto& diagram = parent_diagram_->GetDiagram();
  auto& node = core::Diagram::FindNode(diagram, node_id);

  node.SetPos(pos);
  MarkToMove(node_id);
}

///
void NodeMover::MoveNodesTo(const std::vector<ne::NodeId>& node_ids,
                            const ImVec2& pos) {
  auto next_node_pos = pos;

  for (const auto node_id : node_ids) {
    MoveNodeTo(node_id, next_node_pos);
    next_node_pos.y += GetNodeSize(node_id).y;
  }
}

///
auto NodeMover::GetNodePos(ne::NodeId node_id) {
  const auto& diagram = parent_diagram_->GetDiagram();
  const auto& node = core::Diagram::FindNode(diagram, node_id);
  return node.GetPos();
}

///
auto NodeMover::GetNodeRect(ne::NodeId node_id) {
  auto rect = ImRect{{}, GetNodeSize(node_id)};
  rect.Translate(GetNodePos(node_id));
  return rect;
}

///
auto NodeMover::GetTreeRect(const flow::TreeNode& tree_node) {
  auto rect = GetNodeRect(tree_node.node_id);

  flow::TraverseDepthFirst(
      tree_node,
      [this, &rect](const auto& tree_node) {
        rect.Add(GetNodeRect(tree_node.node_id));
      },
      [](const auto&) {});

  return rect;
}

///
void NodeMover::MakeTreeImpl(const flow::TreeNode& tree_node) {
  flow::TraverseDepthFirst(
      tree_node, [](const auto&) {},
      [this](const auto& tree_node) {
        if (tree_node.child_nodes.empty()) {
          return;
        }

        const auto node_rect = GetNodeRect(tree_node.node_id);
        const auto next_child_x = node_rect.Max.x;

        const auto& first_child = tree_node.child_nodes.cbegin()->second;
        const auto tree_top_to_first_child_distance =
            GetNodeRect(first_child.node_id).Min.y -
            GetTreeRect(first_child).Min.y;

        const auto& last_child =
            std::prev(tree_node.child_nodes.cend())->second;
        const auto last_child_to_tree_bot_distance =
            GetTreeRect(last_child).Max.y -
            GetNodeRect(last_child.node_id).Max.y;

        const auto direct_children_height = std::accumulate(
            tree_node.child_nodes.cbegin(), tree_node.child_nodes.cend(),
            -tree_top_to_first_child_distance - last_child_to_tree_bot_distance,
            [this](const auto height, const auto& child_node) {
              return height + GetTreeRect(child_node.second).GetHeight();
            });

        auto next_child_y = node_rect.GetCenter().y -
                            tree_top_to_first_child_distance -
                            direct_children_height / 2;

        for (const auto& child_node : tree_node.child_nodes) {
          MoveTreeTo(child_node.second, {next_child_x, next_child_y});

          const auto child_tree_rect = GetTreeRect(child_node.second);
          next_child_y += child_tree_rect.GetHeight();
        }
      });
}

///
void NodeMover::MakeTree(const flow::TreeNode& tree_node) {
  MakeTreeImpl(tree_node);
}

///
void NodeMover::MakeTrees(const std::vector<flow::TreeNode>& tree_nodes) {
  const auto& diagram = parent_diagram_->GetDiagram();

  auto last_tree_rect = std::optional<ImRect>{};

  for (const auto& tree_node : tree_nodes) {
    MakeTreeImpl(tree_node);
    const auto tree_rect = GetTreeRect(tree_node);

    if (!last_tree_rect.has_value()) {
      last_tree_rect = tree_rect;
      continue;
    }

    const auto& node = core::Diagram::FindNode(diagram, tree_node.node_id);
    MoveNodeTo(tree_node.node_id,
               {last_tree_rect->Min.x,
                node.GetPos().y - tree_rect.Min.y + last_tree_rect->Max.y});

    MakeTreeImpl(tree_node);
    last_tree_rect = GetTreeRect(tree_node);
  }
}

///
void NodeMover::MovePinTo(ne::PinId pin_id, const ImVec2& pos) {
  const auto current_pin_pos = GetPinPos(pin_id);
  const auto& node =
      core::Diagram::FindPinNode(parent_diagram_->GetDiagram(), pin_id);
  const auto matching_node_pos = node.GetPos() - current_pin_pos + pos;

  MoveNodeTo(node.GetId(), matching_node_pos);
}

///
auto NodeMover::GetNodeSize(ne::NodeId node_id) const -> const ImVec2& {
  const auto node_id_value = node_id.Get();
  Expects(node_sizes_.contains(node_id_value));
  return node_sizes_.at(node_id_value);
}

///
void NodeMover::SetNodeSize(ne::NodeId node_id, const ImVec2& size) {
  node_sizes_.emplace(node_id, size);
}

///
auto NodeMover::GetPinPos(ne::PinId pin_id) const -> const ImVec2& {
  const auto pin_id_value = pin_id.Get();
  Expects(pin_poses_.contains(pin_id_value));
  return pin_poses_.at(pin_id_value);
}

///
void NodeMover::SetPinPos(ne::PinId pin_id, const ImVec2& pos) {
  pin_poses_.emplace(pin_id, pos);
}

///
void NodeMover::MoveTreeTo(const flow::TreeNode& tree_node, const ImVec2& pos) {
  const auto delta = pos - GetTreeRect(tree_node).Min;

  flow::TraverseDepthFirst(
      tree_node,
      [this, &delta](const auto& tree_node) {
        MoveNodeTo(tree_node.node_id, GetNodePos(tree_node.node_id) + delta);
      },
      [](const auto&) {});
}

///
void NodeMover::MarkToMove(ne::NodeId node_id) {
  nodes_to_move_.insert(node_id.Get());
}

///
void NodeMover::MarkNewNodesToMove() {
  const auto& diagram = parent_diagram_->GetDiagram();

  for (const auto& node : diagram.GetNodes()) {
    const auto node_id = node->GetId();

    if (const auto node_is_new = !node_sizes_.contains(node_id.Get())) {
      MarkToMove(node_id);
    }
  }
}

///
void NodeMover::ApplyMoves() const {
  const auto& diagram = parent_diagram_->GetDiagram();

  for (const auto node_id : nodes_to_move_) {
    ne::SetNodePosition(node_id,
                        core::Diagram::FindNode(diagram, node_id).GetPos());
  }
}
}  // namespace vh::ponc::coreui