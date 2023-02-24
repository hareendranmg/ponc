/**
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 */

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include "cpp_assert.h"
#include "draw_links.h"
#include "frame_node.h"
#include "imgui_bezier_math.h"
#include "imgui_node_editor.h"

namespace esc::draw {
namespace {
auto GetCurve(const ImVec2& m_Start, const ImVec2& m_End) {
  const auto strength = ne::GetStyle().LinkStrength;
  const auto start_dir = ne::GetStyle().SourceDirection;
  const auto end_dir = ne::GetStyle().TargetDirection;

  auto easeLinkStrength = [](const ImVec2& a, const ImVec2& b, float strength) {
    const auto distanceX = b.x - a.x;
    const auto distanceY = b.y - a.y;
    const auto distance = ImSqrt(distanceX * distanceX + distanceY * distanceY);
    const auto halfDistance = distance * 0.5f;

    if (halfDistance < strength) {
      strength = strength * ImSin(IM_PI * 0.5f * halfDistance / strength);
    }

    return strength;
  };

  const auto startStrength = easeLinkStrength(m_Start, m_End, strength);
  const auto endStrength = easeLinkStrength(m_Start, m_End, strength);
  const auto cp0 = m_Start + start_dir * startStrength;
  const auto cp1 = m_End + end_dir * endStrength;

  ImCubicBezierPoints result;
  result.P0 = m_Start;
  result.P1 = cp0;
  result.P2 = cp1;
  result.P3 = m_End;

  return result;
}

void DrawLinkBeingRepinned_v2(const frame::Curve& curve) {
  const auto mouse_pos = ImGui::GetMousePos();
  const auto bezier = GetCurve(curve.start_position.value_or(mouse_pos),
                               curve.end_position.value_or(mouse_pos));

  auto* draw_list = ImGui::GetWindowDrawList();
  Expects(draw_list != nullptr);
  draw_list->AddBezierCubic(bezier.P0, bezier.P1, bezier.P2, bezier.P3,
                            curve.color, curve.thickness);
}
}  // namespace

// ---
void Links::Draw(const frame::Frame& frame) {
  for (const auto& link : frame.links) {
    ne::Link(link.id, link.start_pin_id, link.end_pin_id, link.color,
             link.thickness);
  }

  if (frame.curve.has_value()) {
    DrawLinkBeingRepinned_v2(*frame.curve);
  }
}
}  // namespace esc::draw