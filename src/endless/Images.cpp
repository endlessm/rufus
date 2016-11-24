#include "stdafx.h"
#include "Images.h"

#include <iostream>

bool ImageVersion::Parse(const char *str, ImageVersion &ret)
{
    char *s = (char *)str;
    char *end = NULL;

    ImageVersion version;
    while (true) {
        long int i = strtol(s, &end, 10);
        if (s == end) {
            return false;
        }

        if (i < 0 || i > UINT32_MAX) {
            return false;
        }

        version.push_back((uint32_t)i);
        if (*end == '\0') {
            break;
        } else if (*end == '.') {
            s = end + 1;
        } else {
            return false;
        }
    }

    ret = version;
    return true;
}
