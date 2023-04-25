#include "draw_diagrams_view.h"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>

#include "core_diagram.h"
#include "cpp_assert.h"
#include "cpp_scope.h"
#include "cpp_scope_function.h"
#include "draw_disable_if.h"
#include "draw_id_label.h"

namespace vh::ponc::draw {
///
auto DiagramsView::GetLabel() const -> std::string { return "Diagrams"; }

///
auto DiagramsView::GetTrimmedRenameBuffer() const {
  // NOLINTNEXTLINE(*-redundant-string-cstr)
  return std::string{rename_buffer_.c_str()};
}

///
auto DiagramsView::DrawRenamePopup() {
  auto rename_confirmed = false;

  if (ImGui::BeginPopup("Rename Diagram")) {
    ImGui::InputText("##Diagram Name", rename_buffer_.data(),
                     rename_buffer_.size());
    ImGui::SameLine();

    {
      const auto disable_scope = DisableIf(GetTrimmedRenameBuffer().empty());

      if (ImGui::Button("OK")) {
        rename_confirmed = true;
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }

  return rename_confirmed;
}

///
auto DiagramsView::DrawControls(coreui::Project& project) {
  auto selected_action = std::optional<Action>{};

  ImGui::BeginHorizontal("Controls");

  if (ImGui::Button("Create")) {
    project.AddDiagram(core::Diagram{"New Diagram"});
  }

  if (ImGui::Button("Clone")) {
    selected_action = Action::kClone;
  }

  if (ImGui::Button("Rename")) {
    selected_action = Action::kStartRenaming;
    ImGui::OpenPopup("Rename Diagram");
  }

  if (const auto rename_confirmed = DrawRenamePopup()) {
    selected_action = Action::kConfirmRename;
  }

  const auto& diagrams = project.GetProject().GetDiagrams();

  {
    const auto disable_scope = DisableIf(diagrams.size() <= 1);

    if (ImGui::Button("Delete")) {
      selected_action = Action::kDelete;
    }
  }

  ImGui::EndHorizontal();

  return selected_action;
}

///
void DiagramsView::Draw(coreui::Project& project) {
  const auto content_scope = DrawContentScope();

  if (!IsOpened()) {
    return;
  }

  const auto control_action = DrawControls(project);
  DrawDiagrams(project, control_action);
}

///
void DiagramsView::ApplyAction(coreui::Project& project, int diagram_index,
                               Action action) {
  Expects(diagram_index >= 0);

  auto& diagrams = project.GetProject().GetDiagrams();
  Expects(diagram_index < static_cast<int>(diagrams.size()));

  auto& diagram = diagrams[diagram_index];

  switch (action) {
    case Action::kClone:
      project.CloneDiagram(diagram);
      break;
    case Action::kStartRenaming: {
      rename_buffer_ = diagram.GetName();

      static constexpr auto kMaxNameLength = 256;
      rename_buffer_.resize(kMaxNameLength);
      break;
    }
    case Action::kConfirmRename:
      diagram.SetName(GetTrimmedRenameBuffer());
      break;
    case Action::kDelete:
      project.DeleteDiagram(diagram_index);
      break;
  }
}

///
void DiagramsView::DrawDiagrams(coreui::Project& project,
                                std::optional<Action> selected_action) {
  // NOLINTBEGIN(*-signed-bitwise)
  const auto table_flags = ImGuiTableFlags_BordersH |
                           ImGuiTableFlags_BordersOuter |
                           ImGuiTableFlags_ScrollY;
  // NOLINTEND(*-signed-bitwise)

  if (ImGui::BeginTable("Diagrams", 1, table_flags)) {
    auto& diagrams = project.GetProject().GetDiagrams();

    for (auto i = 0; i < static_cast<int>(diagrams.size()); ++i) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      auto& diagram = diagrams[i];
      const auto item_is_selected =
          &diagram == &project.GetDiagram().GetDiagram();

      if (ImGui::Selectable(IdLabel(i, diagram.GetName()).c_str(),
                            item_is_selected) &&
          !item_is_selected) {
        project.SetDiagram(i);
      }

      if (item_is_selected && selected_action.has_value()) {
        ApplyAction(project, i, *selected_action);
      }
    }

    ImGui::EndTable();
  }
}
}  // namespace vh::ponc::draw