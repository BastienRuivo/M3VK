if(NOT DEFINED INPUT_FILE OR INPUT_FILE STREQUAL "")
    message(FATAL_ERROR "CompileShader.cmake error: INPUT_FILE variable is not defined.")
endif()

if(NOT EXISTS "${INPUT_FILE}")
    message(FATAL_ERROR "CompileShader.cmake error: Shader source file not found: ${INPUT_FILE}")
endif()

file(STRINGS "${INPUT_FILE}" SHADER_LINES)

set(GLSL_DEFINITIONS "")

# Iterate through each line of the shader file
foreach(LINE IN LISTS SHADER_LINES)


endforeach()
