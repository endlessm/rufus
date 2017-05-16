#pragma once

#include <intrin.h>
#pragma intrinsic(_BitScanReverse)

#ifdef  _M_X64
#  pragma intrinsic(_BitScanReverse64)
#else
static __forceinline unsigned char _BitScanReverse64(
    unsigned long *Index,
    unsigned __int64 Mask)
{
    LARGE_INTEGER s;
    unsigned long i;

    s.QuadPart = Mask;

    if (_BitScanReverse(&i, s.HighPart)) {
        *Index = i + 32;
        return 1;
    }

    return _BitScanReverse(Index, s.LowPart);
}
#endif

// Returns log2(x), rounded towards 0, or 0 if x is 0.
static inline unsigned long log2ll(uint64_t x) {
    unsigned long i = 0;

    if (_BitScanReverse64(&i, x)) {
        return i;
    }

    return 0;
}
