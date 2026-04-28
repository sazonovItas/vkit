#pragma once

#include <atomic>

namespace vkit {

template <class T>
class UniqueId {
 public:
  using IdType = std::size_t;

  static constexpr IdType kNullId{0};

  constexpr UniqueId() : id_{allocateId()} {}

  UniqueId(const UniqueId&) = delete;

  UniqueId(UniqueId&& other) noexcept : id_{other.getId()} { other.reset(); }

  auto operator=(UniqueId&& other) noexcept -> UniqueId& {
    id_ = other.getId();
    other.reset();
    return *this;
  }

  ~UniqueId() noexcept = default;

  void reset() { id_ = kNullId; }

  [[nodiscard]] auto getId() const -> IdType { return id_; }

 private:
  [[nodiscard]] static auto allocateId() -> IdType {
    static std::atomic<IdType> counter{1};

    return counter++;
  }

  IdType id_{kNullId};
};

template <class T>
class Item {
 public:
  explicit Item(std::string_view name) : name_{name} {}
  virtual ~Item() = default;

  [[nodiscard]] auto getId() const -> std::size_t { return id_.getId(); };
  [[nodiscard]] auto getName() const -> std::string_view { return name_; };

  void setName(std::string_view name) { name_ = name; };

 protected:
  UniqueId<T> id_;
  std::string name_;
};

};  // namespace vkit
