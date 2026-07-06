#pragma once



#include "allocation/RessourceUsage.h"
#include <cstdint>
template<typename T>
class MultiFrameRessource
{
    public:
    inline RessourceUsage Usage() const { return _usage; }
    inline const T& operator[](uint32_t index) const { return _internals[index]; }
    inline const uint32_t CurrentIndex() const { return RessourceUsageToCurrentIndex(_usage); }
    inline const uint32_t PreviousIndex() const { return RessourceUsageToPreviousIndex(_usage); }

    protected:
    inline const T& Current() const { return _internals[RessourceUsageToCurrentIndex(_usage)]; }
    inline const T& Previous() const { return _internals[RessourceUsageToPreviousIndex(_usage)]; }
    RessourceUsage _usage;
    std::vector<T> _internals;
};
