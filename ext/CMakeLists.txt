cmake_minimum_required(VERSION 3.24)

project(ext)

set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)

message(STATUS "[glfw]")
add_subdirectory(glfw)
add_library(glfw::glfw ALIAS glfw)

set(GLM_ENABLE_CXX_20 ON)

message(STATUS "[glm]")
add_subdirectory(glm)

target_compile_definitions(glm PUBLIC
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_SILENT_WARNINGS
    GLM_ENABLE_EXPERIMENTAL
)

message(STATUS "[Vulkan-Headers]")
add_subdirectory(Vulkan-Headers)

message(STATUS "[VulkanMemoryAllocator-Hpp]")
add_subdirectory(VulkanMemoryAllocator-Hpp)

message(STATUS "[Dear ImGui]")

add_library(imgui)
add_library(imgui::imgui ALIAS imgui)

target_compile_definitions(imgui PUBLIC
  IMGUI_DEFINE_MATH_OPERATORS
)

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
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_internal.h
    imgui/imgui.h

    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_glfw.h
    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_vulkan.h
)

message(STATUS "[ImGuizmo ImGui]")

add_library(imguizmo)
add_library(imguizmo::imguizmo ALIAS imguizmo)

target_include_directories(imguizmo SYSTEM PUBLIC ImGuizmo)

target_precompile_headers(imguizmo PRIVATE <imgui.h> <imgui_internal.h>)

target_link_libraries(imguizmo PUBLIC imgui)

target_sources(imguizmo PRIVATE
    ImGuizmo/GraphEditor.cpp
    ImGuizmo/GraphEditor.h
    ImGuizmo/ImCurveEdit.cpp
    ImGuizmo/ImCurveEdit.h
    ImGuizmo/ImGradient.cpp
    ImGuizmo/ImGradient.h
    ImGuizmo/ImGuizmo.cpp
    ImGuizmo/ImGuizmo.h
    ImGuizmo/ImLightRig.h
    ImGuizmo/ImSequencer.cpp
    ImGuizmo/ImSequencer.h
    ImGuizmo/ImZoomSlider.h
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

message(STATUS "[stb]")

add_library(stb)
add_library(stb::stb ALIAS stb)

target_include_directories(stb SYSTEM PUBLIC stb)

target_sources(stb PRIVATE
    stb_image.cpp
)


message(STATUS "[ext]")
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} INTERFACE
    glm::glm
    imgui::imgui
    imguizmo::imguizmo
    vma::vma
    fastgltf::fastgltf
    meshoptimizer
    stb::stb
    dl
)

target_compile_definitions(${PROJECT_NAME} INTERFACE
    GLFW_INCLUDE_VULKAN
)
