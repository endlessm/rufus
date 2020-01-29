#pragma once

#include <vector>
#include <cstdint>

/* Well-known file names */

// Combined USBs and dual-boot systems use an uncompressed OS image with this name
#define ENDLESS_IMG_FILE_NAME           L"endless.img"
// ISOs use a squashfs image containing one file named ENDLESS_IMG_FILE_NAME
#define ENDLESS_SQUASH_FILE_NAME        L"endless.squash"
// Combined USBs and ISOs store the "true" name of the image in a text file
#define EXFAT_ENDLESS_LIVE_FILE_NAME    L"live"

// vectors have lexicographic ordering! Hooray
typedef std::vector<uint32_t> ImageVersion;
bool ParseImageVersion(const char *str, ImageVersion &ret);

bool ParseImgFileName(const CString& filename, CString &personality, CString &version, CString &date, bool &isInstallerImage);