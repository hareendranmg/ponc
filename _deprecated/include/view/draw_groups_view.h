#ifndef VH_DRAW_GROUPS_VIEW_H_
#define VH_DRAW_GROUPS_VIEW_H_

#include <string>

#include "draw_i_hidable.h"

namespace esc {
class State;
}  // namespace esc

namespace esc::draw {
class GroupsView : public IHidable {
 public:
  auto GetLabel() const -> std::string;
  void Draw(coreui::Frame &frame);
};
}  // namespace esc::draw

#endif  // VH_DRAW_GROUPS_VIEW_H_
