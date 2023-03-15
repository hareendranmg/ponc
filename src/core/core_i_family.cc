#include "core_i_family.h"

#include <optional>

#include "core_i_node.h"
#include "core_id_generator.h"

namespace esc::core {
///
auto IFamily::GetType() const -> std::optional<FamilyType> {
  return std::nullopt;
}

///
auto IFamily::GetId() const -> FamilyId { return id_; }

///
auto IFamily::CreateSampleNode() const -> std::unique_ptr<INode> {
  auto id_generator = IdGenerator{};
  return CreateNode(id_generator);
}

///
IFamily::IFamily(FamilyId id) : id_{id} {}
}  // namespace esc::core