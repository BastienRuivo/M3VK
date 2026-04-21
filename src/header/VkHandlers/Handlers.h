#pragma once

#include <utility>
#include <vulkan/vulkan_core.h>
#include <concepts>

template<typename T>
    requires std::same_as<T, VkCommandPool>
class Handler
{
public:
    Handler() {}
    virtual ~Handler() {};
    Handler(Handler&& other) noexcept
    {
        _internal = std::exchange(other._internal, VK_NULL_HANDLE);
    }
    Handler& operator=(Handler&& other) noexcept
    {
        if(this != &other)
        {
            _internal = std::exchange(other._internal, VK_NULL_HANDLE);
        }
        return *this;
    }

    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;

    inline T& Internal() { return _internal; }

protected:
    T _internal = VK_NULL_HANDLE;
};

// -- HANDLER DEFINITION

class VkCommandPoolHandler : public Handler<VkCommandPool>
{
public:
    VkCommandPoolHandler();
    ~VkCommandPoolHandler();
};
