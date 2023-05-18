/**
 * PON Calculator @link https://github.com/qoala101/ponc @endlink
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 * @copyright Copyright (c) 2023, MIT License
 */

#include "draw_i_popup.h"

#include <imgui.h>
#include <imgui_node_editor.h>

#include "cpp_scope_function.h"
#include "draw_id_label.h"

namespace ne = ax::NodeEditor;

namespace vh::ponc::draw {
namespace {
///
auto GenerateId() {
  static auto id = ImGuiID{};
  return id++;
}
}  // namespace

//
auto IPopup::IsOpened() const -> bool { return opened_; }

///
void IPopup::Open() {
  ImGui::OpenPopup(IdLabel(id_).c_str(),
                   ImGuiPopupFlags_NoOpenOverExistingPopup);
  opened_ = true;
}

///
IPopup::IPopup() : id_{GenerateId()} {}

///
auto IPopup::DrawContentScope(std::string_view title,
                              const Callbacks &callbacks)
    -> cpp::ScopeFunction {
  ne::Suspend();

  if (ImGui::BeginPopup(IdLabel(id_).c_str())) {
    if (!title.empty()) {
      ImGui::TextUnformatted(title.data());
      ImGui::Separator();
    }

    return cpp::ScopeFunction{[]() {
      ImGui::EndPopup();
      ne::Resume();
    }};
  }

  if (opened_) {
    opened_ = false;

    if (callbacks.closed.has_value()) {
      (*callbacks.closed)();
    }
  }

  return cpp::ScopeFunction{[]() { ne::Resume(); }};
}
}  // namespace vh::ponc::draw