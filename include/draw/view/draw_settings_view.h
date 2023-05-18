/**
 * PON Calculator https://github.com/qoala101/ponc
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 * @copyright Copyright (c) 2023, MIT License
 */

#ifndef VH_PONC_DRAW_SETTINGS_VIEW_H_
#define VH_PONC_DRAW_SETTINGS_VIEW_H_

#include <string>

#include "core_settings.h"
#include "draw_i_view.h"

namespace vh::ponc::draw {
///
class SettingsView : public IView {
 public:
  ///
  auto GetLabel() const -> std::string override;

  ///
  void Draw(core::Settings &settings);
};
}  // namespace vh::ponc::draw

#endif  // VH_PONC_DRAW_SETTINGS_VIEW_H_
