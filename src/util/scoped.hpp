#pragma once

namespace vkit::util {
template <typename Type>
concept Scopeable = std::equality_comparable<Type>;

template <Scopeable Type, typename Deleter>
class Scoped {
 public:
  Scoped(Scoped const&) = delete;
  auto operator=(Scoped const&) = delete;

  Scoped() = default;

  constexpr Scoped(Scoped&& rhs) noexcept
      : m_t_(std::exchange(rhs.m_t_, Type{})) {}

  constexpr auto operator=(Scoped&& rhs) noexcept -> Scoped& {
    if (&rhs != this) {
      std::swap(m_t_, rhs.m_t_);
    }
    return *this;
  }

  explicit(false) constexpr Scoped(Type t) : m_t_(std::move(t)) {}

  constexpr ~Scoped() {
    if (m_t_ == Type{}) {
      return;
    }
    Deleter{}(m_t_);
  }

  [[nodiscard]] constexpr auto get() const -> Type const& { return m_t_; }
  [[nodiscard]] constexpr auto get() -> Type& { return m_t_; }

 private:
  Type m_t_{};
};
}  // namespace vkit::util
