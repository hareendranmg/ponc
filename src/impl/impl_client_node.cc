#include "impl_client_node.h"

#include <memory>

#include "core_i_node.h"
#include "cpp_assert.h"
#include "crude_json.h"
#include "draw_flow_input_pin_drawer.h"
#include "draw_i_node_drawer.h"
#include "draw_i_node_factory_drawer.h"
#include "draw_i_pin_drawer.h"
#include "esc_id_generator.h"
#include "imgui_node_editor.h"
#include "json_node_serializer.h"

namespace esc::impl {
namespace {
class Node;
class NodeFactory;

constexpr auto kTypeName = "ClientNode";

auto CreateNodeWriter(std::shared_ptr<Node> node)
    -> std::unique_ptr<json::INodeWriter>;
auto CreateNodeDrawer(std::shared_ptr<Node> node)
    -> std::unique_ptr<draw::INodeDrawer>;
auto CreateNodeFactoryWriter(std::shared_ptr<NodeFactory> node_factory)
    -> std::unique_ptr<json::INodeFactoryWriter>;
auto CreateNodeFactoryDrawer(std::shared_ptr<NodeFactory> node_factory)
    -> std::unique_ptr<draw::INodeFactoryDrawer>;

// NOLINTNEXTLINE(*-multiple-inheritance)
class Node : public core::INode, public std::enable_shared_from_this<Node> {
 public:
  Node(ne::NodeId id, std::vector<ne::PinId> pin_ids, float min = {},
       float max = {})
      : INode{id, std::move(pin_ids)}, min_{min}, max_{max} {}

  auto CreateWriter() -> std::unique_ptr<json::INodeWriter> override {
    return CreateNodeWriter(shared_from_this());
  }

  auto CreateDrawer() -> std::unique_ptr<draw::INodeDrawer> override {
    return CreateNodeDrawer(shared_from_this());
  }

  float min_{};
  float max_{};
};

class NodeParser : public json::INodeParser {
 private:
  auto GetTypeName() const -> std::string override { return kTypeName; }

  auto ParseFromJson(ne::NodeId parsed_node_id,
                     std::vector<ne::PinId> parsed_pin_ids,
                     const crude_json::value& json) const
      -> std::shared_ptr<core::INode> override {
    return std::make_shared<Node>(parsed_node_id, std::move(parsed_pin_ids),
                                  json["min"].get<crude_json::number>(),
                                  json["max"].get<crude_json::number>());
  }
};

class NodeWriter : public json::INodeWriter {
 public:
  explicit NodeWriter(std::shared_ptr<Node> node) : node_{std::move(node)} {}

 private:
  auto GetTypeName() const -> std::string override { return kTypeName; }

  auto WriteToJson() const -> crude_json::value override {
    auto json = crude_json::value{};
    json["min"] = static_cast<crude_json::number>(node_->min_);
    json["max"] = static_cast<crude_json::number>(node_->max_);
    return json;
  }

 private:
  std::shared_ptr<Node> node_{};
};

auto CreateNodeWriter(std::shared_ptr<Node> node)
    -> std::unique_ptr<json::INodeWriter> {
  return std::make_unique<NodeWriter>(std::move(node));
}

class MinPinDrawer : public draw::IPinDrawer {
 public:
  explicit MinPinDrawer(std::shared_ptr<Node> node) : node_{std::move(node)} {}

  auto GetLabel [[nodiscard]] () const -> std::string override { return "min"; }

  auto GetKind [[nodiscard]] () const -> ne::PinKind override {
    return ne::PinKind::Input;
  }

  auto GetFloat [[nodiscard]] () -> float* override { return &node_->min_; }

  auto IsEditable [[nodiscard]] () const -> bool override { return true; }

 private:
  std::shared_ptr<Node> node_{};
};

class MaxPinDrawer : public draw::IPinDrawer {
 public:
  explicit MaxPinDrawer(std::shared_ptr<Node> node) : node_{std::move(node)} {}

  auto GetLabel [[nodiscard]] () const -> std::string override { return "max"; }

  auto GetKind [[nodiscard]] () const -> ne::PinKind override {
    return ne::PinKind::Input;
  }

  auto GetFloat [[nodiscard]] () -> float* override { return &node_->max_; }

  auto IsEditable [[nodiscard]] () const -> bool override { return true; }

 private:
  std::shared_ptr<Node> node_{};
};

class NodeDrawer : public draw::INodeDrawer {
 public:
  explicit NodeDrawer(std::shared_ptr<Node> node) : node_{std::move(node)} {}

  auto GetLabel() const -> std::string override {
    return ClientNode::CreateNodeFactory()->CreateDrawer()->GetLabel();
  }

  auto GetColor() const -> ImColor override {
    return ClientNode::CreateNodeFactory()->CreateDrawer()->GetColor();
  }

  auto CreatePinDrawer(ne::PinId pin_id) const
      -> std::unique_ptr<draw::IPinDrawer> override {
    const auto pin_index = node_->GetPinIndex(pin_id);

    if (pin_index == 0) {
      return std::make_unique<draw::FlowInputPinDrawer>();
    }

    if (pin_index == 1) {
      return std::make_unique<MinPinDrawer>(node_);
    }

    if (pin_index == 2) {
      return std::make_unique<MaxPinDrawer>(node_);
    }

    cpp::Expects(false);
  }

 private:
  std::shared_ptr<Node> node_{};
};

auto CreateNodeDrawer(std::shared_ptr<Node> node)
    -> std::unique_ptr<draw::INodeDrawer> {
  return std::make_unique<NodeDrawer>(std::move(node));
}

// NOLINTNEXTLINE(*-multiple-inheritance)
class NodeFactory : public core::INodeFactory,
                    public std::enable_shared_from_this<NodeFactory> {
 public:
  auto CreateNode(IdGenerator& id_generator)
      -> std::shared_ptr<core::INode> override {
    return std::make_shared<Node>(
        id_generator.GetNext<ne::NodeId>(),
        std::vector<ne::PinId>{id_generator.GetNext<ne::PinId>(),
                               id_generator.GetNext<ne::PinId>(),
                               id_generator.GetNext<ne::PinId>()});
  }

  auto CreateNodeParser() -> std::unique_ptr<json::INodeParser> override {
    return std::make_unique<NodeParser>();
  }

  auto CreateWriter() -> std::unique_ptr<json::INodeFactoryWriter> override {
    return CreateNodeFactoryWriter(shared_from_this());
  }

  auto CreateDrawer() -> std::unique_ptr<draw::INodeFactoryDrawer> override {
    return CreateNodeFactoryDrawer(shared_from_this());
  }
};

class NodeFactoryParser : public json::INodeFactoryParser {
 public:
  auto GetTypeName() const -> std::string override { return kTypeName; }

  auto ParseFromJson(const crude_json::value& json) const
      -> std::shared_ptr<core::INodeFactory> override {
    return std::make_shared<NodeFactory>();
  }
};

class NodeFactoryWriter : public json::INodeFactoryWriter {
 public:
  explicit NodeFactoryWriter(std::shared_ptr<NodeFactory> node_factory)
      : node_factory_{std::move(node_factory)} {}

  auto GetTypeName() const -> std::string override { return kTypeName; }

  auto WriteToJson() const -> crude_json::value override { return {}; }

 private:
  std::shared_ptr<NodeFactory> node_factory_{};
};

auto CreateNodeFactoryWriter(std::shared_ptr<NodeFactory> node_factory)
    -> std::unique_ptr<json::INodeFactoryWriter> {
  return std::make_unique<NodeFactoryWriter>(std::move(node_factory));
}

class NodeFactoryDrawer : public draw::INodeFactoryDrawer {
 public:
  explicit NodeFactoryDrawer(std::shared_ptr<NodeFactory> node_factory)
      : node_factory_{std::move(node_factory)} {}

  auto GetLabel() const -> std::string override { return "Client"; }

  auto GetColor() const -> ImColor override { return {0, 255, 0}; }

 private:
  std::shared_ptr<NodeFactory> node_factory_{};
};

auto CreateNodeFactoryDrawer(std::shared_ptr<NodeFactory> node_factory)
    -> std::unique_ptr<draw::INodeFactoryDrawer> {
  return std::make_unique<NodeFactoryDrawer>(std::move(node_factory));
}
}  // namespace

auto ClientNode::CreateNodeFactory() -> std::shared_ptr<core::INodeFactory> {
  return std::make_unique<NodeFactory>();
}

auto ClientNode::CreateNodeFactoryParser()
    -> std::unique_ptr<json::INodeFactoryParser> {
  return std::make_unique<NodeFactoryParser>();
}
}  // namespace esc::impl