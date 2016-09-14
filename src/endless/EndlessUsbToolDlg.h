
// EndlessUsbToolDlg.h : header file
//

#pragma once

#include "localization.h"
#include "DownloadManager.h"

// Remove this define to disable hardcoded stuff added for internal release on August 31st
#define TEST_RELEASE_HARDCODED_STUFF 1

typedef struct FileImageEntry {
    CString filePath;
    ULONGLONG size;
    BOOL autoAdded;
    LONG htmlIndex;
    BOOL stillPresent;
	CString personality;
	BOOL hasBootArchive;
	BOOL hasBootArchiveSig;
	BOOL hasUnpackedImgSig;
} FileImageEntry_t, *pFileImageEntry_t;

typedef enum ErrorCause {
    ErrorCauseGeneric,
    ErrorCauseCanceled,
    ErrorCauseJSONDownloadFailed,
    ErrorCauseDownloadFailed,
    ErrorCauseDownloadFailedDiskFull,
    ErrorCauseVerificationFailed,
    ErrorCauseWriteFailed,
    ErrorCauseNot64Bit,
    ErrorCauseBitLocker,
    ErrorCauseNotNTFS,
    ErrorCauseNonWindowsMBR,
    ErrorCauseNone
} ErrorCause_t;

typedef struct RemoteImageEntry {
    ULONGLONG compressedSize;
    ULONGLONG extractedSize;
    CString personality;
    CString urlFile;
    CString urlSignature;
    CString displayName;
    CString downloadJobName;
    CString version;
} RemoteImageEntry_t, *pRemoteImageEntry_t;

// CEndlessUsbToolDlg dialog
class CEndlessUsbToolDlg : public CDHtmlDialog
{
	// Construction
public:
	CEndlessUsbToolDlg(UINT globalMessage, bool enableLogDebugging, CWnd* pParent = NULL);	// standard constructor
    ~CEndlessUsbToolDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ENDLESSUSBTOOL_DIALOG, IDH = IDR_HTML_ENDLESSUSBTOOL_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// For dragging the window
	HRESULT OnHtmlMouseDown(IHTMLElement* pElement);
	//// Disable text selection
	//HRESULT OnHtmlSelectStart(IHTMLElement* pElement);

	// Dual Boot Page Handlers
	HRESULT OnAdvancedOptionsClicked(IHTMLElement* pElement);
	HRESULT OnInstallDualBootClicked(IHTMLElement* pElement);

	// First Page Handlers
	HRESULT OnTryEndlessSelected(IHTMLElement* pElement);
	HRESULT OnInstallEndlessSelected(IHTMLElement* pElement);
	HRESULT OnLinkClicked(IHTMLElement* pElement);
	HRESULT OnLanguageChanged(IHTMLElement* pElement);
	HRESULT OnFirstPagePreviousClicked(IHTMLElement* pElement);

	// Select File Page Handlers
	HRESULT OnSelectFilePreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectFileNextClicked(IHTMLElement* pElement);
	HRESULT OnSelectFileButtonClicked(IHTMLElement* pElement);
    HRESULT OnSelectedImageFileChanged(IHTMLElement* pElement);
    HRESULT OnSelectedRemoteFileChanged(IHTMLElement* pElement);
    HRESULT OnSelectedImageTypeChanged(IHTMLElement* pElement);
    HRESULT OnDownloadLightButtonClicked(IHTMLElement* pElement);
    HRESULT OnDownloadFullButtonClicked(IHTMLElement* pElement);

	// Select USB Page handlers
	HRESULT OnSelectUSBPreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectUSBNextClicked(IHTMLElement* pElement);
    HRESULT OnSelectedUSBDiskChanged(IHTMLElement* pElement);
    HRESULT OnAgreementCheckboxChanged(IHTMLElement* pElement);

	// Select Storage Page handlers
	HRESULT OnSelectStoragePreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectStorageNextClicked(IHTMLElement* pElement);
	HRESULT OnSelectedStorageSizeChanged(IHTMLElement* pElement);

    // Install Page handlers
    HRESULT OnInstallCancelClicked(IHTMLElement* pElement);

	// Error/Thank You page handlers
	HRESULT OnCloseAppClicked(IHTMLElement* pElement);
    HRESULT OnRecoverErrorButtonClicked(IHTMLElement* pElement);
    HRESULT OnDeleteCheckboxChanged(IHTMLElement* pElement);

	// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();

	BOOL PreTranslateMessage(MSG* pMsg);
	static void CALLBACK RefreshTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    BOOL CanAccessExternal() { return TRUE; } // disable a ActiveX warning
    void JavascriptDebug(LPCTSTR debugMsg);

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
    DECLARE_DISPATCH_MAP()

	STDMETHODIMP ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
	STDMETHODIMP TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID);

	// Browse navigation handling methods
	void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl);

private:
	bool m_dualBootSelected;
	int m_nrGigsSelected;
	bool m_liveInstall;
	loc_cmd* m_selectedLocale;
	char m_localizationFile[MAX_PATH];
	ULONG m_shellNotificationsRegister;
	uint64_t m_lastDevicesRefresh;
    BOOL m_lgpSet;
    BOOL m_lgpExistingKey;	// For LGP set/restore
    BOOL m_automount;
    HANDLE m_FilesChangedHandle;
    HANDLE m_cancelOperationEvent;
    HANDLE m_closeFileScanThreadEvent;
    HANDLE m_fileScanThread;
    HANDLE m_operationThread;
    HANDLE m_downloadUpdateThread;
    HANDLE m_checkConnectionThread;
    bool m_useLocalFile;
    long m_selectedRemoteIndex;
    long m_baseImageRemoteIndex;
    bool m_usbDeleteAgreement;
    bool m_closeRequested;
    int m_currentStep;
    bool m_isConnected;
    bool m_localFilesScanned;
    bool m_jsonDownloadAttempted;
    CMap<CString, LPCTSTR, pFileImageEntry_t, pFileImageEntry_t> m_imageFiles;
    CList<CString> m_imageIndexToPath;
    CList<RemoteImageEntry_t> m_remoteImages;
    RemoteImageEntry_t m_installerImage;
    FileImageEntry_t m_localInstallerImage;
    static CMap<CString, LPCTSTR, uint32_t, uint32_t> m_personalityToLocaleMsg;
    static CMap<CStringA, LPCSTR, CString, LPCTSTR> m_localeToPersonality;
    static CMap<CStringA, LPCSTR, CStringA, LPCSTR> m_localeToIniLocale;

    CString m_localFile;
    CString m_localFileSig;
    ULONGLONG m_selectedFileSize;

    CString m_LiveFile;
    CString m_LiveFileSig;
    static CString m_appDir;

	CString m_bootArchive;
	CString m_bootArchiveSig;
	CString m_unpackedImageSig;

	CComPtr<IHTMLDocument3> m_spHtmlDoc3;
    CComPtr<IHTMLElement> m_spStatusElem;
    CComPtr<IHTMLWindow2> m_spWindowElem;
    CComDispatchDriver m_dispWindow;
    CComPtr<IDispatchEx> m_dispexWindow;

    DownloadManager m_downloadManager;
    DWORD m_ieVersion;
    UINT m_globalWndMessage;

    bool m_enableLogDebugging;
    CFile m_logFile;
    ErrorCause_t m_lastErrorCause;
    long m_maximumUSBVersion;
	unsigned long m_cancelImageUnpack;

    void StartOperationThread(int operation, LPTHREAD_START_ROUTINE threadRoutine, LPVOID param = NULL);

	void StartInstallationProcess();

	void InitRufus();
    static DWORD WINAPI RufusISOScanThread(LPVOID param);

	void LoadLocalizationData();
	void AddLanguagesToUI();
	void ApplyRufusLocalization();

	void ChangePage(PCTSTR newPage);

	HRESULT GetSelectedOptionElementText(CComPtr<IHTMLSelectElement>, CString &text);
    HRESULT GetSelectElement(PCTSTR selectId, CComPtr<IHTMLSelectElement> &selectElem);
    HRESULT ClearSelectElement(PCTSTR selectId);
	HRESULT AddEntryToSelect(PCTSTR selectId, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE);
	HRESULT AddEntryToSelect(CComPtr<IHTMLSelectElement> &selectElem, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE);
    static bool IsButtonDisabled(IHTMLElement *pElement);

	void LeavingDevicesPage();

    void UpdateFileEntries(bool shouldInit = false);
    static DWORD WINAPI FileScanThread(void* param);

    void StartJSONDownload();
    void UpdateDownloadOptions();
    bool UnpackFile(LPCSTR archive, LPCSTR destination, int compressionType, void* progress_function = NULL, unsigned long* cancel_request = NULL);
    bool ParseJsonFile(LPCTSTR filename, bool isInstallerJson);
    void AddDownloadOptionsToUI();

	void Uninit();

    void ErrorOccured(ErrorCause_t errorCause);

    HRESULT CallJavascript(LPCTSTR method, CComVariant parameter1, CComVariant parameter2 = NULL);
    void UpdateCurrentStep(int currentStep);
    bool CancelInstall();
    DownloadType_t GetSelectedDownloadType();

    static DWORD WINAPI FileVerificationThread(void* param);
    static bool FileHashingCallback(__int64 currentSize, __int64 totalSize, LPVOID context);
    
    static DWORD WINAPI FileCopyThread(void* param);
    static DWORD CALLBACK CopyProgressRoutine(
        LARGE_INTEGER TotalFileSize,
        LARGE_INTEGER TotalBytesTransferred,
        LARGE_INTEGER StreamSize,
        LARGE_INTEGER StreamBytesTransferred,
        DWORD         dwStreamNumber,
        DWORD         dwCallbackReason,
        HANDLE        hSourceFile,
        HANDLE        hDestinationFile,
        LPVOID        lpData
    );

    static bool ParseImgFileName(const CString& filename, CString &personality, CString &version, bool &installerImage);
    void GetImgDisplayName(CString &displayName, const CString &version, const CString &personality, ULONGLONG size = 0);

    static ULONGLONG GetExtractedSize(const CString& filename, BOOL isInstallerImage);

    void GetIEVersion();

    static DWORD WINAPI UpdateDownloadProgressThread(void* param);
    static DWORD WINAPI CheckInternetConnectionThread(void* param);

    void GoToSelectFilePage();
    void InitLogging();
    void EnableHibernate(bool enable = true);
    void CancelRunningOperation();
    void StartCheckInternetConnectionThread();
    bool CanUseLocalFile();
    bool CanUseRemoteFile();
    void FindMaxUSBSpeed();
    void CheckUSBHub(LPCTSTR devicePath);
    void UpdateUSBSpeedMessage(DWORD deviceIndex);
    void JSONDownloadFailed();

	void GoToSelectStoragePage();
	BOOL AddStorageEntryToSelect(CComPtr<IHTMLSelectElement> &selectElement, int noOfGigs, uint8_t extraData);

	void ChangeDriveAutoRunAndMount(bool setEndlessValues);

	static DWORD WINAPI CreateUSBStick(LPVOID param);
	static bool CreateFakePartitionLayout(HANDLE hPhysical, PBYTE layout, PBYTE geometry);
	static bool FormatFirstPartitionOnDrive(DWORD DriveIndex, int fsToUse, HANDLE m_cancelOperationEvent, const wchar_t *partLabel);
	static bool MountFirstPartitionOnDrive(DWORD DriveIndex, CString &driveLetter);
	static bool CreateCorrectPartitionLayout(HANDLE hPhysical, PBYTE layout, PBYTE geometry);

	static bool UnpackZip(const CComBSTR source, const CComBSTR dest);
	static void RemoveNonEmptyDirectory(const CString directoryPath);
	static bool CopyFilesToESP(const CString &fromFolder, const CString &driveLetter);
	static void ImageUnpackCallback(const uint64_t read_bytes);
	static bool CopyFilesToexFAT(CEndlessUsbToolDlg *dlg, const CString &fromFolder, const CString &driveLetter);
	static bool WriteMBRAndSBRToUSB(HANDLE hPhysical, const CString &bootFilesPath, DWORD bytesPerSector);

	static DWORD WINAPI SetupDualBoot(LPVOID param);

	static bool ExtendImageFile(const CString &endlessImgPath, ULONGLONG selectedGigs);
	static bool UnpackBootComponents(const CString &bootFilesPathGz, const CString &bootFilesPath);
	static bool CopyMultipleItems(const CString &fromPath, const CString &toPath);
	static bool IsLegacyBIOSBoot();
	static bool IsWindowsMBR(FILE* fpDrive, const CString &TargetName);
	static bool WriteMBRAndSBRToWinDrive(const CString &systemDriveLetter, const CString &bootFilesPath, const CString &endlessFilesPath);
	static bool SetupEndlessEFI(const CString &systemDriveLetter, const CString &bootFilesPath);
	static HANDLE GetPhysicalFromDriveLetter(const CString &driveLetter);

	// used by ImageUnpackCallback
	// bled doesn't allow us to set a context variable for the callback
	static int ImageUnpackOperation;
	static int ImageUnpackPercentStart;
	static int ImageUnpackPercentEnd;
	static ULONGLONG ImageUnpackFileSize;

	static bool Has64BitSupport();

	static BOOL SetAttributesForFilesInFolder(CString path, bool addAttributes);
	static BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	static BOOL ChangeAccessPermissions(CString path, bool restrictAccess);

	static CStringW GetSystemDrive();
	static BOOL SetEndlessRegistryKey(HKEY parentKey, const CString &keyPath, const CString &keyName, CComVariant keyValue, bool createBackup = true);
	static BOOL IsBitlockedDrive(const CString &drive);
};
