#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

namespace WMI {
    BOOL IsBitlockedDrive(const CString &drive);
    BOOL GetMachineInfo(CString &manufacturer, CString &model);
    BOOL IsHyperVEnabled(void);

    // Adds an entry for the given bootloader to the system boot menu.
    // Supported on BIOS systems running Windows Vista or newer.
    // name (in): human-readable label for boot menu
    // mbrPath (in): full path, including drive letter, to a small (<= 8KiB) bootloader file
    // guid (in): GUID for the boot menu entry
    // Returns: TRUE if the boot menu entry was successfully added
    BOOL AddBcdEntry(const CString &name, const CString &mbrPath, const CString &guid);

    // Removes an entry from the system boot menu.
    // guid (in): GUID for an existing boot menu entry
    BOOL RemoveBcdEntry(const CString &guid, bool &foundBootEntry);
};
