
// EndlessUsbTool.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "EndlessUsbTool.h"
#include "EndlessUsbToolDlg.h"
#include "Analytics.h"

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


// CEndlessUsbToolApp construction

CEndlessUsbToolApp::CEndlessUsbToolApp()
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

    // check command line parameters;
    CAppCmdLineInfo commandLineInfo;
    ParseCommandLine(commandLineInfo);

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    UINT wndMsg = RegisterWindowMessage(_T("EndlessUSBToolShow"));
    // check if we are the first instance
    HANDLE singleInstanceMutex = CreateMutex(NULL, TRUE, _T("Global\\EndlessUsbTool"));

	Analytics::instance()->sessionControl(true);

    if (singleInstanceMutex != NULL && GetLastError() != ERROR_ALREADY_EXISTS) {
        //CEndlessUsbToolDlg dlg(wndMsg, commandLineInfo.logDebugInfo);
		CEndlessUsbToolDlg dlg(wndMsg, true);
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

	Analytics::instance()->sessionControl(false);

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

    ReleaseMutex(singleInstanceMutex);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

