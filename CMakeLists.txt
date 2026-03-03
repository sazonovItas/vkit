cmake_minimum_required(VERSION 3.24)

project(vkit)

set(CMAKE_CXX_COMPILER=clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(ext)

add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE learn-vk::ext)
target_precompile_headers(${PROJECT_NAME} PRIVATE <glm/glm.hpp>
                          <vulkan/vulkan.hpp>)
target_include_directories(${PROJECT_NAME} PRIVATE src)

file(
  GLOB_RECURSE sources
  LIST_DIRECTORIES false
  "src/*.[hc]pp")
target_sources(${PROJECT_NAME} PRIVATE ${sources})
