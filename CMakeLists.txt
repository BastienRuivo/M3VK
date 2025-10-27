cmake_minimum_required(VERSION 3.10)
project(M3VK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Automatically collect all .cpp and .hpp/.h files
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/src/headers/*.h" "${CMAKE_SOURCE_DIR}/src/headers/*.hpp")

# Find dependencies
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Create executable
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/headers
    ${Vulkan_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw)
