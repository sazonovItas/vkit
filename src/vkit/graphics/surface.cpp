#include "vkit/graphics/surface.hpp"

namespace vkit::graphics {

Surface::Surface(const Window& window, const Instance& instance)
    : surface_{window.createSurface(instance.get())} {}

};  // namespace vkit::graphics
