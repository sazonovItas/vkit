#pragma once

#include <cstdint>

#include "vkit/material/material.hpp"

namespace vkit::workflow::node::mat {

struct MaterialLink {
  material::Type type{material::Type::kNone};
  std::uint32_t managerId{0};
};

};  // namespace vkit::workflow::node::mat
