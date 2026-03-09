cmake_minimum_required(VERSION 3.14)
project(M3VK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8 # Or a specific version/commit
)

FetchContent_MakeAvailable(glfw)

# Automatically collect all .cpp and .hpp/.h files
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/src/headers/*.h" "${CMAKE_SOURCE_DIR}/src/headers/*.hpp")

# Find dependencies
find_package(Vulkan REQUIRED)
#find_package(glfw3 REQUIRED)

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

# -------------------------------------------------------------------
# 🔹 Shader Compilation (outputs to ${CMAKE_BINARY_DIR}/shaders)
# -------------------------------------------------------------------

# All GLSL shader files under src/shaders
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/src/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/src/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/src/shaders/*.comp"
    "${CMAKE_SOURCE_DIR}/src/shaders/*.geom"
    "${CMAKE_SOURCE_DIR}/src/shaders/*.tesc"
    "${CMAKE_SOURCE_DIR}/src/shaders/*.tese"
)

# Create a shaders output directory in the build folder
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(SPIRV_BINARY_FILES "")
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
        COMMENT "🌀 Compiling shader: ${FILE_NAME}"
        VERBATIM
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

# Build all shaders before the main binary
add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(${PROJECT_NAME} Shaders)
