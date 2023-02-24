#ifndef VH_DRAW_WIDGETS_H_
#define VH_DRAW_WIDGETS_H_

#include <imgui_node_editor.h>

#include <optional>
#include <unordered_map>

#include "app_event_queue.h"
#include "app_state.h"
#include "core_id_generator.h"
#include "core_project.h"
#include "draw_background_popup.h"
#include "draw_link_popup.h"
#include "draw_new_link_popup.h"
#include "draw_node_editor.h"
#include "draw_node_popup.h"
#include "draw_open_file_dialog.h"
#include "draw_save_as_file_dialog.h"
#include "draw_texture.h"

namespace esc {
class EventQueue;

namespace core {
class Project;
class IdGenerator;
}  // namespace core

namespace flow {
class FlowCalculator;
}  // namespace flow

namespace draw {
struct Widgets {
  void Draw(const AppState &app_state);

  OpenFileDialog open_file_dialog{};
  SaveAsFileDialog save_as_file_dialog{};

  //   // FamiliesView families_view{};
  //   // FlowTreeView flow_tree_view{};
  //   // GroupsView groups_view{};
  //   // GroupSettingsView group_settings_view{};
  SettingsView settings_view{};

  NodeEditor node_editor{};
};
}  // namespace draw
}  // namespace esc

#endif  // VH_DRAW_WIDGETS_H_
