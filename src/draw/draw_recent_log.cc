#include "draw_recent_log.h"

#include "draw_log_view.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_node_editor.h>

namespace vh::ponc::draw {
///
void RecentLog::Draw(const coreui::Log& log) {
  const auto* viewport = ImGui::GetMainViewport();
  const auto padding = ImGui::GetStyle().WindowPadding * 2;
  const auto window_pos =
      viewport->WorkPos + ImVec2{padding.x, viewport->WorkSize.y - padding.y};
  const auto window_pivot = ImVec2{0.F, 1.F};

  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pivot);

  // NOLINTBEGIN(*-signed-bitwise)
  const auto window_flags =
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
      ImGuiWindowFlags_NoDecoration;
  // NOLINTEND(*-signed-bitwise)

  if (ImGui::Begin("Recent Log", &opened_, window_flags)) {
    LogView::DrawMessages(log.GetRecentMessages(), false);

    if (ImGui::BeginPopupContextWindow()) {
      if (ImGui::MenuItem("Close")) {
        opened_ = false;
      }

      ImGui::EndPopup();
    }
  }

  ImGui::End();
}
}  // namespace vh::ponc::draw