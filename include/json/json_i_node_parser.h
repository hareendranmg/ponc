/**
 * PON Calculator https://github.com/qoala101/ponc
 * @author Volodymyr Hromakov (4y5t6r@gmail.com)
 * @copyright Copyright (c) 2023, MIT License
 */

#ifndef VH_PONC_JSON_I_NODE_PARSER_H_
#define VH_PONC_JSON_I_NODE_PARSER_H_

#include <memory>

#include "core_i_node.h"
#include "cpp_non_copyable.h"
#include "crude_json.h"

namespace vh::ponc::json {
///
class INodeParser : public cpp::NonCopyable {
 public:
  ///
  auto ParseFromJson(const crude_json::value &json) const
      -> std::unique_ptr<core::INode>;

 private:
  ///
  virtual auto ParseFromJson(const core::INode::ConstructorArgs &parsed_args,
                             const crude_json::value &json) const
      -> std::unique_ptr<core::INode> = 0;
};
}  // namespace vh::ponc::json

#endif  // VH_PONC_JSON_I_NODE_PARSER_H_
