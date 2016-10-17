
// EndlessUsbTool.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define ENDLESS_INSTALLER_TEXT 		L"Endless Installer"
#define PATH_ENDLESS_SUBDIRECTORY	L"endless\\"

// CEndlessUsbToolApp:
// See EndlessUsbTool.cpp for the implementation of this class
//

class CEndlessUsbToolApp : public CWinApp
{
public:
	CEndlessUsbToolApp();

// Overrides
public:
	virtual BOOL InitInstance();

	static CString m_appDir;
	static CString m_imageDir;
	static bool m_enableLogDebugging;
	static bool m_enableOverwriteMbr;
	static bool m_enableIgnoreEFIError;

	static void Log(const char *logMessage);
	static CString TempFilePath(CString filename);

// Implementation

	DECLARE_MESSAGE_MAP()

private:
	static void InitLogging();
	static void UninitLogging();
	static CFile m_logFile;
};

extern CEndlessUsbToolApp theApp;
