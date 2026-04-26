#include "vkit/primitive/storage.hpp"

namespace vkit::primitive {

auto Storage::add(const std::shared_ptr<Primitive>& prim) -> std::uint32_t {
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

void Storage::remove(const std::uint32_t index) {
  if (index < primitives_.size() && primitives_[index]) {
    primitives_[index]->setId(std::nullopt);
    primitives_[index].reset();

    primitiveData_[index] = Primitive::Data{};

    freeList_.push_back(index);
  }
}

void Storage::update() {
  for (std::size_t i = 0; i < primitives_.size(); ++i) {
    if (primitives_[i]) {
      primitiveData_[i] = primitives_[i]->getData();
    }
  }
}

auto Storage::getPrimitives() const
    -> const std::vector<std::shared_ptr<Primitive>>& {
  return primitives_;
}

auto Storage::getData() const -> std::span<const std::byte> {
  return std::as_bytes(std::span{primitiveData_});
}

};  // namespace vkit::primitive
