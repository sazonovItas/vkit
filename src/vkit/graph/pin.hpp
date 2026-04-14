#pragma once

namespace vkit::graph {

class Node;
class Pin;
class Link;

class Pin {
 private:
  friend class Node;
  Pin(Node* ownerNode, std::size_t slot, bool isSrc, std::size_t key,
      std::string_view name);

 public:
  virtual ~Pin() noexcept;

  [[nodiscard]] auto isSrc() const -> bool;
  [[nodiscard]] auto isSink() const -> bool;
  [[nodiscard]] auto getId() const -> int;
  [[nodiscard]] auto getKey() const -> std::size_t;
  [[nodiscard]] auto getName() const -> std::string_view;

  void addLink(Link* link);
  void removeLink(Link* link);

  [[nodiscard]] auto getLinks() const -> const std::vector<Link*>&;
  [[nodiscard]] auto getLinks() -> std::vector<Link*>&;
  [[nodiscard]] auto getOwnerNode() const -> Node*;
  [[nodiscard]] auto getSlot() const -> std::size_t;

 protected:
  int id_;
  std::size_t key_;
  Node* ownerNode_{nullptr};
  std::size_t slot_;
  bool isSrc_{true};
  std::string name_;
  std::vector<Link*> links_;
};

};  // namespace vkit::graph
