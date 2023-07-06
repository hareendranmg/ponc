/**
 * PONC @link https://github.com/qoala101/ponc @endlink
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 * @copyright Copyright (c) 2023, MIT License
 */

#ifndef VH_PONC_DRAW_NODE_POPUP_H_
#define VH_PONC_DRAW_NODE_POPUP_H_

#include "coreui_diagram.h"
#include "draw_area_popup.h"
#include "draw_i_popup.h"

namespace vh::ponc::draw {
///
class NodePopup : public IPopup {
 public:
  ///
  void Draw(coreui::Diagram &diagram);

 private:
  ///
  AreaPopup area_popup_{};
};
}  // namespace vh::ponc::draw

#endif  // VH_PONC_DRAW_NODE_POPUP_H_
