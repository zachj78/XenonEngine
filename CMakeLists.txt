cmake_minimum_required(VERSION 3.10)
project(MyVulkanEngine)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect source and header files recursively
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "include/*.h" "include/*.hpp")

# Create executable (include headers for IDE visibility)
add_executable(MyVulkanEngine ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(MyVulkanEngine PRIVATE src include)

target_compile_options(MyVulkanEngine PRIVATE /FS)

if(WIN32)
    # TODO FIX STATIC LINKING, SHITS STUPID
    #GLM
    target_include_directories(MyVulkanEngine PRIVATE "C:/c++_libs/glm-master")

    # Vulkan
    find_package(Vulkan REQUIRED)
    target_link_libraries(MyVulkanEngine PRIVATE Vulkan::Vulkan)

    # GLFW
    set(glfw3_DIR "C:/c++_libs/glfw-3.4/glfw-install/lib/cmake/glfw3")
    find_package(glfw3 REQUIRED)
    target_link_libraries(MyVulkanEngine PRIVATE glfw)
elseif (UNIX)
    # Find the GLFW package
    find_package(GLFW3 REQUIRED)

    # Find Vulkan (assuming Vulkan SDK is installed and set up)
    find_package(Vulkan REQUIRED)

    # Include GLM headers (no need to link)
    include_directories(/usr/include)  # Adjust path if necessary

    # Link GLFW and Vulkan to your project
    target_link_libraries(MyVulkanEngine PRIVATE glfw Vulkan::Vulkan)
endif()

# IDE folder structure: Group sources/headers by subdirectory
foreach(source_file ${SOURCES} ${HEADERS})
    get_filename_component(source_path "${source_file}" PATH)
    file(RELATIVE_PATH group_path "${CMAKE_CURRENT_SOURCE_DIR}" "${source_path}")
    string(REPLACE "/" "\\" group_path "${group_path}")
    source_group("${group_path}" FILES "${source_file}")
endforeach()
