cmake_minimum_required(VERSION 3.14)
project(M3VK)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# --- Dependencies ---

# GLFW
fetchcontent_declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)

# STB
fetchcontent_declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)

# GLM
fetchcontent_declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
)

fetchcontent_declare(
    spirv_reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG        main
)

fetchcontent_makeavailable(glfw stb glm spirv_reflect)

# --- Interface Library Setup ---

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# SPIRV-Reflect compilation target
add_library(spirv_reflect_lib STATIC
    "${spirv_reflect_SOURCE_DIR}/spirv_reflect.c"
    "${spirv_reflect_SOURCE_DIR}/spirv_reflect.h"
)
target_include_directories(spirv_reflect_lib PUBLIC ${spirv_reflect_SOURCE_DIR})

# --- Vulkan and Shaders ---

find_package(Vulkan REQUIRED)
target_link_libraries(spirv_reflect_lib PRIVATE Vulkan::Vulkan)

find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please ensure the Vulkan SDK is installed.")
endif()

set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/shaders")

if(NOT EXISTS ${SHADER_BINARY_DIR})
    file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})
endif()

file(GLOB_RECURSE SHADER_SOURCES
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.comp"
)

set(SPIRV_BINARY_FILES "")

foreach(SHADER_SOURCE ${SHADER_SOURCES})
    get_filename_component(FILE_NAME ${SHADER_SOURCE} NAME)
    set(SPIRV_OUTPUT "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SOURCE} -o ${SPIRV_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader: ${FILE_NAME}"
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUTPUT})
endforeach()

add_custom_target(CompileShaders DEPENDS ${SPIRV_BINARY_FILES})

# --- Executable ---

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/header/*.h" "${CMAKE_SOURCE_DIR}/header/*.hpp" "${CMAKE_SOURCE_DIR}/common/*.h" "${CMAKE_SOURCE_DIR}/common/*.hpp")

# define data source directory
add_definitions(-DDATA_SOURCE_DIR="data/")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SHADER_DIRECTORY="${SHADER_BINARY_DIR}/"
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    GLFW_INCLUDE_VULKAN
)

add_dependencies(${PROJECT_NAME} CompileShaders)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/header
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
)

# Link the new libraries here
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glfw
    stb
    glm::glm
    spirv_reflect_lib
)

# --- Asset Management ---

set(DATA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/data")
set(DATA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/data")

file(MAKE_DIRECTORY ${DATA_OUTPUT_DIR})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${DATA_SOURCE_DIR}
            ${DATA_OUTPUT_DIR}
    COMMENT "Syncing data assets to build directory..."
)
