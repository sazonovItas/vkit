#include "vkit/primitive/storage.hpp"

#include <stdexcept>

namespace vkit::primitive {

auto Storage::add(const std::shared_ptr<Primitive>& prim) -> std::uint32_t {
  if (!prim) {
    throw std::invalid_argument{"Cannot add a null primitive"};
  }

  const auto lock = std::scoped_lock{mutex_};
  std::uint32_t index = 0;

  if (!freeList_.empty()) {
    index = freeList_.back();
    freeList_.pop_back();

    primitives_[index] = prim;
    primitiveData_[index] = prim->getData();
  } else {
    index = static_cast<std::uint32_t>(primitives_.size());

    primitives_.push_back(prim);
    primitiveData_.push_back(prim->getData());
  }

  prim->setId(index);
  return index;
}

void Storage::remove(std::uint32_t index) {
  const auto lock = std::scoped_lock{mutex_};

  if (index < primitives_.size() && primitives_[index]) {
    primitives_[index]->setId(std::nullopt);
    primitives_[index] = nullptr;

    primitiveData_[index] = Primitive::Data{};

    freeList_.push_back(index);
  }
}

void Storage::update() {
  const auto lock = std::scoped_lock{mutex_};

  for (std::size_t i = 0; i < primitives_.size(); ++i) {
    if (primitives_[i]) {
      primitiveData_[i] = primitives_[i]->getData();
    }
  }
}

auto Storage::getPrimitives() const -> std::vector<std::shared_ptr<Primitive>> {
  const auto lock = std::scoped_lock{mutex_};

  std::vector<std::shared_ptr<Primitive>> result;
  result.reserve(primitives_.size() - freeList_.size());

  for (const auto& prim : primitives_) {
    if (prim) {
      result.push_back(prim);
    }
  }

  return result;
}

auto Storage::getPrimitive(std::uint32_t index) const
    -> std::shared_ptr<Primitive> {
  const auto lock = std::scoped_lock{mutex_};

  if (index < primitives_.size()) {
    return primitives_[index];
  }

  return nullptr;
}

auto Storage::getData() const -> std::span<const std::byte> {
  const auto lock = std::scoped_lock{mutex_};
  return std::as_bytes(std::span{primitiveData_});
}

};  // namespace vkit::primitive
