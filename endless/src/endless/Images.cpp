#include "stdafx.h"
#include "Images.h"

#include <iostream>

#include "GeneralCode.h"

#define EOS_PRODUCT_TEXT            "eos"
#define EOS_INSTALLER_PRODUCT_TEXT  "eosinstaller"
#define EOS_NONFREE_PRODUCT_TEXT    "eosnonfree"
#define EOS_OEM_PRODUCT_TEXT        "eosoem"
#define EOS_RETAIL_PRODUCT_TEXT     "eosretail"
#define EOS_DVD_PRODUCT_TEXT        "eosdvd"

bool ParseImageVersion(const char *str, ImageVersion &ret)
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

bool ParseImgFileName(const CString& filename, CString &personality, CString &version, CString &date, bool &isInstallerImage)
{
    FUNCTION_ENTER_FMT("%ls", filename);

    // parse filename to get personality and version
    CString lastPart;
    PCTSTR t1 = _T("-"), t2 = _T(".");
    int pos = 0;

    // RADU: Add some more validation here for the filename
    CString resToken = filename.Tokenize(t1, pos);
    CString product;
    int elemIndex = 0;
    while (!resToken.IsEmpty()) {
        switch (elemIndex) {
        case 0: product = resToken; break;
        case 1: version = resToken; break;
        case 3: date = resToken; break;
        case 4: lastPart = resToken; break;
        }
        resToken = filename.Tokenize(t1, pos);
        elemIndex++;
    };

    version.Replace(_T(EOS_PRODUCT_TEXT), _T(""));
    IFFALSE_GOTOERROR(!version.IsEmpty() && !lastPart.IsEmpty(), "");
    isInstallerImage = product == _T(EOS_INSTALLER_PRODUCT_TEXT);
    if (product != _T(EOS_PRODUCT_TEXT) &&
        product != _T(EOS_NONFREE_PRODUCT_TEXT) &&
        product != _T(EOS_OEM_PRODUCT_TEXT) &&
        product != _T(EOS_RETAIL_PRODUCT_TEXT) &&
        product != _T(EOS_DVD_PRODUCT_TEXT) &&
        !isInstallerImage) {
        uprintf("Unrecognised product name '%ls'; assuming it's some new product", product);
    }

    date = date.Right(date.GetLength() - date.Find('.') - 1);

    pos = 0;
    resToken = lastPart.Tokenize(t2, pos);
    elemIndex = 0;
    while (!resToken.IsEmpty()) {
        switch (elemIndex) {
        case 1: personality = resToken; break;
        case 4: goto error; break; // we also have diskX
        }
        resToken = lastPart.Tokenize(t2, pos);
        elemIndex++;
    };
    IFFALSE_GOTOERROR(!personality.IsEmpty(), "empty personality");

    return true;
error:
    return false;
}