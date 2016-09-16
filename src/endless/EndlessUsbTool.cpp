
// EndlessUsbTool.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "EndlessUsbTool.h"
#include "EndlessUsbToolDlg.h"
#include "Analytics.h"

#include "StringHelperMethods.h"
#include "version.h"

extern "C" {
	#include "rufus.h"
	extern HANDLE GlobalLoggingMutex;

	char app_dir[MAX_PATH];
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAppCmdLineInfo : public CCommandLineInfo
{
public:
    CAppCmdLineInfo(void) :
        logDebugInfo(false) {}

    virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
    {
        if (0 == _tcscmp(_T("debug"), pszParam)) {
            logDebugInfo = true;
        } else {
            CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
        }
    }

    bool logDebugInfo;
};


// CEndlessUsbToolApp

BEGIN_MESSAGE_MAP(CEndlessUsbToolApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CString CEndlessUsbToolApp::m_appDir = L"";
bool CEndlessUsbToolApp::m_enableLogDebugging = false;
CFile CEndlessUsbToolApp::m_logFile;

// CEndlessUsbToolApp construction

CEndlessUsbToolApp::CEndlessUsbToolApp() : CWinApp(ENDLESS_INSTALLER_TEXT)
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CEndlessUsbToolApp object

CEndlessUsbToolApp theApp;


// CEndlessUsbToolApp initialization

BOOL CEndlessUsbToolApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	CWinApp::SetRegistryKey(_T("Endless"));

	AfxEnableControlContainer();

	GlobalLoggingMutex = CreateMutex(NULL, FALSE, NULL);

	// initialize application dir
	TCHAR *path = m_appDir.GetBufferSetLength(MAX_PATH + 1);
	// Retrieve the current application directory
	if (GetModuleFileName(NULL, path, MAX_PATH) == 0) {
		uprintf("Could not get current directory: gle=%d", GetLastError());
		app_dir[0] = 0;
		m_appDir = _T("");
	}
	else {
		m_appDir = CSTRING_GET_PATH(m_appDir, _T('\\'));
		strcpy_s(app_dir, sizeof(app_dir) - 1, ConvertUnicodeToUTF8(m_appDir));
		app_dir[sizeof(app_dir) - 1] = 0;
	}
	m_appDir.ReleaseBuffer();

	// Set the Windows version
	GetWindowsVersion();

    // check command line parameters;
    CAppCmdLineInfo commandLineInfo;
    ParseCommandLine(commandLineInfo);

	//m_enableLogDebugging = commandLineInfo.logDebugInfo;
	m_enableLogDebugging = true;
	InitLogging();


	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    UINT wndMsg = RegisterWindowMessage(_T("EndlessUSBToolShow"));

	Analytics::instance()->sessionControl(true, false);

    // check if we are the first instance
    HANDLE singleInstanceMutex = CreateMutex(NULL, TRUE, _T("Global\\EndlessUsbTool"));

    if (singleInstanceMutex != NULL && GetLastError() != ERROR_ALREADY_EXISTS) {
        //CEndlessUsbToolDlg dlg(wndMsg, commandLineInfo.logDebugInfo);
		CEndlessUsbToolDlg dlg(wndMsg);
        m_pMainWnd = &dlg;
        INT_PTR nResponse = dlg.DoModal();
        if (nResponse == IDOK)
        {
            // TODO: Place code here to handle when the dialog is
            //  dismissed with OK
        }
        else if (nResponse == IDCANCEL)
        {
            // TODO: Place code here to handle when the dialog is
            //  dismissed with Cancel
        }
        else if (nResponse == -1)
        {
            TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
            TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
        }
    } else {
        ::SendMessage(HWND_BROADCAST, wndMsg, 0, 0);
    }

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

    ReleaseMutex(singleInstanceMutex);

	Analytics::instance()->sessionControl(false, false);

	UninitLogging();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

void CEndlessUsbToolApp::InitLogging()
{
	if (!m_enableLogDebugging) {
		uprintf("Logging not enabled.");
		return;
	}

	// Create file name
	CTime time = CTime::GetCurrentTime();
	CString s = time.Format(_T("%Y%m%d_%H_%M_%S"));
	CString fileName(ENDLESS_INSTALLER_TEXT);
	fileName.Replace(L" ", L"");
	fileName += s;
	fileName += L".log";
	fileName = GET_LOCAL_PATH(fileName);

	try {
		BOOL result = m_logFile.Open(fileName, CFile::modeWrite | CFile::typeUnicode | CFile::shareDenyWrite | CFile::modeCreate | CFile::osWriteThrough);
		if (!result) {
			m_enableLogDebugging = false;
		}

		//uprintf("Log original date time %ls\n", s);
		//uprintf("Application version: %s\n", RELEASE_VER_STR);
		//uprintf("Windows version: %s\n", WindowsVersionStr);
		//uprintf("Windows version number: 0x%X\n", nWindowsVersion);
		//uprintf("-----------------------------------\n", s);
	}
	catch (CFileException *ex) {
		m_enableLogDebugging = false;
		uprintf("CFileException on file [%ls] with cause [%d] and OS error [%d]", fileName, ex->m_cause, ex->m_lOsError);
		ex->Delete();
	}
}

void CEndlessUsbToolApp::UninitLogging()
{
	if (m_enableLogDebugging) {
		m_enableLogDebugging = false;
		m_logFile.Close();
	}

	if (GlobalLoggingMutex != NULL) CloseHandle(GlobalLoggingMutex);
}

// Do not use any method that calls _uprintf as it will generate an infinite recursive loop
// Use OutputDebugString instead.
void CEndlessUsbToolApp::Log(const char *logMessage)
{
	static CStringA strMessage;
	static bool firstMessage = true;

	if (firstMessage) {
		// calling uprintf here works because it will only be executed once
		firstMessage = false;
		CStringA debugMessage;
		debugMessage.Format("Log original file name %ls\n", m_logFile.GetFileName()); Log(debugMessage);
		debugMessage.Format("Application version: %s\n", RELEASE_VER_STR); Log(debugMessage);
		debugMessage.Format("Windows version: %s\n", WindowsVersionStr); Log(debugMessage);
		debugMessage.Format("Windows version number: 0x%X\n", nWindowsVersion); Log(debugMessage);
		debugMessage.Format("-----------------------------------\n"); Log(debugMessage);
	}

	if (CEndlessUsbToolApp::m_enableLogDebugging) {
		CStringA time(CTime::GetCurrentTime().Format(_T("%H:%M:%S - ")));
		strMessage = time + CStringA(logMessage);
		m_logFile.Write(strMessage, strMessage.GetLength());
	}
}
