/**
 * PONC @link https://github.com/qoala101/ponc @endlink
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 * @copyright Copyright (c) 2023, MIT License
 */

#include "coreui_diagram.h"

#include <imgui.h>
#include <imgui_node_editor.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "core_area.h"
#include "core_connection.h"
#include "core_diagram.h"
#include "core_i_family.h"
#include "core_i_node.h"
#include "core_id_generator.h"
#include "core_id_value.h"
#include "core_link.h"
#include "core_project.h"
#include "core_settings.h"
#include "coreui_area_creator.h"
#include "coreui_event_loop.h"
#include "coreui_family.h"
#include "coreui_flow_tree_node.h"
#include "coreui_i_family_traits.h"
#include "coreui_i_node_traits.h"
#include "coreui_i_pin_traits.h"
#include "coreui_link.h"
#include "coreui_linker.h"
#include "coreui_native_facade.h"
#include "coreui_node.h"
#include "coreui_pin.h"
#include "coreui_project.h"
#include "cpp_assert.h"
#include "cpp_safe_ptr.h"
#include "cpp_share.h"
#include "flow_algorithms.h"
#include "flow_node_flow.h"
#include "flow_tree_node.h"
#include "flow_tree_traversal.h"
#include "style_default_colors.h"
#include "style_default_sizes.h"
#include "style_utils.h"

namespace vh::ponc::coreui {
///
auto Diagram::FindNode(const Diagram& diagram, ne::NodeId node_id)
    -> const Node& {
  // NOLINTNEXTLINE(*-const-cast)
  return FindNode(const_cast<Diagram&>(diagram), node_id);
}

///
auto Diagram::FindNode(Diagram& diagram, ne::NodeId node_id) -> Node& {
  const auto node = std::find_if(diagram.nodes_.begin(), diagram.nodes_.end(),
                                 [node_id](const auto& node) {
                                   return node.GetNode().GetId() == node_id;
                                 });

  Expects(node != diagram.nodes_.cend());
  return *node;
}
///
auto Diagram::FindLink(const Diagram& diagram, ne::LinkId link_id)
    -> const Link& {
  const auto link = std::find_if(
      diagram.links_.begin(), diagram.links_.end(),
      [link_id](const auto& link) { return link.core_link.id == link_id; });

  Expects(link != diagram.links_.cend());
  return *link;
}

///
Diagram::Diagram(cpp::SafePtr<Project> parent_project,
                 cpp::SafePtr<core::Diagram> diagram)
    : parent_project_{std::move(parent_project)},
      diagram_{std::move(diagram)},
      node_mover_{
          safe_owner_.MakeSafe(this),
          safe_owner_.MakeSafe(&parent_project_->GetProject().GetSettings())},
      node_replacer_{parent_project_},
      linker_{safe_owner_.MakeSafe(this)},
      area_creator_{safe_owner_.MakeSafe(this),
                    safe_owner_.MakeSafe(
                        &parent_project_->GetProject().GetIdGenerator())} {}

///
void Diagram::OnFrame() {
  flow::RebuildFlowTrees(*diagram_, flow_trees_);

  node_mover_.OnFrame();

  const auto node_flows = flow::CalculateNodeFlows(
      flow_trees_,
      [&diagram = *diagram_](const auto node_id) {
        return core::Diagram::FindNode(diagram, node_id).GetInitialFlow();
      },
      [&diagram = *diagram_,
       &parent_project = parent_project_](const auto pin_id) {
        const auto link = core::Diagram::FindPinLink(diagram, pin_id);
        Expects(link.has_value());
        return core::Link::GetDrop(**link, parent_project->GetProject());
      });

  UpdateLinks(node_flows);
  UpdateNodes(node_flows);
  UpdateFamilyGroups();
  UpdateNodeTrees();
}

///
auto Diagram::GetDiagram() const -> core::Diagram& { return *diagram_; }

///
auto Diagram::GetFlowTrees() const -> const std::vector<flow::TreeNode>& {
  return flow_trees_;
}

///
auto Diagram::GetNodeMover() const -> const NodeMover& {
  // NOLINTNEXTLINE(*-const-cast)
  return const_cast<Diagram*>(this)->GetNodeMover();
}

///
auto Diagram::GetNodeMover() -> NodeMover& { return node_mover_; }

///
auto Diagram::GetNodeReplacer() const -> const NodeReplacer& {
  return node_replacer_;
}

///
auto Diagram::GetLinker() const -> const Linker& {
  // NOLINTNEXTLINE(*-const-cast)
  return const_cast<Diagram*>(this)->GetLinker();
}

///
auto Diagram::GetLinker() -> Linker& { return linker_; }

///
auto Diagram::GetAreaCreator() -> AreaCreator& { return area_creator_; }

///
auto Diagram::GetFamilyGroups() const -> const std::vector<FamilyGroup>& {
  return family_groups_;
}

///
auto Diagram::GetNodes() const -> const std::vector<Node>& {
  // NOLINTNEXTLINE(*-const-cast)
  return const_cast<Diagram*>(this)->GetNodes();
}

///
auto Diagram::GetNodes() -> std::vector<Node>& { return nodes_; }

///
auto Diagram::AddNode(std::unique_ptr<core::INode> node) const -> Event& {
  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, node = cpp::Share(std::move(node))]() {
        diagram->EmplaceNode(std::move(*node));
      });
}

///
auto Diagram::GetFreePinFamily(ne::PinKind pin_kind) const -> auto& {
  for (const auto& group : family_groups_) {
    const auto free_pin_family = std::find_if(
        group.families.cbegin(), group.families.cend(),
        [pin_kind](const auto& family) {
          const auto& core_family = family.GetFamily();

          if (core_family.GetType() != core::FamilyType::kFreePin) {
            return false;
          }

          const auto node = core_family.CreateSampleNode();
          return core::INode::FindFirstPinOfKind(*node, pin_kind).has_value();
        });

    if (free_pin_family != group.families.cend()) {
      return *free_pin_family;
    }
  }

  Expects(false);
}

///
auto Diagram::IsFreePin(const core::INode& node) const {
  const auto family_id = node.GetFamilyId();

  return (family_id ==
          GetFreePinFamily(ne::PinKind::Input).GetFamily().GetId()) ||
         (family_id ==
          GetFreePinFamily(ne::PinKind::Output).GetFamily().GetId());
}

///
auto Diagram::DeleteNode(ne::NodeId node_id) -> Event& {
  const auto& node = core::Diagram::FindNode(*diagram_, node_id);

  if (IsFreePin(node)) {
    return DeleteNodeWithLinks(node_id);
  }

  const auto input_pin = node.GetInputPinId();

  if (input_pin.has_value() && core::Diagram::HasLink(*diagram_, *input_pin)) {
    const auto& free_pin_family = GetFreePinFamily(ne::PinKind::Input);

    MoveConnectedLinkToNewFreePin(*input_pin, ne::PinKind::Input,
                                  free_pin_family);
  }

  const auto& free_pin_family = GetFreePinFamily(ne::PinKind::Output);

  for (const auto ouput_pin : node.GetOutputPinIds()) {
    if (core::Diagram::HasLink(*diagram_, ouput_pin)) {
      MoveConnectedLinkToNewFreePin(ouput_pin, ne::PinKind::Output,
                                    free_pin_family);
    }
  }

  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, node_id]() { diagram->DeleteNode(node_id); });
}

///
auto Diagram::DeleteNodeWithLinks(ne::NodeId node_id) const -> Event& {
  const auto& node = core::Diagram::FindNode(*diagram_, node_id);

  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, pins = core::INode::GetAllPins(node), node_id]() {
        for (const auto& pin : pins) {
          if (const auto link =
                  core::Diagram::FindPinLink(*diagram, pin.first)) {
            diagram->DeleteLink((*link)->id);
          }
        }

        diagram->DeleteNode(node_id);
      });
}

///
void Diagram::TreeSelect(const std::vector<ne::NodeId>& node_ids) {
  if (node_ids.empty()) {
    return;
  }

  for (const auto node_id : node_ids) {
    const auto& tree_node = flow::FindTreeNode(flow_trees_, node_id);

    flow::TraverseDepthFirst(
        tree_node,
        [](const auto& tree_node) {
          NativeFacade::SelectNode(tree_node.node_id, true);
        },
        [](const auto&) {});
  }
}

///
void Diagram::TreeArrange(const std::vector<ne::NodeId>& node_ids) {
  if (node_ids.empty()) {
    return;
  }

  for (const auto node_id : node_ids) {
    const auto& tree_node = flow::FindTreeNode(flow_trees_, node_id);
    node_mover_.ArrangeAsTree(tree_node);
  }
}

///
auto Diagram::GetLinks() const -> const std::vector<Link>& { return links_; }

///
auto Diagram::CreateLink(ne::PinId start_pin_id, ne::PinId end_pin_id) const
    -> Event& {
  auto& core_project = parent_project_->GetProject();

  auto& id_generator = core_project.GetIdGenerator();
  const auto link_id = id_generator.Generate<ne::LinkId>();

  const auto& default_connection =
      core_project.GetSettings().default_connection;
  const auto connection = default_connection.has_value()
                              ? core::LinkConnection{*default_connection}
                              : core::LinkConnection{};

  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, link = core::Link{.id = link_id,
                                             .start_pin_id = start_pin_id,
                                             .end_pin_id = end_pin_id,
                                             .connection = connection}]() {
        diagram->EmplaceLink(link);
      });
}

///
auto Diagram::MoveLink(ne::PinId source_pin_id, ne::PinId target_pin_id) const
    -> Event& {
  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, source_pin_id, target_pin_id]() {
        const auto link = core::Diagram::FindPinLink(*diagram, source_pin_id);

        if (!link.has_value()) {
          return;
        }

        auto& pin_to_move = ((*link)->start_pin_id == source_pin_id)
                                ? (*link)->start_pin_id
                                : (*link)->end_pin_id;
        pin_to_move = target_pin_id;
      });
}

///
auto Diagram::DeleteLink(ne::LinkId link_id) const -> Event& {
  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, link_id]() { diagram->DeleteLink(link_id); });
}

///
auto Diagram::GetNodeTrees() const -> const std::vector<TreeNode>& {
  return node_trees_;
}

///
auto Diagram::AddArea(const core::Area& area) -> Event& {
  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, area]() { diagram->EmplaceArea(area); });
}

///
auto Diagram::DeleteArea(core::AreaId area_id) -> Event& {
  return parent_project_->GetEventLoop().PostEvent(
      [diagram = diagram_, area_id]() { diagram->DeleteArea(area_id); });
}

///
auto Diagram::GetFlowColor(float flow) const {
  const auto& settings = parent_project_->GetProject().GetSettings();

  if (!settings.color_flow) {
    return ImColor{style::DefaultColors::kWhite};
  }

  return core::Settings::GetFlowColor(settings, flow);
}

///
auto Diagram::LinkFrom(const core::Link& core_link,
                       const flow::NodeFlows& node_flows) const {
  const auto& project = parent_project_->GetProject();
  const auto& settings = project.GetSettings();

  const auto thickness =
      settings.thick_links
          ? core::Settings::GetLinkThickness(settings, core_link.length)
          : style::DefaultSizes::kNormalThickness;

  auto link = Link{.core_link = core_link,
                   .thickness = thickness,
                   .drop = core::Link::GetDrop(core_link, project)};

  const auto link_alpha =
      linker_.IsRepiningLink(link.core_link.id) ? 0.5F : 1.F;

  if (!settings.color_flow) {
    const auto color = std::visit(
        [&project](const auto& v) {
          using V = std::remove_cvref_t<decltype(v)>;

          if constexpr (std::is_same_v<V, std::monostate>) {
            return ImColor{style::DefaultColors::kWhite};
          } else if constexpr (std::is_same_v<V, core::ConnectionId>) {
            const auto& connection = core::Project::FindConnection(project, v);
            return connection.color;
          } else if constexpr (std::is_same_v<V, core::CustomConnection>) {
            return v.color;
          }
        },
        core_link.connection);

    link.color = style::WithAlpha(color, link_alpha);
    return link;
  }

  const auto start_pin_id = core_link.start_pin_id;
  const auto start_pin_node_id =
      core::Diagram::FindPinNode(*diagram_, start_pin_id).GetId().Get();

  Expects(node_flows.contains(start_pin_node_id));
  const auto& node_flow = node_flows.at(start_pin_node_id);

  const auto start_pin_flow =
      flow::NodeFlow::GetPinFlow(node_flow, start_pin_id);
  const auto end_pin_flow = start_pin_flow + link.drop;

  link.color = style::GetGradient(GetFlowColor(start_pin_flow),
                                  GetFlowColor(end_pin_flow), 0.5);
  link.color.Value.w = link_alpha;

  return link;
}

///
void Diagram::UpdateLinks(const flow::NodeFlows& node_flows) {
  links_.clear();

  const auto& links = diagram_->GetLinks();
  links_.reserve(links.size());

  std::transform(links.cbegin(), links.cend(), std::back_inserter(links_),
                 [this, &node_flows](const auto& link) {
                   return LinkFrom(link, node_flows);
                 });
}

///
auto Diagram::GetHeaderColor(const IHeaderTraits& header_traits,
                             const flow::NodeFlow& node_flow) const {
  const auto& settings = parent_project_->GetProject().GetSettings();

  if (!settings.color_flow) {
    return header_traits.GetColor();
  }

  if (const auto input_flow = node_flow.input_pin_flow) {
    return GetFlowColor(input_flow->second);
  }

  return ImColor{style::DefaultColors::kWhite};
}

///
auto Diagram::PinFrom(const IPinTraits& pin_traits,
                      const flow::NodeFlow& node_flow) const {
  auto pin = Pin{.label = pin_traits.GetLabel()};

  const auto pin_value = pin_traits.GetValue();

  if (std::holds_alternative<float>(pin_value)) {
    pin.value = std::get<float>(pin_value);
  } else if (std::holds_alternative<float*>(pin_value)) {
    pin.value = std::get<float*>(pin_value);
  }

  const auto pin_type = pin_traits.GetPin();

  if (!std::holds_alternative<ne::PinId>(pin_type)) {
    return pin;
  }

  const auto pin_id = std::get<ne::PinId>(pin_type);
  const auto pin_flow = flow::NodeFlow::GetPinFlow(node_flow, pin_id);
  const auto& settings = parent_project_->GetProject().GetSettings();

  pin.flow_data =
      PinFlowData{.id = pin_id,
                  .color = GetFlowColor(pin_flow),
                  .filled = core::Diagram::HasLink(*diagram_, pin_id)};

  if (settings.color_flow && pin.label.has_value()) {
    pin.label->color = pin.flow_data->color;
  }

  if (!linker_.CanConnectToPin(pin_id)) {
    pin.flow_data->color.Value.w = 0.25;
  }

  if (std::holds_alternative<PinFlow>(pin_value)) {
    pin.value = pin_flow;
  }

  return pin;
}

///
auto Diagram::NodeFlowFrom(const core::INode& core_node,
                           const flow::NodeFlow& core_flow) const {
  auto node_flow = NodeFlow{};

  if (const auto& input_flow = core_flow.input_pin_flow) {
    node_flow.input_flow = FlowValue{.value = input_flow->second,
                                     .color = GetFlowColor(input_flow->second)};
  }

  auto& output_flows = node_flow.output_flows;

  for (const auto output_pin : core_node.GetOutputPinIds()) {
    const auto output_flow = flow::NodeFlow::GetPinFlow(core_flow, output_pin);

    if (std::none_of(output_flows.cbegin(), output_flows.cend(),
                     [output_flow](const auto& other_flow) {
                       return other_flow.value == output_flow;
                     })) {
      output_flows.emplace_back(FlowValue{.value = output_flow});
    }
  }

  for (auto& output_flow : output_flows) {
    output_flow.color = GetFlowColor(output_flow.value);
  }

  return node_flow;
}

///
auto Diagram::NodeFrom(core::INode& core_node,
                       const flow::NodeFlow& node_flow) const {
  const auto node_traits = core_node.CreateUiTraits();
  const auto label = node_traits->GetLabel();

  auto node_data =
      NodeData{.label = label + " #" + std::to_string(core_node.GetId().Get()),
               .flow = NodeFlowFrom(core_node, node_flow)};

  if (const auto header_traits = node_traits->CreateHeaderTraits()) {
    node_data.header = Header{
        .label = label, .color = GetHeaderColor(**header_traits, node_flow)};
  }

  for (const auto& pin_traits : node_traits->CreatePinTraits()) {
    auto& pins =
        (IPinTraits::GetPinKind(*pin_traits, core_node) == ne::PinKind::Input)
            ? node_data.input_pins
            : node_data.output_pins;

    pins.emplace_back(PinFrom(*pin_traits, node_flow));
  }

  return Node{safe_owner_.MakeSafe(&core_node), std::move(node_data)};
}

///
void Diagram::UpdateNodes(const flow::NodeFlows& node_flows) {
  nodes_.clear();

  const auto& nodes = diagram_->GetNodes();
  nodes_.reserve(nodes.size());

  std::transform(nodes.cbegin(), nodes.cend(), std::back_inserter(nodes_),
                 [this, &node_flows](const auto& node) {
                   const auto node_id = node->GetId().Get();
                   Expects(node_flows.contains(node_id));
                   return NodeFrom(*node, node_flows.at(node_id));
                 });
}

///
auto Diagram::GetFamilyNodes(core::FamilyId family_id) const {
  auto nodes = std::vector<cpp::SafePtr<const Node>>{};

  for (const auto& node : nodes_) {
    if (node.GetNode().GetFamilyId() == family_id) {
      nodes.emplace_back(safe_owner_.MakeSafe(&node));
    }
  }

  return nodes;
}

///
auto Diagram::FamilyFrom(const core::IFamily& core_family) const {
  return Family{parent_project_, safe_owner_.MakeSafe(&core_family),
                GetFamilyNodes(core_family.GetId())};
}

///
void Diagram::UpdateFamilyGroups() {
  family_groups_.clear();

  for (const auto& core_family : parent_project_->GetProject().GetFamilies()) {
    auto family = FamilyFrom(*core_family);

    const auto group_label = core_family->CreateUiTraits()->GetGroupLabel();
    const auto group =
        std::find_if(family_groups_.begin(), family_groups_.end(),
                     [&group_label](const auto& group) {
                       return group.label == group_label;
                     });

    if (group != family_groups_.cend()) {
      group->families.emplace_back(std::move(family));
      continue;
    }

    auto& new_group =
        family_groups_.emplace_back(FamilyGroup{.label = group_label});
    new_group.families.emplace_back(std::move(family));
  }
}

///
// NOLINTNEXTLINE(*-no-recursion)
void Diagram::UpdateTreeNode(TreeNode& tree_node) {
  tree_node.node->SetTreeNode(safe_owner_.MakeSafe(&tree_node));

  for (auto& child : tree_node.child_nodes) {
    UpdateTreeNode(child);
  }

  const auto& families = parent_project_->GetProject().GetFamilies();
  auto& num_children_per_family = tree_node.num_children_per_family;

  for (const auto& child_node : tree_node.child_nodes) {
    for (const auto& [child_family, child_num_children] :
         child_node.num_children_per_family) {
      num_children_per_family[child_family] += child_num_children;
    }

    const auto child_family = std::find_if(
        families.cbegin(), families.cend(),
        [child_family_id = child_node.node->GetNode().GetFamilyId()](
            const auto& family) { return family->GetId() == child_family_id; });

    Expects(child_family != families.cend());
    ++num_children_per_family[(*child_family)->GetId().Get()];
  }
}

///
void Diagram::UpdateNodeTrees() {
  node_trees_.clear();

  if (flow_trees_.empty()) {
    return;
  }

  auto parent_stack = std::stack<TreeNode*>{};

  for (const auto& root_node : flow_trees_) {
    flow::TraverseDepthFirst(
        root_node,
        [this, &parent_stack](const auto& core_tree_node) {
          auto& node = FindNode(*this, core_tree_node.node_id);
          auto& tree_node = parent_stack.empty()
                                ? node_trees_.emplace_back(
                                      TreeNode{safe_owner_.MakeSafe(&node)})
                                : parent_stack.top()->child_nodes.emplace_back(
                                      TreeNode{safe_owner_.MakeSafe(&node)});
          parent_stack.emplace(&tree_node);
        },
        [&parent_stack](const auto&) { parent_stack.pop(); });
  }

  for (auto& root_node : node_trees_) {
    UpdateTreeNode(root_node);
  }
}

///
auto Diagram::MoveConnectedLinkToNewFreePin(
    ne::PinId pin_id, ne::PinKind pin_kind,
    const coreui::Family& free_pin_family) -> Event& {
  auto free_pin_node = free_pin_family.CreateNode();

  const auto pin_pos = node_mover_.GetPinPos(pin_id);
  free_pin_node->SetPos(pin_pos);

  const auto free_pin_id =
      core::INode::GetFirstPinOfKind(*free_pin_node, pin_kind);

  AddNode(std::move(free_pin_node));

  return MoveLink(pin_id, free_pin_id)
      .Then([node_mover = safe_owner_.MakeSafe(&node_mover_), free_pin_id,
             pin_pos]() { node_mover->MovePinTo(free_pin_id, pin_pos); });
}
}  // namespace vh::ponc::coreui