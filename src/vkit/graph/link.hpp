#pragma once

namespace vkit::graph {

class Pin;
class Link;

class Link {
 public:
  Link();
  Link(Link&& old) noexcept;
  Link(const Link& other) = delete;
  Link& operator=(Link&& old) noexcept;
  Link& operator=(const Link& other) = delete;
  Link(Pin* src, Pin* sink);
  virtual ~Link() noexcept;

  [[nodiscard]] auto getId() const -> int;
  [[nodiscard]] auto getSrc() const -> Pin*;
  [[nodiscard]] auto getSink() const -> Pin*;
  [[nodiscard]] auto isConnected() const -> bool;
  void disconnect();

 private:
  int id_;
  Pin* src_{nullptr};
  Pin* sink_{nullptr};
};

};  // namespace vkit::graph
