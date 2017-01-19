#pragma once

#include <vector>
#include <cstdint>

// vectors have lexicographic ordering! Hooray
class ImageVersion : public std::vector<uint32_t> {
public:
    static bool Parse(const char *str, ImageVersion &ret);
};

bool ParseImgFileName(const CString& filename, CString &personality, CString &version, CString &date, bool &isInstallerImage);