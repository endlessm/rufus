#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

namespace WMI {
    BOOL IsBitlockedDrive(const CString &drive);
    BOOL GetMachineInfo(CString &manufacturer, CString &model);
};
