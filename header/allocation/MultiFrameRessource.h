#pragma once



#include "allocation/RessourceUsage.h"
#include <cassert>
#include <cstdint>
#include <vector>
template<typename T>
class MultiFrameRessource
{
    public:
    inline RessourceUsage Usage() const { return _usage; }

    inline const T& operator[](uint32_t index) const
    {
        assert(index < _internals.size() && "MultiFrameRessource: index out of range");
        return _internals[index];
    }

    inline const uint32_t CurrentIndex() const { return RessourceUsageToCurrentIndex(_usage); }
    inline const uint32_t PreviousIndex() const { return RessourceUsageToPreviousIndex(_usage); }

    inline auto begin() const { return _internals.begin(); }
    inline auto end() const { return _internals.end(); }

    protected:
    inline T& operator[](uint32_t index)
    {
        assert(index < _internals.size() && "MultiFrameRessource: index out of range");
        return _internals[index];
    }

    inline T& Get(uint32_t index) { return (*this)[index]; }
    inline const T& Get(uint32_t index) const { return (*this)[index]; }

    inline T& Current() { return Get(RessourceUsageToCurrentIndex(_usage)); }
    inline const T& Current() const { return Get(RessourceUsageToCurrentIndex(_usage)); }
    inline T& Previous() { return Get(RessourceUsageToPreviousIndex(_usage)); }
    inline const T& Previous() const { return Get(RessourceUsageToPreviousIndex(_usage)); }

    inline auto begin() { return _internals.begin(); }
    inline auto end() { return _internals.end(); }

    RessourceUsage _usage;
    std::vector<T> _internals;
};
