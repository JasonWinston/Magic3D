/*==================================================================================================

                       Copyright (c) 2016 GeometryPlusPlus, ThreePark
                             Unpublished - All rights reserved

====================================================================================================*/
#pragma once
#include "Vector3.h"

namespace GPP
{
    extern Vector3 ColorCoding(Real scaleValue);

    extern std::string GetTimeString(void);

    class ThreadManager
    {
        static ThreadManager* mpThreadManager;
        explicit ThreadManager();
    public:
        static ThreadManager* Get();
        void SetThreadCount(Int count);
        Int GetThreadCount(void) const;
        ~ThreadManager();

    private:
        Int mThreadCount;
    };

    // Reset progress value before calling api
    extern GPP_EXPORT void ResetApiProgress(void);

    // Get progress value when running api
    // progress value range: [0, 1]
    extern GPP_EXPORT Real GetApiProgress(void);

    // Active library:
    extern GPP_EXPORT bool SetActivationKey(std::string key);

    // count >= 0: 0 - it will auto-allocate thread according CPU processor count.
    extern GPP_EXPORT void SetThreadCount(Int count);
}
