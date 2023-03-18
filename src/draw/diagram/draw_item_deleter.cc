#include "draw_item_deleter.h"

#include <concepts>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>

#include "core_concepts.h"
#include "core_link.h"
#include "coreui_diagram.h"
#include "imgui_node_editor.h"

namespace esc::draw {
namespace {
///
template <typename T>
void UnregisterDeletedItemsImpl(
    ItemDeleter::ItemIds &item_ids, const std::vector<T> &items,
    const std::invocable<const T &> auto &get_item_id,
    const std::invocable<uintptr_t> auto &unregister_item) {
  auto new_item_ids = std::set<uintptr_t>{};
  std::transform(items.begin(), items.end(),
                 std::inserter(new_item_ids, new_item_ids.begin()),
                 get_item_id);

  auto deleted_item_ids = std::vector<uintptr_t>{};
  std::set_difference(item_ids.registered_ids_.begin(),
                      item_ids.registered_ids_.end(), new_item_ids.begin(),
                      new_item_ids.end(), std::back_inserter(deleted_item_ids));

  for (const auto deleted_id : deleted_item_ids) {
    unregister_item(deleted_id);
    item_ids.unregistered_ids_.insert(deleted_id);
  }

  item_ids.registered_ids_ = std::move(new_item_ids);
}

///
template <core::Id T>
void DeleteUnregisteredItemsImpl(
    ItemDeleter::ItemIds &item_ids,
    const std::invocable<T *> auto &query_deleted_item,
    const std::invocable<T> auto &delete_item) {
  auto item_id = T{};

  while (query_deleted_item(&item_id)) {
    if (!ne::AcceptDeletedItem()) {
      continue;
    }

    const auto item_id_value = item_id.Get();

    if (item_ids.unregistered_ids_.contains(item_id_value)) {
      item_ids.unregistered_ids_.erase(item_id_value);
      continue;
    }

    delete_item(item_id);
    item_ids.registered_ids_.erase(item_id_value);
  }
}
}  // namespace

///
void ItemDeleter::UnregisterDeletedItems(const core::Diagram &diagram) {
  UnregisterDeletedItemsImpl(
      link_ids_, diagram.GetLinks(),
      [](const auto &link) { return link.id.Get(); }, &ne::DeleteLink);

  UnregisterDeletedItemsImpl(
      node_ids_, diagram.GetNodes(),
      [](const auto &node) { return node->GetId().Get(); }, &ne::DeleteNode);
}

///
void ItemDeleter::DeleteUnregisteredItems(coreui::Diagram &diagram) {
  if (ne::BeginDelete()) {
    DeleteUnregisteredItemsImpl<ne::LinkId>(
        link_ids_,
        [](const auto link_id) { return ne::QueryDeletedLink(link_id); },
        std::bind_front(&coreui::Diagram::DeleteLink, &diagram));

    DeleteUnregisteredItemsImpl<ne::NodeId>(
        node_ids_, &ne::QueryDeletedNode,
        std::bind_front(&coreui::Diagram::DeleteNode, &diagram));
  }

  ne::EndDelete();
}
}  // namespace esc::draw