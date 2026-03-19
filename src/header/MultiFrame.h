#pragma once

#include <cstddef>
#include <utility>
#include <vector>
template <typename T>
class MultiFrameObject
{
    public:

    template<typename... Args>
    MultiFrameObject(size_t maxFrameInCount, Args&&... args)
    {
        _internals.reserve(maxFrameInCount);
        for(size_t i = 0; i < maxFrameInCount; ++i)
        {
            _internals.emplace_back(std::forward<Args>(args)...);
        }
    }

    MultiFrameObject(size_t maxFrameInCount)
    {
        _internals.reserve(maxFrameInCount);
    }

    template<typename... Args>
    inline void EmplaceBack(Args&&... args) {
        _internals.emplace_back(std::forward<Args>(args)...);
    }

    inline void Clear()
    {
        _internals.clear();
    }

    inline void Reserve(size_t size)
    {
        _internals.reserve(size);
    }

    // VkHandles can be destroyed by one of the two object when copying, so we enable moving and disable copying.
    // technically *for now* you can vk call directly to drestroy the VK thingy inside with the Get, honestly don't care for now, if the user want to explicitly do that it's his business, not mine.
    MultiFrameObject(MultiFrameObject&& other) noexcept:
    _internals(std::move(other._internals))
    {
    }
    MultiFrameObject& operator=(MultiFrameObject&& other) noexcept
    {
        if(this != &other)
        {
            _internals = std::move(other._internals);
        }
        return *this;
    }

    MultiFrameObject(const MultiFrameObject&) = delete;
    MultiFrameObject& operator=(const MultiFrameObject&) = delete;

    inline T& Get(size_t frameIndex)
    {
        return _internals[frameIndex];
    }

    inline size_t Size() const { return _internals.size(); }

    protected:
    std::vector<T> _internals;
};

template <typename T>
class MultiFrameHandler : public MultiFrameObject<T>
{
    public:
    using MultiFrameObject<T>::MultiFrameObject;

    MultiFrameHandler(MultiFrameHandler&& other) noexcept
        : MultiFrameObject<T>(std::move(other))
    {

    }

    MultiFrameHandler& operator=(MultiFrameHandler&& other) noexcept {
        if (this != &other) {
            MultiFrameObject<T>::operator=(std::move(other));
        }
        return *this;
    }

    MultiFrameHandler(const MultiFrameHandler&) = delete;
    MultiFrameHandler& operator=(const MultiFrameHandler&) = delete;

    inline auto GetInternal(size_t frameIndex)
    {
        return this->_internals[frameIndex].Get();
    }
};
