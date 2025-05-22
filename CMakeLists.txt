cmake_minimum_required(VERSION 3.10)
project(MyVulkanEngine)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect source and header files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "include/*.h" "include/*.hpp")

# Add include directories
include_directories(src include)

# Create executable
add_executable(MyVulkanEngine ${SOURCES} ${HEADERS})

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(MyVulkanEngine PRIVATE Vulkan::Vulkan)

# GLFW
set(glfw3_DIR "C:/c++_libs/glfw-3.4/glfw-install/lib/cmake/glfw3")
find_package(glfw3 REQUIRED)
target_link_libraries(MyVulkanEngine PRIVATE glfw)