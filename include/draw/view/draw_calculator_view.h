#ifndef VH_PONC_DRAW_CALCULATOR_VIEW_H_
#define VH_PONC_DRAW_CALCULATOR_VIEW_H_

#include <cstdint>
#include <string>
#include <vector>

#include "core_i_family.h"
#include "core_project.h"
#include "coreui_diagram.h"
#include "coreui_family.h"
#include "cpp_callbacks.h"
#include "draw_i_view.h"
#include "flow_calculator.h"
#include "flow_tree.h"

namespace vh::ponc::draw {
class CalculatorView : public IView {
 public:
  struct Callbacks {
    cpp::Signal<std::vector<core::Diagram>> calculated_diagrams{};
  };

  auto GetLabel() const -> std::string override;

  void Draw(core::Project &project, const Callbacks &callbacks);

 private:
  std::vector<flow::FamilyFlow> family_flows_{};

  int input_{6000};
  int min_output_{-2200};
  int max_output_{-1800};
  int num_clients_{6};
};
}  // namespace vh::ponc::draw

#endif  // VH_PONC_DRAW_CALCULATOR_VIEW_H_
