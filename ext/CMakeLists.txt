project(learn-vk-ext)

set(CMAKE_CXX_COMPILER=clang++)

set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
message(STATUS "[glfw]")
add_subdirectory(glfw)
add_library(glfw::glfw ALIAS glfw)

set(GLM_ENABLE_CXX_20 ON)
message(STATUS "[glm]")
add_subdirectory(glm)
target_compile_definitions(glm PUBLIC
  GLM_FORCE_XYZW_ONLY
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
  GLM_ENABLE_EXPERIMENTAL
  GLM_EXT_INCLUDED
)

message(STATUS "[Vulkan-Headers]")
add_subdirectory(Vulkan-Headers)

message(STATUS "[VulkanMemoryAllocator-Hpp]")
add_subdirectory(VulkanMemoryAllocator-Hpp)

message(STATUS "[Dear ImGui]")
add_library(imgui)
add_library(imgui::imgui ALIAS imgui)
target_include_directories(imgui SYSTEM PUBLIC imgui)
target_link_libraries(imgui PUBLIC
  glfw::glfw
  Vulkan::Headers
)
target_compile_definitions(imgui PUBLIC
  VK_NO_PROTOTYPES
)
target_sources(imgui PRIVATE
  imgui/imconfig.h
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_internal.h
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui.cpp
  imgui/imgui.h

  imgui/backends/imgui_impl_glfw.cpp
  imgui/backends/imgui_impl_glfw.h
  imgui/backends/imgui_impl_vulkan.cpp
  imgui/backends/imgui_impl_vulkan.h
)

message(STATUS "[vma]")
add_library(vma)
add_library(vma::vma ALIAS vma)
target_link_libraries(vma PUBLIC
  Vulkan::Headers
  GPUOpen::VulkanMemoryAllocator
)
target_include_directories(vma SYSTEM PUBLIC
  VulkanMemoryAllocator-Hpp/include
  VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator/include
)
target_compile_definitions(vma PUBLIC
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1
)
target_sources(vma PRIVATE
  vk_mem_alloc.cpp
)
target_compile_options(vma PRIVATE -w)

message(STATUS "[fastgltf]")
add_subdirectory(fastgltf)

message(STATUS "[meshoptimizer]")
add_subdirectory(meshoptimizer)

message(STATUS "[image magick++]")
find_package(ImageMagick COMPONENTS Magick++)

add_library(${PROJECT_NAME} INTERFACE)
add_library(learn-vk::ext ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} INTERFACE
  glm::glm
  imgui::imgui
  vma::vma
  fastgltf::fastgltf
  meshoptimizer
  ${ImageMagick_LIBRARIES}
)
target_include_directories(${PROJECT_NAME} INTERFACE
  ${ImageMagick_INCLUDE_DIRS}
)
target_compile_definitions(${PROJECT_NAME} INTERFACE
  GLFW_INCLUDE_VULKAN
  MAGICKCORE_QUANTUM_DEPTH=32
  MAGICKCORE_HDRI_ENABLE=0
)

target_link_libraries(${PROJECT_NAME} INTERFACE dl)
