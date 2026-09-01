#pragma once

// Precompiled header: heavy, widely-included third-party headers only.
// Parsed once per build config instead of once per translation unit -
// vulkan.hpp + vulkan_raii.hpp alone account for the bulk of build RAM
// since they're included (directly or transitively) from most headers.
//
// Do not add project headers here - it would force a full rebuild on every
// project header change instead of just the file that actually changed.

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>
