#pragma once

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

#include "application/ApplicationInfo.h"

template <typename T>
class MultiFrameObject
{
    public:
    template<typename... Args>
    MultiFrameObject(Args&&... args)
    {
        _internals.reserve(ApplicationInfo::Constant::MaxFrameInFlight);
        for(size_t i = 0; i < ApplicationInfo::Constant::MaxFrameInFlight; ++i)
        {
            _internals.emplace_back(std::forward<Args>(args)...);
        }
    }

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    MultiFrameObject(MultiFrameObject&&) noexcept = default;
    MultiFrameObject& operator=(MultiFrameObject&&) noexcept = default;

    MultiFrameObject(const MultiFrameObject&) = delete;
    MultiFrameObject& operator=(const MultiFrameObject&) = delete;

    inline T& Get(size_t frameIndex)
    {
        assert(frameIndex < _internals.size() && "MultiFrameObject: index out of range");
        return _internals[frameIndex];
    }
    inline const T& Get(size_t frameIndex) const
    {
        assert(frameIndex < _internals.size() && "MultiFrameObject: index out of range");
        return _internals[frameIndex];
    }

    inline T& operator[](size_t index) { return Get(index); }
    inline const T& operator[](size_t index) const { return Get(index); }

    inline T& Current() { return Get(ApplicationInfo::CurrentFrame()); }
    inline const T& Current() const { return Get(ApplicationInfo::CurrentFrame()); }

    // Unfortunally, c++ doesn't chain implicit conversion in this case, if we need a chain directly use the Current()
    inline operator T&() { return Current(); }
    inline operator const T&() const { return Current(); }

    inline size_t Size() const { return _internals.size(); }
    inline T* Data() { return _internals.data(); }
    inline const T* Data() const { return _internals.data(); }

    inline auto begin() { return _internals.begin(); }
    inline auto end() { return _internals.end(); }
    inline auto begin() const { return _internals.begin(); }
    inline auto end() const { return _internals.end(); }

    private:
    std::vector<T> _internals;
};
