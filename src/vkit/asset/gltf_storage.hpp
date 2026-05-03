#pragma once

#include "vkit/asset/asset.hpp"
#include "vkit/item/storage.hpp"

namespace vkit::asset {

class GltfStorage : public Storage<Asset> {
 public:
  GltfStorage() = default;
  ~GltfStorage() override = default;
};

};  // namespace vkit::asset
