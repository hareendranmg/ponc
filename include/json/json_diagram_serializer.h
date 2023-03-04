#ifndef VH_JSON_DIAGRAM_SERIALIZER_H_
#define VH_JSON_DIAGRAM_SERIALIZER_H_

#include <memory>

#include "core_diagram.h"
#include "core_i_family.h"
#include "cpp_no_instances.h"
#include "crude_json.h"

namespace esc::json {
///
struct DiagramSerializer : public cpp::NoInstances {
  ///
  static auto ParseFromJson(
      const crude_json::value &json,
      const std::vector<std::unique_ptr<core::IFamily>> &families)
      -> core::Diagram;
  ///
  static auto WriteToJson(const core::Diagram &diagram) -> crude_json::value;
};
}  // namespace esc::json

#endif  // VH_JSON_DIAGRAM_SERIALIZER_H_
