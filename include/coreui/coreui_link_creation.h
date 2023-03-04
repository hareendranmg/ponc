#ifndef VH_COREUI_LINK_CREATION_H_
#define VH_COREUI_LINK_CREATION_H_

#include <imgui_node_editor.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "core_i_node.h"
#include "core_link.h"
#include "cpp_callbacks.h"

namespace esc::coreui {
///
class LinkCreation {
 public:
  ///
  struct Callbacks {
    ///
    cpp::Query<const core::INode&, ne::PinId> find_pin_node{};
    ///
    cpp::Query<std::optional<const core::Link*>, ne::PinId> find_pin_link{};
    ///
    cpp::Action<void(ne::PinId, ne::PinId)> create_link{};
    ///
    cpp::Action<void(ne::LinkId)> delete_link{};
  };

  ///
  explicit LinkCreation(Callbacks callbacks);

  ///
  void SetPins(const std::optional<ne::PinId>& dragged_from_pin,
               const std::optional<ne::PinId>& hovering_over_pin = {});
  ///
  auto IsCreatingLink() const -> bool;
  ///
  auto CanConnectToPin(ne::PinId pin_id) const -> bool;
  ///
  auto IsHoveringOverPin() const -> bool;
  ///
  auto CanCreateLink() const -> bool;
  ///
  auto GetCanCreateLinkReason() const -> std::pair<bool, std::string>;
  ///
  auto IsRepinningLink() const -> bool;
  ///
  auto IsLinkBeingRepinned(ne::LinkId link_id) const -> bool;
  ///
  auto GetCurrentLinkSourcePin() const -> std::pair<ne::PinId, ne::PinKind>;
  ///
  void AcceptCurrentLink() const;

 private:
  ///
  struct HoveringData {
    ///
    ne::PinId hovering_over_pin{};
    ///
    ne::PinKind hovering_over_pin_kind{};
    ///
    bool can_connect_to_hovered_pin{};
    ///
    std::string reason{};
  };

  ///
  struct RepinningData {
    ///
    ne::LinkId link_to_repin{};
    ///
    ne::PinId fixed_pin{};
    ///
    ne::PinKind fixed_pin_kind{};
    ///
    ne::NodeId fixed_pin_node{};
  };

  ///
  struct CreatingData {
    ///
    ne::PinId dragged_from_pin{};
    ///
    ne::NodeId dragged_from_node{};
    ///
    ne::PinKind dragged_from_pin_kind{};
    ///
    std::optional<HoveringData> hovering_data{};
    ///
    std::optional<RepinningData> repinning_data{};
  };

  ///
  auto GetCanConnectToPinReason(ne::PinId pin_id) const
      -> std::pair<bool, std::string>;

  ///
  Callbacks callbacks_{};
  ///
  std::optional<CreatingData> creating_data_{};
};
}  // namespace esc::coreui

#endif  // VH_COREUI_LINK_CREATION_H_
