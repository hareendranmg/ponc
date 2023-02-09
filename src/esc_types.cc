#include "esc_types.h"

#include <cstdint>

#include "imgui_node_editor.h"

namespace ne = ax::NodeEditor;

Pin::Pin(ne::PinId id, std::string name, PinType type, PinKind kind, Node* node)
    : ID{id},
      node{node},
      Name{std::move(name)},
      Type{type},
      Kind{kind} {}
// vh: ok
auto CanCreateLink(const Pin* left, const Pin* right) -> bool {
  return (left != nullptr) && (right != nullptr) && (left != right) &&
         (left->Kind != right->Kind) && (left->Type == right->Type) &&
         (left->node != right->node);
}

Node::Node(ne::NodeId id, std::string name, ImColor color)
    : ID{id}, Name{std::move(name)}, Color{color}, Size{0, 0} {}

auto GetCouplerPercentageNames() -> const std::vector<std::string>& {
  static auto kNames = std::vector<std::string>{
      "05%%-95%%", "10%%-90%%", "15%%-85%%", "20%%-80%%", "25%%-75%%",
      "30%%-70%%", "35%%-65%%", "40%%-60%%", "45%%-55%%", "50%%-50%%"};
  return kNames;
}

auto GetCouplerPercentageValues()
    -> const std::vector<std::pair<float, float>>& {
  static auto kValues = std::vector<std::pair<float, float>>{
      {13.80, 00.40}, {10.60, 00.70}, {08.80, 00.95}, {07.50, 01.20},
      {06.50, 01.55}, {05.70, 01.85}, {05.00, 02.20}, {04.40, 02.60},
      {03.90, 03.00}, {03.40, 03.40},
  };
  return kValues;
}