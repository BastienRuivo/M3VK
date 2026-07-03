#pragma once

#include "application/ApplicationInfo.h"
#include <cstdint>
enum RessourceUsage
{
    Static,
    PerFrame,
    Transient
};

inline uint32_t RessourceUsageToCurrentIndex(RessourceUsage usage)
{
    return usage == RessourceUsage::PerFrame ? ApplicationInfo::CurrentFrame() : 0;
}

inline uint32_t RessourceUsageToPreviousIndex(RessourceUsage usage)
{
    return usage == RessourceUsage::PerFrame ? ApplicationInfo::PreviousFrame() : 0;
}

inline uint32_t RessourceUsageCount(RessourceUsage usage)
{
    return usage == RessourceUsage::PerFrame ? ApplicationInfo::Constant::MaxFrameInFlight : 1;
}
