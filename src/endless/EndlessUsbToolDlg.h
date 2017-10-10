
// EndlessUsbToolDlg.h : header file
//

#pragma once

#include <list>

#include "localization.h"
#include "DownloadManager.h"
#include "EndlessISO.h"
#include "Eosldr.h"

typedef struct FileImageEntry {
    // Full, real path on disk
    CString filePath;
    // Size of file on disk
    ULONGLONG fileSize;
    // Size of Endless OS image (identical to fileSize for uncompressed images,
    // substantially larger for compressed images)
    ULONGLONG extractedSize;
    BOOL autoAdded;
    LONG htmlIndex;
    BOOL stillPresent;
    CString personality;
    CString bootArchivePath;
    CString bootArchiveSigPath;
    ULONGLONG bootArchiveSize;
    // Signature for 'filePath' (usually filePath + ".asc", except when
    // operating on a live USB)
    CString imgSigPath;
    // Signature matching the uncompressed image inside 'filePath' (equal to
    // imgSigPath if the image is already uncompressed)
    CString unpackedImgSigPath;
    // TRUE if bootArchivePath exists
    BOOL hasBootArchive;
    // TRUE if bootArchiveSigPath exists
    BOOL hasBootArchiveSig;
    // TRUE if unpackedImgSigPath exists
    BOOL hasUnpackedImgSig;
    CString version;
    CString date;
} FileImageEntry_t, *pFileImageEntry_t;

typedef enum ErrorCause {
    ErrorCauseNone,
    ErrorCauseGeneric,
    ErrorCauseCancelled,
    ErrorCauseDownloadFailed,
    ErrorCauseDownloadFailedDiskFull,
    ErrorCauseVerificationFailed,
    ErrorCauseWriteFailed,
    ErrorCauseNot64Bit,
    ErrorCause32BitEFI,
    ErrorCauseBitLocker,
    ErrorCauseNotNTFS,
    ErrorCauseNonWindowsMBR,
    ErrorCauseNonEndlessMBR,
    ErrorCauseCantCheckMBR,
    ErrorCauseInstallFailedDiskFull,
    ErrorCauseSuspended,
    ErrorCauseInstallEosldrFailed,
    ErrorCauseUninstallEosldrFailed,
    ErrorCauseCantUnpackBootZip,
} ErrorCause_t;

typedef struct RemoteImageEntry {
    ULONGLONG compressedSize;
    ULONGLONG extractedSize;
    CString personality;
    CString urlFile;
    CString urlSignature;
	CString urlUnpackedSignature;
    CString urlBootArchive;
    CString urlBootArchiveSignature;
	ULONGLONG bootArchiveSize;
    CString downloadJobName;
    CString version;
} RemoteImageEntry_t, *pRemoteImageEntry_t;

typedef enum InstallMethod {
	None,
	LiveUsb,
	ReformatterUsb,
	CombinedUsb,
	InstallDualBoot,
	UninstallDualBoot
} InstallMethod_t;

enum JSONDownloadState {
    Pending,
    Retrying,
    Failed,
    Succeeded
};

enum CompressionType {
    CompressionTypeUnknown = 0,
    CompressionTypeNone,
    CompressionTypeGz,
    CompressionTypeXz,
    CompressionTypeSquash,
};

struct SignedFile_t {
    SignedFile_t(const CString &filePath_, ULONGLONG fileSize_, const CString &sigPath_)
        :
        filePath(filePath_),
        fileSize(fileSize_),
        sigPath(sigPath_)
    {}

    CString filePath;
    ULONGLONG fileSize;
    CString sigPath;
};

// CEndlessUsbToolDlg dialog
class CEndlessUsbToolDlg : public CDHtmlDialog
{
	// Construction
public:
	CEndlessUsbToolDlg(UINT globalMessage, CWnd* pParent = NULL);	// standard constructor
    ~CEndlessUsbToolDlg();

	static bool IsUninstaller();
	static bool IsCoding();
	static bool ShouldUninstall();

	void Uninit();

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

	// Advanced Page Handlers
	HRESULT OnLiveUsbClicked(IHTMLElement* pElement);
	HRESULT OnReformatterUsbClicked(IHTMLElement* pElement);
	HRESULT OnLinkClicked(IHTMLElement* pElement);
	HRESULT OnLanguageChanged(IHTMLElement* pElement);
	HRESULT OnAdvancedPagePreviousClicked(IHTMLElement* pElement);
	HRESULT OnCombinedUsbButtonClicked(IHTMLElement* pElement);

	// Select File Page Handlers
	HRESULT OnSelectFilePreviousClicked(IHTMLElement* pElement);
	HRESULT OnSelectFileNextClicked(IHTMLElement* pElement);
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

	LRESULT OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);

private:
	BOOL m_initialized;
	uint64_t m_selectedInstallSizeBytes;
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
    JSONDownloadState m_jsonDownloadState;
    CMap<CString, LPCTSTR, pFileImageEntry_t, pFileImageEntry_t> m_imageFiles;
    CList<CString> m_imageIndexToPath;
    CList<RemoteImageEntry_t> m_remoteImages;
    RemoteImageEntry_t m_installerImage;
    FileImageEntry_t m_localInstallerImage;
    static CMap<CString, LPCTSTR, uint32_t, uint32_t> m_personalityToLocaleMsg;
    static CMap<CStringA, LPCSTR, CString, LPCTSTR> m_localeToPersonality;
    // Language-specific personalities covered by the 'sea' personality.
    static CList<CString> m_seaPersonalities;
    static CMap<CStringA, LPCSTR, CStringA, LPCSTR> m_localeToIniLocale;

    // The OS image file. For ReformatterUsb, this is still the 'eos' image;
    // for the 'eosinstaller' image see m_localInstallerImage. Note that
    // ReformatterUsb and LiveUsb delegate to Rufus proper to write an image
    // to disk, using a global variable 'image_path'.
    CString m_localFile;
    // Path to signature for m_localFile. In the ISO case, this is a signature
    // for the uncompressed image inside 'endless.squash'.
    CString m_localFileSig;
    // Path to signature for the decompressed contents of m_localFile.
    // If m_localFile is not compressed or is a SquashFS, this will be
    // identical to m_localFileSig. Used for CombinedUsb.
    CString m_unpackedImageSig;

    // (Compressed) size of m_localFile.
    ULONGLONG m_selectedFileSize;

    /* The size of data ultimately written to disk, taking into account the
       eosinstaller image for old-style installers and the extracted image
       size for every other mode.

       Only valid once an image has been selected.
    */
    ULONGLONG m_finalSize;

    // Path to boot archive; used for CombinedUsb and DualBoot
    CString m_bootArchive;
    // Path to signature for m_bootArchive
    CString m_bootArchiveSig;
    ULONGLONG m_bootArchiveSize;

    // Files to verify. When verification fails, the head of this list is the
    // mismatching file/sig pair.
    std::list<SignedFile_t> m_verifyFiles;
    // Guards access to m_verifyFiles
    CCriticalSection m_verifyFilesMutex;

	CComPtr<IHTMLDocument3> m_spHtmlDoc3;
    CComPtr<IHTMLElement> m_spStatusElem;
    CComPtr<IHTMLWindow2> m_spWindowElem;
    CComDispatchDriver m_dispWindow;
    CComPtr<IDispatchEx> m_dispexWindow;

	CComPtr<ITaskbarList3> m_taskbarProgress;

    DownloadManager m_downloadManager;
    EndlessISO m_iso;
    DWORD m_ieVersion;
    UINT m_globalWndMessage;

    ErrorCause_t m_lastErrorCause;
    long m_maximumUSBVersion;
	unsigned long m_cancelImageUnpack;
	InstallMethod_t m_selectedInstallMethod;

	std::unique_ptr<EosldrInstaller> m_eosldrInstaller;

	void TrackEvent(const CString &action, const CString &label = CString(), LONGLONG value = -1L);
	void TrackEvent(const CString &action, LONGLONG value);
	void SetSelectedInstallMethod(InstallMethod_t method);

    void StartOperationThread(int operation, LPTHREAD_START_ROUTINE threadRoutine, LPVOID param = NULL);

	void StartInstallationProcess();

	void InitRufus();
    static DWORD WINAPI RufusISOScanThread(LPVOID param);

	void LoadLocalizationData();
	void AddLanguagesToUI();
	void ApplyRufusLocalization();
	void ShowWindowsTooOldError();
	void ShowIETooOldError();

	void ChangePage(PCTSTR newPage);

	HRESULT UpdateSelectOptionText(CComPtr<IHTMLSelectElement> selectElement, const CString &text, LONG index);
	HRESULT GetSelectedValue(IHTMLElement* pElement, CComBSTR &selectedValue);
	HRESULT GetSelectedOptionElementText(CComPtr<IHTMLSelectElement>, CString &text);
    HRESULT GetSelectElement(PCTSTR selectId, CComPtr<IHTMLSelectElement> &selectElem);
    HRESULT ClearSelectElement(PCTSTR selectId);
	HRESULT AddEntryToSelect(PCTSTR selectId, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE);
	HRESULT AddEntryToSelect(CComPtr<IHTMLSelectElement> &selectElem, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected = FALSE, const CComBSTR &title = L"");
    static bool IsButtonDisabled(IHTMLElement *pElement);

	void LeavingDevicesPage();

    void UpdateFileEntries(bool shouldInit = false);
    static DWORD WINAPI FileScanThread(void* param);

    void StartJSONDownload();
    void UpdateDownloadOptions();
    // Unpack a file, using BLED. 'compressionType' must be a member of enum bled_compression_type
    bool UnpackFile(const CString &archive, const CString &destination, int compressionType = 0, void* progress_function = NULL, unsigned long* cancel_request = NULL);
    // Unpack an OS image to the given destination, by whatever means are necessary
    bool UnpackImage(const CString &image, const CString &destination);
    bool ParseJsonFile(LPCTSTR filename, bool isInstallerJson);
    void GetPreferredPersonality(CString &personality);
    void AddDownloadOptionsToUI();
    void UpdateDownloadableState();

    void ErrorOccured(ErrorCause_t errorCause);

    HRESULT CallJavascript(LPCTSTR method, CComVariant parameter1, CComVariant parameter2 = NULL);
    void UpdateCurrentStep(int currentStep);
    bool CancelInstall();
    DownloadType_t GetSelectedDownloadType();

    void StartFileVerificationThread();
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

    const CString LocalizePersonalityName(const CString &personality);
    void GetImgDisplayName(CString &displayName, const CString &version, const CString &personality, ULONGLONG size = 0);
    static CompressionType GetCompressionType(const CString& filename);
    static int GetBledCompressionType(const CompressionType type);
    ULONGLONG GetExtractedSize(const CString& filename, BOOL isInstallerImage, CompressionType &compressionType);

    void GetIEVersion();

    static DWORD WINAPI UpdateDownloadProgressThread(void* param);
    static DWORD WINAPI CheckInternetConnectionThread(void* param);

    void GoToSelectFilePage(bool forwards);
    void EnableHibernate(bool enable = true);
    void CancelRunningOperation(bool userCancel = false);
    void StartCheckInternetConnectionThread();
    bool CanUseLocalFile();
    bool CanUseRemoteFile();
    void FindMaxUSBSpeed();
    void CheckUSBHub(LPCTSTR devicePath);
    void UpdateUSBSpeedMessage(int deviceIndex);
    void SetJSONDownloadState(JSONDownloadState state);

	void GoToSelectStoragePage();
	BOOL AddStorageEntryToSelect(CComPtr<IHTMLSelectElement> &selectElement, const uint64_t bytes, uint8_t extraData);

	void ChangeDriveAutoRunAndMount(bool setEndlessValues);

	static DWORD WINAPI CreateUSBStick(LPVOID param);
	static bool CreateUSBPartitionLayout(HANDLE hPhysical, DWORD &BytesPerSector);
	static bool FormatFirstPartitionOnDrive(DWORD DriveIndex, const wchar_t *kFSType, ULONG ulClusterSize, HANDLE cancelEvent, const wchar_t *kPartLabel);
	static bool FormatPartitionWithRetry(const char *partition, const wchar_t *kFSType, ULONG ulClusterSize, HANDLE cancelEvent, const wchar_t *kPartLabel);
	static bool MountFirstPartitionOnDrive(DWORD DriveIndex, CStringA &driveLetter);

	static bool UnpackZip(const CComBSTR source, const CComBSTR dest);
	static void RemoveNonEmptyDirectory(const CString directoryPath);
	static bool CopyFilesToESP(const CString &fromFolder, const CString &driveLetter);
	static void ImageUnpackCallback(const uint64_t read_bytes);
	static void UpdateUnpackProgress(const uint64_t current_bytes, const uint64_t total_bytes);
	static bool CopyFilesToexFAT(CEndlessUsbToolDlg *dlg, const CString &fromFolder, const CString &driveLetter);
	static bool WriteMBRAndSBRToUSB(HANDLE hPhysical, const CString &bootFilesPath, DWORD bytesPerSector);

	static DWORD WINAPI SetupDualBoot(LPVOID param);
	static bool SetupDualBootFiles(CEndlessUsbToolDlg *dlg, const CString &systemDriveLetter, const CString &bootFilesPath, ErrorCause &errorCause);

	static bool EnsureUncompressed(const CString &filePath);
	static bool ExtendImageFile(const CString &endlessImgPath, uint64_t bytes);
	static bool UnpackBootComponents(const CString &bootFilesZipPath, const CString &bootFilesPath);
	static bool CopyMultipleItems(const CString &fromPath, const CString &toPath);
	static bool IsLegacyBIOSBoot();
	static bool IsWindowsMBR(FILE* fpDrive, const CString &TargetName);
	static bool CanInstallToDrive(const CString &systemDriveLetter, const bool isBIOS, const bool canInstallEosldr, ErrorCause &cause);
	static bool WriteMBRAndSBRToWinDrive(CEndlessUsbToolDlg *dlg, const CString &systemDriveLetter, const CString &bootFilesPath, const CString &endlessFilesPath);
	static bool SetupEndlessEFI(const CString &systemDriveLetter, const CString &bootFilesPath);
	static HANDLE GetPhysicalFromDriveLetter(const CString &driveLetter);

	// used by ImageUnpackCallback
	// bled doesn't allow us to set a context variable for the callback
	static int ImageUnpackOperation;
	static int ImageUnpackPercentStart;
	static int ImageUnpackPercentEnd;
	static ULONGLONG ImageUnpackFileSize;

	static bool Has64BitSupport();

	static BOOL SetAttributesForFilesInFolder(CString path, DWORD attributes);
	static BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
	static BOOL ChangeAccessPermissions(CString path, bool restrictAccess);

	static CStringW GetSystemDrive();
	static BOOL SetEndlessRegistryKey(HKEY parentKey, const CString &keyPath, const CString &keyName, CComVariant keyValue);

	static CStringW GetExePath();
	static BOOL AddUninstallRegistryKeys(const CStringW &uninstallExePath, const CStringW &installPath);

	static BOOL UninstallDualBoot(CEndlessUsbToolDlg *dlg);
	static BOOL ResetEndlessRegistryKey(HKEY parentKey, const CString &keyPath, const CString &keyName);

	static BOOL MountESPFromDrive(HANDLE hPhysical, const char **espMountLetter, const CString &systemDriveLetter);

	static void DelayDeleteFolder(const CString &folder);

	static bool HasImageBootSupport(const CString &version, const CString &date);

	bool PackedImageAlreadyExists(const CString &filePath, ULONGLONG expectedSize, ULONGLONG expectedUnpackedSize, bool isInstaller);

	ULONGLONG GetActualDownloadSize(const RemoteImageEntry &r, bool fullSize = false);
	bool RemoteMatchesUnpackedImg(const CString &remoteFilePath, CString *unpackedImgSig = NULL);
	bool IsDualBootOrCombinedUsb();
	void UpdateDualBootTexts();
	void QueryAndDoUninstall();
	static bool IsEndlessMBR(FILE* fp, const CString &systemDriveLetter);
	static bool UninstallEndlessMBR(const CString &systemDriveLetter, const CString &endlessFilesPath, HANDLE hPhysical, ErrorCause &cause);
	static bool UninstallEndlessEFI(const CString &systemDriveLetter, HANDLE hPhysical, bool &found_boot_entry);
	static bool RestoreOriginalBoottrack(const CString &endlessPath, HANDLE hPhysical, FILE *fp);

	CComBSTR GetDownloadString(const RemoteImageEntry &imageEntry);

	static ULONGLONG RoundToSector(ULONGLONG size);
	ULONGLONG GetNeededSpaceForDualBoot(ULONGLONG *downloadSize, bool *isBaseImage = NULL);

	static const UINT m_uTaskbarBtnCreatedMsg;

	static const wchar_t* UninstallerFileName();
	static const char* JsonLiveFile(bool withCompressedSuffix=true);
	static const char* JsonInstallerFile(bool withCompressedSuffix=true);
	static const wchar_t* JsonLiveFileURL();
	static const wchar_t* JsonInstallerFileURL();
};
