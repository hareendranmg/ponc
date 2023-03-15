#include "draw_create_node_popup.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "coreui_diagram.h"
#include "coreui_family.h"
#include "draw_family_groups_menu.h"
#include "imgui.h"

namespace esc::draw {
///
void CreateNodePopup::Draw(coreui::Diagram& diagram) {
  const auto content_scope = DrawContentScope("Crate Node");

  if (!IsOpened()) {
    return;
  }

  FamilyGroupsMenu::Draw(
      diagram.GetFamilyGroups(),
      {.family_selected = [&diagram, pos = pos_](const auto& family) {
        auto new_node = family.CreateNode();
        new_node->SetPos(pos);
        diagram.AddNode(std::move(new_node));
      }});
}

///
void CreateNodePopup::SetPos(const ImVec2& pos) { pos_ = pos; }
}  // namespace esc::draw