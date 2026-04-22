#include "vkit/scene/item.hpp"

namespace vkit::scene {

Item::Item(const std::string_view name) : name_{name} {}

auto Item::getId() const -> std::size_t { return id_.getId(); }

auto Item::getName() const -> std::string_view { return name_; }

void Item::setName(const std::string_view name) { this->name_ = name; }

};  // namespace vkit::scene
