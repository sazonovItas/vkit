#pragma once

#include <any>
#include <string>
#include <string_view>
#include <vector>

namespace vkit::graph {

class Node;
class Link;

enum class PinKind { kInput, kOutput };

class Pin {
 private:
  friend class Node;
  explicit Pin(Node* ownerNode, std::size_t slot, bool isSrc, std::size_t key,
               std::string_view name);

 public:
  virtual ~Pin() noexcept;

  [[nodiscard]] auto isSrc() const -> bool;
  [[nodiscard]] auto isSink() const -> bool;

  [[nodiscard]] auto getKind() const -> PinKind;

  [[nodiscard]] auto getId() const -> int;
  [[nodiscard]] auto getKey() const -> std::size_t;
  [[nodiscard]] auto getName() const -> std::string_view;

  void addLink(Link* link);
  void removeLink(Link* link);

  [[nodiscard]] auto getLinks() const -> const std::vector<Link*>&;
  [[nodiscard]] auto getLinks() -> std::vector<Link*>&;
  [[nodiscard]] auto getOwnerNode() const -> Node*;
  [[nodiscard]] auto getSlot() const -> std::size_t;

  void setOwnerNode(Node* node);

  template <typename T>
  void setData(T value) {
    payload_ = std::make_any<T>(std::move(value));
  }

  template <typename T>
  [[nodiscard]] auto getData() const -> const T* {
    return std::any_cast<T>(&payload_);
  }

  void clearData() { payload_.reset(); }

  [[nodiscard]] auto hasData() const -> bool { return payload_.has_value(); }

 protected:
  int id_;
  std::size_t key_;
  Node* ownerNode_{nullptr};
  std::size_t slot_;

  bool isSrc_{true};

  std::string name_;
  std::vector<Link*> links_;

  std::any payload_;
};

};  // namespace vkit::graph
