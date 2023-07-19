/**
 * PONC @link https://github.com/qoala101/ponc @endlink
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 * @copyright Copyright (c) 2023, MIT License
 */

#include "draw_edit_link_popup.h"

#include <imgui.h>
#include <imgui_node_editor.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core_connection.h"
#include "core_diagram.h"
#include "core_link.h"
#include "core_project.h"
#include "style_default_colors.h"
#include "style_utils.h"

namespace vh::ponc::draw {
namespace {
///
auto GetCustomConnection(core::Link& link) {
  return std::visit(
      [](auto& v) -> std::optional<core::CustomConnection*> {
        using V = std::remove_cvref_t<decltype(v)>;

        if constexpr (std::is_same_v<V, core::CustomConnection>) {
          return &v;
        } else {
          return {};
        }
      },
      link.connection);
}

///
auto GetConnection(core::Link& link, const core::Project& project) {
  return std::visit(
      [&project](auto& v) -> std::optional<const core::Connection*> {
        using V = std::remove_cvref_t<decltype(v)>;

        if constexpr (std::is_same_v<V, core::ConnectionId>) {
          return &core::Project::FindConnection(project, v);
        } else {
          return {};
        }
      },
      link.connection);
}
}  // namespace

///
auto EditLinkPopup::IsDefaultConnection() const {
  return connection_index_ == 0;
}

///
void EditLinkPopup::Draw(coreui::Diagram& diagram,
                         const core::Project& project) {
  const auto content_scope = DrawContentScope();

  if (!IsOpened()) {
    return;
  }

  auto& link = core::Diagram::FindLink(diagram.GetDiagram(), link_id_);
  const auto& connections = project.GetConnections();

  if (WasJustOpened()) {
    link_copy_ = link;
    custom_connection_copy_ =
        core::CustomConnection{.color = style::GenerateBrightColor()};

    CopyConnections(connections);
  }

  ImGui::InputFloat("Length", &link.length);

  const auto custom_connection = GetCustomConnection(link);
  const auto connection = GetConnection(link, project);

  if (custom_connection.has_value()) {
    ImGui::ColorEdit3(
        "##Color", &(*custom_connection)->color.Value.x,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
  } else {
    const auto color = connection.has_value()
                           ? (*connection)->color
                           : ImColor{style::DefaultColors::kWhite};
    ImGui::ColorButton("##Color", color, ImGuiColorEditFlags_NoAlpha);
  }

  const auto color_edit_width =
      ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;

  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - color_edit_width);

  if (ImGui::Combo("Connection", &connection_index_, connection_names_.data(),
                   static_cast<int>(connection_names_.size()))) {
    SetSelectedConnection(link, connections);
  }

  if (custom_connection.has_value()) {
    ImGui::InputFloat("Attenuation/Length",
                      &(*custom_connection)->drop_per_length);
    ImGui::InputFloat("Attenuation Added", &(*custom_connection)->drop_added);
  } else {
    const auto drop_per_length =
        connection.has_value() ? (*connection)->drop_per_length : 0.F;
    ImGui::LabelText("Attenuation/Length", "%.2f", drop_per_length);

    const auto drop_added =
        connection.has_value() ? (*connection)->drop_added : 0.F;
    ImGui::LabelText("Attenuation Added", "%.2f", drop_added);
  }

  if (ImGui::Button("Cancel")) {
    link = link_copy_;
    ImGui::CloseCurrentPopup();
  }
}

///
void EditLinkPopup::SetLinkId(ne::LinkId link_id) { link_id_ = link_id; }

///
void EditLinkPopup::CopyConnections(
    const std::vector<core::Connection>& connections) {
  connection_names_ = {"<None>"};
  connection_names_.reserve(connection_names_.size() + connections.size() + 1);

  std::transform(
      connections.cbegin(), connections.cend(),
      std::back_inserter(connection_names_),
      [](const auto& connection) { return connection.name.c_str(); });

  connection_names_.emplace_back("<Custom>");
  connection_index_ = std::visit(
      [&link_copy = link_copy_, &connections](const auto& v) {
        using V = std::remove_cvref_t<decltype(v)>;

        if constexpr (std::is_same_v<V, std::monostate>) {
          return 0;
        } else if constexpr (std::is_same_v<V, core::ConnectionId>) {
          const auto connection_id =
              std::get<core::ConnectionId>(link_copy.connection);

          const auto connection =
              std::find_if(connections.cbegin(), connections.cend(),
                           [connection_id](const auto& connection) {
                             return connection.id == connection_id;
                           });

          return static_cast<int>(connection - connections.cbegin()) + 1;
        } else if constexpr (std::is_same_v<V, core::CustomConnection>) {
          return static_cast<int>(connections.size()) + 1;
        }
      },
      link_copy_.connection);
}

///
void EditLinkPopup::SetSelectedConnection(
    core::Link& link, const std::vector<core::Connection>& connections) {
  if (const auto link_custom_connection = GetCustomConnection(link)) {
    custom_connection_copy_ = **link_custom_connection;
  }

  if (IsDefaultConnection()) {
    link.connection = {};
    return;
  }

  if (connection_index_ > static_cast<int>(connections.size())) {
    link.connection = custom_connection_copy_;
    return;
  }

  const auto& connection = connections[connection_index_ - 1];
  link.connection = connection.id;
}
}  // namespace vh::ponc::draw