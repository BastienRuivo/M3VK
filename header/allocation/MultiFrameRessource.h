#pragma once



#include "allocation/RessourceUsage.h"
template<typename T>
class MultiFrameRessource
{
    protected:
    inline const T& Current() const { return _internals[RessourceUsageToCurrentIndex(_usage)]; }
    inline const T& Previous() const { return _internals[RessourceUsageToPreviousIndex(_usage)]; }
    RessourceUsage _usage;
    std::vector<T> _internals;
};
