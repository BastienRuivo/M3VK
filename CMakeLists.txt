cmake_minimum_required(VERSION 3.14)
project(M3VK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# --- Dependencies ---

fetchcontent_declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)

fetchcontent_declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)

fetchcontent_makeavailable(glfw stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# -------------------------------------------------------------------

set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
add_definitions(-DSHADER_DIRECTORY="${SHADER_OUTPUT_DIR}/")

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/src/headers/*.h" "${CMAKE_SOURCE_DIR}/src/headers/*.hpp")

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/headers
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw stb)

# -------------------------------------------------------------------
#  Data Assets Sync (outputs to ${CMAKE_BINARY_DIR}/data)
# -------------------------------------------------------------------

set(DATA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/data")
set(DATA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/data")

# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${DATA_OUTPUT_DIR})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${DATA_SOURCE_DIR}
            ${DATA_OUTPUT_DIR}
    COMMENT "Syncing data assets to build directory..."
)
