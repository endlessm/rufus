// EndlessUsbToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EndlessUsbTool.h"
#include "EndlessUsbToolDlg.h"
#include "Analytics.h"
#include "afxdialogex.h"

#include <windowsx.h>
#include <dbt.h>
#include <atlpath.h>
#include <intrin.h>
#include <Aclapi.h>
#include <Wbemidl.h>

#include "json/json.h"
#include <fstream>
#include "Version.h"
#include "WindowsUsbDefines.h"

// Rufus include files
extern "C" {
#include "rufus.h"
#include "missing.h"
#include "msapi_utf8.h"
#include "drive.h"
#include "bled/bled.h"
#include "file.h"
#include "ms-sys/inc/br.h"

#include "usb.h"
#include "mbr_grub2.h"
#include "grub/grub.h"


// RADU: try to remove the need for all of these
OPENED_LIBRARIES_VARS;
HWND hMainDialog = NULL, hLog = NULL;
BOOL right_to_left_mode = FALSE;
GetTickCount64_t pfGetTickCount64 = NULL;
RUFUS_UPDATE update = { { 0,0,0 },{ 0,0 }, NULL, NULL };
char *ini_file = NULL;
BOOL usb_debug = FALSE;
WORD selected_langid = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

extern const loc_control_id control_id[];
extern loc_dlg_list loc_dlg[];
extern char** msg_table;

// Trying to reuse as much as rufus as possible
HWND hDeviceList = NULL, hBootType = NULL, hPartitionScheme = NULL, hFileSystem = NULL, hClusterSize = NULL, hLabel = NULL;
HWND hNBPasses = NULL, hDiskID = NULL, hInfo = NULL;
HINSTANCE hMainInstance = NULL;

BOOL detect_fakes = FALSE;

char app_dir[MAX_PATH], system_dir[MAX_PATH], sysnative_dir[MAX_PATH];
char* image_path = NULL;
// Number of steps for each FS for FCC_STRUCTURE_PROGRESS
const int nb_steps[FS_MAX] = { 5, 5, 12, 1, 10 };
BOOL format_op_in_progress = FALSE;
BOOL allow_dual_uefi_bios, togo_mode, force_large_fat32, enable_ntfs_compression = FALSE, lock_drive = TRUE;
BOOL zero_drive = FALSE, preserve_timestamps, list_non_usb_removable_drives = FALSE;
BOOL use_own_c32[NB_OLD_C32] = { FALSE, FALSE };
uint16_t rufus_version[3], embedded_sl_version[2];
char embedded_sl_version_str[2][12] = { "?.??", "?.??" };
char embedded_sl_version_ext[2][32];
uint32_t dur_mins, dur_secs;
StrArray DriveID, DriveLabel;
BOOL enable_HDDs = TRUE, use_fake_units, enable_vmdk;
int dialog_showing = 0;

PF_TYPE_DECL(WINAPI, BOOL, SHChangeNotifyDeregister, (ULONG));
PF_TYPE_DECL(WINAPI, ULONG, SHChangeNotifyRegister, (HWND, int, LONG, UINT, int, const SHChangeNotifyEntry *));

BOOL FormatDrive(DWORD DriveIndex, int fsToUse, const wchar_t *partLabel);

extern HANDLE GlobalLoggingMutex;

// Added by us so we don't go through the hastle of getting device speed again
// Rufus code already does it
DWORD usbDeviceSpeed[128];
BOOL usbDeviceSpeedIsLower[128];
DWORD usbDevicesCount;
};

#include "localization.h"
// End Rufus include files

#include "gpt/gpt.h"
#include "PGPSignature.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define HTML_BUTTON_ID(__id__)          (__id__##"C")
// HTML element ids and classes
// pages
#define ELEMENT_DUALBOOT_PAGE           "DualBootInstallPage"
#define ELEMENT_FIRST_PAGE              "FirstPage"
#define ELEMENT_FILE_PAGE               "SelectFilePage"
#define ELEMENT_USB_PAGE                "SelectUSBPage"
#define ELEMENT_STORAGE_PAGE            "SelectStoragePage"
#define ELEMENT_INSTALL_PAGE            "InstallingPage"
#define ELEMENT_SUCCESS_PAGE            "ThankYouPage"
#define ELEMENT_ERROR_PAGE              "ErrorPage"

//classes
#define CLASS_PAGE_HEADER_TITLE         "PageHeaderTitle"
#define CLASS_PAGE_HEADER               "PageHeader"
#define CLASS_BUTTON_DISABLED           "ButtonDisabled"

//Dual boot elements
#define ELEMENT_LANGUAGE_DUALBOOT       "LanguageSelectDualBoot"
#define ELEMENT_DUALBOOT_CLOSE_BUTTON   "DualBootPageCloseButton"
#define ELEMENT_DUALBOOT_ADVANCED_LINK  "AdvancedOptionsLink"
#define ELEMENT_DUALBOOT_INSTALL_BUTTON "DualBootInstallButon"

//First page elements
#define ELEMENT_TRY_BUTTON              "TryEndlessButton"
#define ELEMENT_INSTALL_BUTTON          "InstallEndlessButton"
#define ELEMENT_COMPARE_OPTIONS         "CompareOptionsLink"
#define ELEMENT_LANGUAGE_SELECT         "LanguageSelect"
#define ELEMENT_FIRST_CLOSE_BUTTON      "FirstPageCloseButton"
#define ELEMENT_FIRST_PREV_BUTTON       "FirstPagePreviousButton"

//Select File page elements
#define ELEMENT_SELFILE_PREV_BUTTON     "SelectFilePreviousButton"
#define ELEMENT_SELFILE_NEXT_BUTTON     "SelectFileNextButton"
#define ELEMENT_SELFILE_BUTTON          "SelectFileButton"
#define ELEMENT_FILES_SELECT            "LocalImagesSelect"
#define ELEMENT_REMOTE_SELECT           "OnlineImagesSelect"
#define ELEMENT_IMAGE_TYPE_LOCAL        "OperatingSystemTypeLocal"
#define ELEMENT_IMAGE_TYPE_REMOTE       "OperatingSystemTypeOnline"
#define ELEMENT_SELFILE_NO_CONNECTION   "NoInternetConnection"
#define ELEMENT_SELFILE_DOWN_LANG       "DownloadLanguageSelect"

#define ELEMENT_LOCAL_FILES_FOUND       "SelectFileLocalPresent"
#define ELEMENT_LOCAL_FILES_NOT_FOUND   "SelectFileLocalNotPresent"

#define ELEMENT_DOWNLOAD_LIGHT_BUTTON   "DownloadLightButton"
#define ELEMENT_DOWNLOAD_FULL_BUTTON    "DownloadFullButton"

#define ELEMENT_DOWNLOAD_LIGHT_SIZE     "LightDownloadSubtitle"
#define ELEMENT_DOWNLOAD_FULL_SIZE      "FullDownloadSubtitle"

#define ELEMENT_CONNECTED_LINK          "ConnectedLink"
#define ELEMENT_CONNECTED_SUPPORT_LINK  "ConnectedSupportLink"

#define ELEMENT_SET_FILE_TITLE          "SelectFilePageTitle"
#define ELEMENT_SET_FILE_SUBTITLE       "SelectFileSubtitle"

//Select USB page elements
#define ELEMENT_SELUSB_PREV_BUTTON      "SelectUSBPreviousButton"
#define ELEMENT_SELUSB_NEXT_BUTTON      "SelectUSBNextButton"
#define ELEMENT_SELUSB_USB_DRIVES       "USBDiskSelect"
#define ELEMENT_SELUSB_NEW_DISK_NAME    "NewDiskName"
#define ELEMENT_SELUSB_NEW_DISK_SIZE    "NewDiskSize"
#define ELEMENT_SELUSB_AGREEMENT        "AgreementCheckbox"
#define ELEMENT_SELUSB_SPEEDWARNING     "UsbSpeedWarning"

//Select Storage page elements
#define ELEMENT_SELSTORAGE_PREV_BUTTON  "SelectStoragePreviousButton"
#define ELEMENT_SELSTORAGE_NEXT_BUTTON  "SelectStorageNextButton"
#define ELEMENT_STORAGE_SELECT          "StorageSpaceSelect"
#define ELEMENT_STORAGE_DESCRIPTION     "StorageSpaceDescription"
#define ELEMENT_STORAGE_AVAILABLE       "SelectStorageAvailableSpace"
#define ELEMENT_STORAGE_MESSAGE			"SelectStorageSubtitle"
#define ELEMENT_STORAGE_SUPPORT_LINK	"StorageSupportLink"

//Installing page elements
#define ELEMENT_INSTALL_STATUS          "InstallStepStatus"
#define ELEMENT_INSTALL_STEP            "CurrentStepText"
#define ELEMENT_INSTALL_STEPS_TOTAL     "TotalStepsText"
#define ELEMENT_INSTALL_STEP_TEXT       "CurrentStepDescription"
#define ELEMENT_INSTALL_CANCEL          "InstallCancelButton"

//Thank You page elements
#define ELEMENT_CLOSE_BUTTON            "CloseAppButton"
#define ELEMENT_INSTALLER_VERSION       "InstallerVersionValue"
#define ELEMENT_INSTALLER_LANGUAGE_ROW  "InstallerLanguageRow"
#define ELEMENT_INSTALLER_LANGUAGE      "InstallerLanguageValue"
#define ELEMENT_INSTALLER_CONTENT       "InstallerContentValue"
#define ELEMENT_THANKYOU_MESSAGE        "ThankYouMessage"
#define ELEMENT_DUALBOOT_REMINDER       "ThankYouDualBootReminder"
#define ELEMENT_LIVE_REMINDER           "ThankYouLiveReminder"
#define ELEMENT_REFLASHER_REMINDER      "ThankYouReflasherReminder"
#define ELEMENT_USBBOOT_HOWTO           "UsbBootHowToLink"
//Error page
#define ELEMENT_ERROR_MESSAGE           "ErrorMessage"
#define ELEMENT_ERROR_CLOSE_BUTTON      "CloseAppButton1"
#define ELEMENT_ENDLESS_SUPPORT         "EndlessSupport"
#define ELEMENT_ERROR_SUGGESTION        "ErrorMessageSuggestion"
#define ELEMENT_ERROR_BUTTON            "ErrorContinueButton"
#define ELEMENT_ERROR_DELETE_CHECKBOX   "DeleteFilesCheckbox"
#define ELEMENT_ERROR_DELETE_TEXT       "DeleteFilesText"

#define ELEMENT_VERSION_CONTAINER       "VersionContainer"

// Javascript methods
#define JS_SET_PROGRESS                 "setProgress"
#define JS_ENABLE_DOWNLOAD              "enableDownload"
#define JS_ENABLE_ELEMENT               "enableElement"
#define JS_ENABLE_BUTTON                "enableButton"
#define JS_SHOW_ELEMENT                 "showElement"
#define JS_RESET_CHECK                  "resetCheck"

// Personalities

#define PERSONALITY_BASE                L"base"
#define PERSONALITY_ENGLISH             L"en"
#define PERSONALITY_SPANISH             L"es"
#define PERSONALITY_PORTUGUESE          L"pt_BR"
#define PERSONALITY_ARABIC              L"ar"
#define PERSONALITY_FRENCH              L"fr"
#define PERSONALITY_CHINESE             L"zh_CN"
#define PERSONALITY_BENGALI             L"bn"
#define PERSONALITY_SPANISH_GT          L"es_GT"

static const wchar_t *globalAvailablePersonalities[] =
{
    PERSONALITY_BASE,
    PERSONALITY_ENGLISH,
    PERSONALITY_SPANISH,
    PERSONALITY_PORTUGUESE,
    PERSONALITY_ARABIC,
    PERSONALITY_FRENCH,
    PERSONALITY_CHINESE,
    PERSONALITY_BENGALI,
    PERSONALITY_SPANISH_GT,
};

static_assert(
    ARRAYSIZE(globalAvailablePersonalities) == MSG_MAX - MSG_400,
    "Have you updated resource.h, localization_data.h and endless.loc?"
);

// Rufus language codes
#define RUFUS_LOCALE_EN     "en-US"
#define RUFUS_LOCALE_ES     "es-ES"
#define RUFUS_LOCALE_PT     "pt-BR"
#define RUFUS_LOCALE_SA     "ar-SA"
#define RUFUS_LOCALE_FR     "fr-FR"
#define RUFUS_LOCALE_ZH_CN  "zh-CN"
#define RUFUS_LOCALE_BN     "bn-BD"

// INI file language codes
#define INI_LOCALE_EN       "en_US.utf8"
#define INI_LOCALE_ES       "es_MX.utf8"
#define INI_LOCALE_PT       "pt_BR.utf8"
#define INI_LOCALE_SA       "ar_AE.utf8"
#define INI_LOCALE_FR       "fr_FR.utf8"
#define INI_LOCALE_ZH       "zh_CN.utf8"
#define INI_LOCALE_BN       "bn_BD.utf8"


#define GET_LOCAL_PATH(__filename__) (m_appDir + "\\" + (__filename__))
#define CSTRING_GET_LAST(__path__, __separator__) __path__.Right(__path__.GetLength() - __path__.ReverseFind(__separator__) - 1)
#define CSTRING_GET_PATH(__path__, __separator__) __path__.Left(__path__.ReverseFind(__separator__))

enum custom_message {
    WM_FILES_CHANGED = UM_NO_UPDATE + 1,
    WM_FINISHED_IMG_SCANNING,
    WM_UPDATE_PROGRESS,
    WM_FILE_DOWNLOAD_STATUS,
    WM_FINISHED_FILE_VERIFICATION,
    WM_FINISHED_FILE_COPY,
    WM_FINISHED_ALL_OPERATIONS,
    WM_INTERNET_CONNECTION_STATE,
};

enum endless_action_type {    
    OP_DOWNLOADING_FILES = OP_MAX,
    OP_VERIFYING_SIGNATURE,
    OP_FLASHING_DEVICE,
    OP_FILE_COPY,
	OP_SETUP_DUALBOOT,
    OP_NO_OPERATION_IN_PROGRESS,
    OP_ENDLESS_MAX
};

#define TID_UPDATE_FILES                TID_REFRESH_TIMER + 1

//#define ENABLE_JSON_COMPRESSION 1

#define BOOT_COMPONENTS_FOLDER	"EndlessBoot"

#define RELEASE_JSON_URLPATH    _T("https://d1anzknqnc1kmb.cloudfront.net/")
#define JSON_LIVE_FILE          "releases-eos.json"
#define JSON_INSTALLER_FILE     "releases-eosinstaller.json"
#define JSON_GZIP               ".gz"
#define JSON_PACKED(__file__)   __file__ JSON_GZIP
#ifdef ENABLE_JSON_COMPRESSION
#define JSON_URL(__file__)      RELEASE_JSON_URLPATH _T(JSON_PACKED(__file__))
#else
#define JSON_URL(__file__)      RELEASE_JSON_URLPATH _T(__file__)
#endif // ENABLE_JSON_COMPRESSION
#define SIGNATURE_FILE_EXT      L".asc"
#define	BOOT_ARCHIVE_SUFFIX		L".boot.zip"
#define	IMAGE_FILE_EXT			L".img"

#define ENDLESS_OS "Endless OS"
#define EOS_PRODUCT_TEXT            "eos"
#define EOS_INSTALLER_PRODUCT_TEXT  "eosinstaller"
#define EOS_NONFREE_PRODUCT_TEXT    "eosnonfree"
#define EOS_OEM_PRODUCT_TEXT		"eosoem"
const wchar_t* mainWindowTitle = L"Endless Installer";

#define ALL_FILES					L"*.*"

#define ENDLESS_UNINSTALLER_NAME L"endless-uninstaller.exe"
#define ENDLESS_OS_NAME L"Endless OS"

//#define HARDCODED_PATH L"release/3.0.2/eos-amd64-amd64/base/eos-eos3.0-amd64-amd64.160827-104530.base"
#define HARDCODED_PATH L"eos-amd64-amd64/eos3.0/base/160906-030224/eos-eos3.0-amd64-amd64.160906-030224.base"
CString hardcoded_BootPath(HARDCODED_PATH BOOT_ARCHIVE_SUFFIX);
CString hardcoded_BootPathAsc(HARDCODED_PATH BOOT_ARCHIVE_SUFFIX SIGNATURE_FILE_EXT);

// Radu: How much do we need to reserve for the exfat partition header?
// reserve 10 mb for now; this will also include the signature file
#define INSTALLER_DELTA_SIZE (10*1024*1024)


#define UPDATE_DOWNLOAD_PROGRESS_TIME       2000
#define CHECK_INTERNET_CONNECTION_TIME      2000


#define FORMAT_STATUS_CANCEL (ERROR_SEVERITY_ERROR | FAC(FACILITY_STORAGE) | ERROR_CANCELLED)

// utility method for quick char* UTF8 conversion to BSTR
CComBSTR UTF8ToBSTR(const char *txt) {
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, txt, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[wchars_num];
	MultiByteToWideChar(CP_UTF8, 0, txt, -1, wstr, wchars_num);
	CComBSTR return_value(wstr);
	delete[] wstr;

	return return_value;
}

CString UTF8ToCString(const char *txt) {
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, txt, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[wchars_num];
    MultiByteToWideChar(CP_UTF8, 0, txt, -1, wstr, wchars_num);
    CString return_value(wstr);
    delete[] wstr;

    return return_value;
}

CStringA ConvertUnicodeToUTF8(const CStringW& uni)
{
    if (uni.IsEmpty()) return ""; // nothing to do
    CStringA utf8;
    int cc = 0;
    // get length (cc) of the new multibyte string excluding the \0 terminator first
    if ((cc = WideCharToMultiByte(CP_UTF8, 0, uni, -1, NULL, 0, 0, 0) - 1) > 0)
    {
        // convert
        char *buf = utf8.GetBuffer(cc);
        if (buf) WideCharToMultiByte(CP_UTF8, 0, uni, -1, buf, cc, 0, 0);
        utf8.ReleaseBuffer();
    }
    return utf8;
}

static LPCTSTR OperationToStr(int op)
{
    switch (op)
    {
    TOSTR(OP_ANALYZE_MBR);
    TOSTR(OP_BADBLOCKS);
    TOSTR(OP_ZERO_MBR);
    TOSTR(OP_PARTITION);
    TOSTR(OP_FORMAT);
    TOSTR(OP_CREATE_FS);
    TOSTR(OP_FIX_MBR);
    TOSTR(OP_DOS);
    TOSTR(OP_FINALIZE);
    TOSTR(OP_DOWNLOADING_FILES);  
    TOSTR(OP_VERIFYING_SIGNATURE);
    TOSTR(OP_FLASHING_DEVICE);
    TOSTR(OP_FILE_COPY);
	TOSTR(OP_SETUP_DUALBOOT);
    TOSTR(OP_NO_OPERATION_IN_PROGRESS);
    TOSTR(OP_ENDLESS_MAX);
    default: return _T("UNKNOWN_OPERATION");
    }
}

static LPCTSTR ErrorCauseToStr(ErrorCause_t errorCause)
{
    switch (errorCause)
    {
        TOSTR(ErrorCauseGeneric);
        TOSTR(ErrorCauseCanceled);
        TOSTR(ErrorCauseJSONDownloadFailed);
        TOSTR(ErrorCauseDownloadFailed);
        TOSTR(ErrorCauseDownloadFailedDiskFull);
        TOSTR(ErrorCauseVerificationFailed);
        TOSTR(ErrorCauseWriteFailed);
        TOSTR(ErrorCauseNot64Bit);
        TOSTR(ErrorCauseBitLocker);
        TOSTR(ErrorCauseNotNTFS);
        TOSTR(ErrorCauseNonWindowsMBR);
        TOSTR(ErrorCauseNone);
        default: return _T("Error Cause Unknown");
    }
}


extern "C" void UpdateProgress(int op, float percent)
{
    static int oldPercent = 0;
    static int oldOp = -1;
    bool change = false;

    if (op != oldOp) {
        oldOp = op;
        oldPercent = (int)floor(percent);
        change = true;
    } else if(oldPercent != (int)floor(percent)) {
        oldPercent = (int)floor(percent);
        change = true;
    }

    if(change) PostMessage(hMainDialog, WM_UPDATE_PROGRESS, (WPARAM)op, (LPARAM)oldPercent);
}

// CEndlessUsbToolDlg dialog

BEGIN_DHTML_EVENT_MAP(CEndlessUsbToolDlg)
	// For dragging the window
	DHTML_EVENT_CLASS(DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN, _T(CLASS_PAGE_HEADER_TITLE), OnHtmlMouseDown)
	DHTML_EVENT_CLASS(DISPID_HTMLELEMENTEVENTS_ONMOUSEDOWN, _T(CLASS_PAGE_HEADER), OnHtmlMouseDown)

	// Dual Boot Page handlers
	DHTML_EVENT_ONCHANGE(_T(ELEMENT_LANGUAGE_DUALBOOT), OnLanguageChanged)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_DUALBOOT_CLOSE_BUTTON), OnCloseAppClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_DUALBOOT_ADVANCED_LINK), OnAdvancedOptionsClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_DUALBOOT_INSTALL_BUTTON), OnInstallDualBootClicked)

	// First Page Handlers		
    DHTML_EVENT_ONCLICK(_T(ELEMENT_TRY_BUTTON), OnTryEndlessSelected)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_INSTALL_BUTTON), OnInstallEndlessSelected)
	DHTML_EVENT_ONCHANGE(_T(ELEMENT_LANGUAGE_SELECT), OnLanguageChanged)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_COMPARE_OPTIONS), OnLinkClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_FIRST_CLOSE_BUTTON), OnCloseAppClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_FIRST_PREV_BUTTON), OnFirstPagePreviousClicked)

	// Select File Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_PREV_BUTTON), OnSelectFilePreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_NEXT_BUTTON), OnSelectFileNextClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_BUTTON), OnSelectFileButtonClicked)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_FILES_SELECT), OnSelectedImageFileChanged)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_REMOTE_SELECT), OnSelectedRemoteFileChanged)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_IMAGE_TYPE_LOCAL), OnSelectedImageTypeChanged)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_IMAGE_TYPE_REMOTE), OnSelectedImageTypeChanged)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_DOWNLOAD_LIGHT_BUTTON), OnDownloadLightButtonClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_DOWNLOAD_FULL_BUTTON), OnDownloadFullButtonClicked)

    DHTML_EVENT_ONCLICK(_T(ELEMENT_CONNECTED_LINK), OnLinkClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_CONNECTED_SUPPORT_LINK), OnLinkClicked)

	// Select USB Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELUSB_PREV_BUTTON), OnSelectUSBPreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELUSB_NEXT_BUTTON), OnSelectUSBNextClicked)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_SELUSB_USB_DRIVES), OnSelectedUSBDiskChanged)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_SELUSB_AGREEMENT), OnAgreementCheckboxChanged)

	// Select Storage Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELSTORAGE_PREV_BUTTON), OnSelectStoragePreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELSTORAGE_NEXT_BUTTON), OnSelectStorageNextClicked)
	DHTML_EVENT_ONCHANGE(_T(ELEMENT_STORAGE_SELECT), OnSelectedStorageSizeChanged)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_STORAGE_SUPPORT_LINK), OnLinkClicked)

	// Installing Page handlers
    DHTML_EVENT_ONCLICK(_T(ELEMENT_INSTALL_CANCEL), OnInstallCancelClicked)

	// Thank You Page handlers
    DHTML_EVENT_ONCLICK(_T(ELEMENT_CLOSE_BUTTON), OnCloseAppClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_USBBOOT_HOWTO), OnLinkClicked)
    // Error Page handlers
    DHTML_EVENT_ONCLICK(_T(ELEMENT_ERROR_CLOSE_BUTTON), OnCloseAppClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_ENDLESS_SUPPORT), OnLinkClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_ERROR_BUTTON), OnRecoverErrorButtonClicked)
    DHTML_EVENT_ONCHANGE(_T(ELEMENT_ERROR_DELETE_CHECKBOX), OnDeleteCheckboxChanged)

END_DHTML_EVENT_MAP()

BEGIN_DISPATCH_MAP(CEndlessUsbToolDlg, CDHtmlDialog)
    DISP_FUNCTION(CEndlessUsbToolDlg, "Debug", JavascriptDebug, VT_EMPTY, VTS_BSTR)
END_DISPATCH_MAP()

CMap<CString, LPCTSTR, uint32_t, uint32_t> CEndlessUsbToolDlg::m_personalityToLocaleMsg;
CMap<CStringA, LPCSTR, CString, LPCTSTR> CEndlessUsbToolDlg::m_localeToPersonality;
CMap<CStringA, LPCSTR, CStringA, LPCSTR> CEndlessUsbToolDlg::m_localeToIniLocale;
CString CEndlessUsbToolDlg::m_appDir;

int CEndlessUsbToolDlg::ImageUnpackOperation;
int CEndlessUsbToolDlg::ImageUnpackPercentStart;
int CEndlessUsbToolDlg::ImageUnpackPercentEnd;
ULONGLONG CEndlessUsbToolDlg::ImageUnpackFileSize;

CEndlessUsbToolDlg::CEndlessUsbToolDlg(UINT globalMessage, bool enableLogDebugging, CWnd* pParent /*=NULL*/)
    : CDHtmlDialog(IDD_ENDLESSUSBTOOL_DIALOG, IDR_HTML_ENDLESSUSBTOOL_DIALOG, pParent),
    m_selectedLocale(NULL),
    m_liveInstall(false),
    m_localizationFile(""),
    m_shellNotificationsRegister(0),
    m_lastDevicesRefresh(0),
    m_spHtmlDoc3(NULL),
    m_lgpSet(FALSE),
    m_lgpExistingKey(FALSE),
    m_FilesChangedHandle(INVALID_HANDLE_VALUE),
    m_fileScanThread(INVALID_HANDLE_VALUE),
    m_operationThread(INVALID_HANDLE_VALUE),
    m_downloadUpdateThread(INVALID_HANDLE_VALUE),
    m_checkConnectionThread(INVALID_HANDLE_VALUE),
    m_spStatusElem(NULL),
    m_spWindowElem(NULL),
    m_dispexWindow(NULL),
    m_downloadManager(),
    m_useLocalFile(true),
    m_selectedRemoteIndex(-1),
    m_baseImageRemoteIndex(-1),
    m_usbDeleteAgreement(false),
    m_currentStep(OP_NO_OPERATION_IN_PROGRESS),
    m_cancelOperationEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
    m_closeFileScanThreadEvent(CreateEvent(NULL, TRUE, FALSE, NULL)),
    m_closeRequested(false),
    m_ieVersion(0),
    m_globalWndMessage(globalMessage),
    m_isConnected(false),
    m_enableLogDebugging(enableLogDebugging),
    m_lastErrorCause(ErrorCause_t::ErrorCauseNone),
    m_localFilesScanned(false),
    m_jsonDownloadAttempted(false),
	m_uninstallMode(false)
{
    GlobalLoggingMutex = CreateMutex(NULL, FALSE, NULL);

    FUNCTION_ENTER;
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);    

    size_t personalitiesCount = sizeof(globalAvailablePersonalities) / sizeof(globalAvailablePersonalities[0]);
    for (uint32_t index = 0; index < personalitiesCount; index++) {
        m_personalityToLocaleMsg.SetAt(globalAvailablePersonalities[index], MSG_400 + index);
    }

    m_localeToPersonality[RUFUS_LOCALE_EN] = PERSONALITY_ENGLISH;
    m_localeToPersonality[RUFUS_LOCALE_ES] = PERSONALITY_SPANISH;
    m_localeToPersonality[RUFUS_LOCALE_PT] = PERSONALITY_PORTUGUESE;
    m_localeToPersonality[RUFUS_LOCALE_SA] = PERSONALITY_ARABIC;
    m_localeToPersonality[RUFUS_LOCALE_FR] = PERSONALITY_FRENCH;
    m_localeToPersonality[RUFUS_LOCALE_ZH_CN] = PERSONALITY_CHINESE;
    m_localeToPersonality[RUFUS_LOCALE_BN] = PERSONALITY_BENGALI;

    m_localeToIniLocale[RUFUS_LOCALE_EN] = INI_LOCALE_EN;
    m_localeToIniLocale[RUFUS_LOCALE_ES] = INI_LOCALE_ES;
    m_localeToIniLocale[RUFUS_LOCALE_PT] = INI_LOCALE_PT;
    m_localeToIniLocale[RUFUS_LOCALE_SA] = INI_LOCALE_SA;
    m_localeToIniLocale[RUFUS_LOCALE_FR] = INI_LOCALE_FR;
    m_localeToIniLocale[RUFUS_LOCALE_ZH_CN] = INI_LOCALE_ZH;
    m_localeToIniLocale[RUFUS_LOCALE_BN] = INI_LOCALE_BN;
}

CEndlessUsbToolDlg::~CEndlessUsbToolDlg() {
    FUNCTION_ENTER;
    if (m_enableLogDebugging) {
        m_enableLogDebugging = false;
        m_logFile.Close();
    }

    if(GlobalLoggingMutex != NULL) CloseHandle(GlobalLoggingMutex);
}

void CEndlessUsbToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEndlessUsbToolDlg, CDHtmlDialog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// Browse navigation handling methods
void CEndlessUsbToolDlg::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
    FUNCTION_ENTER;

	CString systemDriveLetter = GetSystemDrive().Left(2);

	CDHtmlDialog::OnDocumentComplete(pDisp, szUrl);

	uprintf("OnDocumentComplete '%ls'", szUrl);

	IFTRUE_RETURN(m_uninstallMode, "Returning as we are in uninstall mode");

	if (this->m_spHtmlDoc == NULL) {
		uprintf("CEndlessUsbToolDlg::OnDocumentComplete m_spHtmlDoc==NULL");
		return;
	}

	if (m_spHtmlDoc3 == NULL) {
		HRESULT hr = m_spHtmlDoc->QueryInterface(IID_IHTMLDocument3, (void**)&m_spHtmlDoc3);
		IFFALSE_GOTOERROR(SUCCEEDED(hr) && m_spHtmlDoc3 != NULL, "Error when querying IID_IHTMLDocument3 interface.");
	}

	AddLanguagesToUI();
	ApplyRufusLocalization(); //apply_localization(IDD_ENDLESSUSBTOOL_DIALOG, GetSafeHwnd());

    if (nWindowsVersion == WINDOWS_XP) {
        CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_INSTALL_BUTTON))), CComVariant(FALSE));
    }

    SetElementText(_T(ELEMENT_VERSION_CONTAINER), CComBSTR(RELEASE_VER_STR));

    StartCheckInternetConnectionThread();
    FindMaxUSBSpeed();

	// TODO: Leave the "Dual boot" button enabled, on click check the below and move to proper error page
	BOOL x64BitSupported = Has64BitSupport() ? TRUE : FALSE;
	uprintf("HW processor has 64 bit support: %s", x64BitSupported ? "YES" : "NO");
	if (!x64BitSupported)
		Analytics::instance()->exceptionTracking(ErrorCauseToStr(ErrorCauseNot64Bit), FALSE);

	BOOL isBitLockerEnabled = IsBitlockedDrive(systemDriveLetter);
	uprintf("Is bitlocker enabled on '%ls': %s", systemDriveLetter, isBitLockerEnabled ? "YES" : "NO");
	if (isBitLockerEnabled)
		Analytics::instance()->exceptionTracking(ErrorCauseToStr(ErrorCauseBitLocker), FALSE);

	CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_DUALBOOT_INSTALL_BUTTON))), CComVariant(x64BitSupported && !isBitLockerEnabled));

	return;
error:
	uprintf("OnDocumentComplete Exit with error");
}

void CEndlessUsbToolDlg::InitRufus()
{
    FUNCTION_ENTER;

    // RADU: try to remove the need for this
    hMainDialog = m_hWnd;
    hDeviceList = m_hWnd;
    hBootType = m_hWnd;
    hPartitionScheme = m_hWnd;
    hFileSystem = m_hWnd;
    hClusterSize = m_hWnd;
    hLabel = m_hWnd;
    hNBPasses = m_hWnd;
    hDiskID = m_hWnd;
    hInfo = m_hWnd;

    hMainInstance = AfxGetResourceHandle();

    if (GetSystemDirectoryU(system_dir, sizeof(system_dir)) == 0) {
        uprintf("Could not get system directory: %s", WindowsErrorString());
        safe_strcpy(system_dir, sizeof(system_dir), "C:\\Windows\\System32");
    }
    // Construct Sysnative ourselves as there is no GetSysnativeDirectory() call
    // By default (64bit app running on 64 bit OS or 32 bit app running on 32 bit OS)
    // Sysnative and System32 are the same
    safe_strcpy(sysnative_dir, sizeof(sysnative_dir), system_dir);
    // But if the app is 32 bit and the OS is 64 bit, Sysnative must differ from System32
#if (!defined(_WIN64) && !defined(BUILD64))
    if (is_x64()) {
        if (GetSystemWindowsDirectoryU(sysnative_dir, sizeof(sysnative_dir)) == 0) {
            uprintf("Could not get Windows directory: %s", WindowsErrorString());
            safe_strcpy(sysnative_dir, sizeof(sysnative_dir), "C:\\Windows");
        }
        safe_strcat(sysnative_dir, sizeof(sysnative_dir), "\\Sysnative");
    }
#endif

    // Initialize COM for folder selection
    IGNORE_RETVAL(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

    //// Some dialogs have Rich Edit controls and won't display without this
    //if (GetLibraryHandle("Riched20") == NULL) {
    //    uprintf("Could not load RichEdit library - some dialogs may not display: %s\n", WindowsErrorString());
    //}

    PF_INIT(GetTickCount64, kernel32);    

    srand((unsigned int)_GetTickCount64());    
}

void CEndlessUsbToolDlg::ChangeDriveAutoRunAndMount(bool setEndlessValues)
{
	if (setEndlessValues) {
		// We use local group policies rather than direct registry manipulation
		// 0x9e disables removable and fixed drive notifications
		m_lgpSet = SetLGP(FALSE, &m_lgpExistingKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "NoDriveTypeAutorun", 0x9e);

		if (nWindowsVersion > WINDOWS_XP) {
			// Re-enable AutoMount if needed
			if (!GetAutoMount(&m_automount)) {
				uprintf("Could not get AutoMount status");
				m_automount = TRUE;	// So that we don't try to change its status on exit
			}
			else if (!m_automount) {
				uprintf("AutoMount was detected as disabled - temporary re-enabling it");
				if (!SetAutoMount(TRUE)) {
					uprintf("Failed to enable AutoMount");
				}
			}
		}
	} else {
		// revert settings we changed
		if (m_lgpSet) {
			SetLGP(TRUE, &m_lgpExistingKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", "NoDriveTypeAutorun", 0);
		}
		if ((nWindowsVersion > WINDOWS_XP) && (!m_automount) && (!SetAutoMount(FALSE))) {
			uprintf("Failed to restore AutoMount to disabled");
		}
	}
}

// The scanning process can be blocking for message processing => use a thread
DWORD WINAPI CEndlessUsbToolDlg::RufusISOScanThread(LPVOID param)
{
    FUNCTION_ENTER;

    if (image_path == NULL) {
        uprintf("ERROR: image_path is NULL");
        goto out;
    }

    PrintInfoDebug(0, MSG_202);

    img_report.is_iso = (BOOLEAN)ExtractISO(image_path, "", TRUE);
    img_report.is_bootable_img = (BOOLEAN)IsBootableImage(image_path);
    if (!img_report.is_iso && !img_report.is_bootable_img) {
        // Failed to scan image
        PrintInfoDebug(0, MSG_203);
        safe_free(image_path);
        PrintStatus(0, MSG_086);
        goto out;
    }

    if (img_report.is_bootable_img) {
        uprintf("  Image is a %sbootable %s image",
            (img_report.compression_type != BLED_COMPRESSION_NONE) ? "compressed " : "", img_report.is_vhd ? "VHD" : "disk");
    }

out:
    ::SendMessage(hMainDialog, WM_FINISHED_IMG_SCANNING, 0, 0);

    PrintInfo(0, MSG_210);
    ExitThread(0);
}


// CEndlessUsbToolDlg message handlers

BOOL CEndlessUsbToolDlg::OnInitDialog()
{
    FUNCTION_ENTER;

	CDHtmlDialog::OnInitDialog();

    InitLogging();

    // Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//refuse dragging file into this dialog
	m_pBrowserApp->put_RegisterAsDropTarget(VARIANT_FALSE);
	m_pBrowserApp->put_RegisterAsBrowser(FALSE);
    SetHostFlags(DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_NO3DBORDER);
    // Expose logging method to Javascript
    EnableAutomation();
    LPDISPATCH pDisp = GetIDispatch(FALSE);
    SetExternalDispatch(pDisp);
    //Disable JavaScript errors
    m_pBrowserApp->put_Silent(VARIANT_TRUE);

	//// Remove caption and border
	SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE)
		& (~(WS_CAPTION | WS_BORDER)));

    // Move window
    SetWindowPos(NULL, 0, 0, 748, 514, SWP_NOMOVE | SWP_NOZORDER);

    GetIEVersion();

    // Make round corners
    if(m_ieVersion >= 9) { // Internet explorer < 9 doesn't support rounded corners
        //  Get the rectangle
        CRect rect;
        GetWindowRect(&rect);
        int w = rect.Width();
        int h = rect.Height();

        int radius = 11;
        CRgn rgnRound, rgnRect, rgnComp;
        rgnRound.CreateRoundRectRgn(0, 0, w + 1, h + radius, radius, radius);
        rgnRect.CreateRectRgn(0, 0, w, h);
        rgnComp.CreateRectRgn(0, 0, w, h);
        int combineResult = rgnComp.CombineRgn(&rgnRect, &rgnRound, RGN_AND);
        //  Set the window region
        SetWindowRgn(static_cast<HRGN>(rgnComp.GetSafeHandle()), TRUE);
    }

	// Init localization before doing anything else
	LoadLocalizationData();

	InitRufus();

    // For Rufus
    CheckDlgButton(IDC_BOOT, BST_CHECKED);
    CheckDlgButton(IDC_QUICK_FORMAT, BST_CHECKED);

    bool result = m_downloadManager.Init(m_hWnd, WM_FILE_DOWNLOAD_STATUS);

    SetWindowTextW(L"");

	CStringW exePath = GetExePath();
	if (CSTRING_GET_LAST(exePath,'\\') == ENDLESS_UNINSTALLER_NAME) {
	//if (CSTRING_GET_LAST(exePath, '\\') == L"EndlessUsbTool.exe") { // for debugging
		m_uninstallMode = true;
		int selected = AfxMessageBox(UTF8ToCString(lmprintf(MSG_361)), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
		if (selected == IDYES) {
			ShowWindow(SW_HIDE);
			UninstallDualBoot();
		} else {
			ExitProcess(0);
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

#define THREADS_WAIT_TIMEOUT 10000 // 10 seconds
void CEndlessUsbToolDlg::Uninit()
{
    FUNCTION_ENTER;

    int handlesCount = 0;
    HANDLE handlesToWaitFor[4];
    
    if (m_fileScanThread != INVALID_HANDLE_VALUE) handlesToWaitFor[handlesCount++] = m_fileScanThread;
    if (m_operationThread != INVALID_HANDLE_VALUE) handlesToWaitFor[handlesCount++] = m_operationThread;
    if (m_downloadUpdateThread != INVALID_HANDLE_VALUE) handlesToWaitFor[handlesCount++] = m_downloadUpdateThread;
    if (m_checkConnectionThread != INVALID_HANDLE_VALUE) handlesToWaitFor[handlesCount++] = m_checkConnectionThread;

    if (handlesCount > 0) {
        if (m_closeFileScanThreadEvent != INVALID_HANDLE_VALUE) SetEvent(m_closeFileScanThreadEvent);
        if (m_cancelOperationEvent != INVALID_HANDLE_VALUE) SetEvent(m_cancelOperationEvent);
        DWORD waitStatus = WaitForMultipleObjects(handlesCount, handlesToWaitFor, TRUE, THREADS_WAIT_TIMEOUT);

        if (waitStatus == WAIT_TIMEOUT) {
            uprintf("Error: waited for %d millis for threads to finish.", THREADS_WAIT_TIMEOUT);
        }
    }

    if (m_cancelOperationEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_cancelOperationEvent);
        m_cancelOperationEvent = INVALID_HANDLE_VALUE;
    }

    if (m_closeFileScanThreadEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_closeFileScanThreadEvent);
        m_closeFileScanThreadEvent = INVALID_HANDLE_VALUE;
    }

    m_downloadManager.Uninit();

    // unregister from notifications and delete drive list related memory
    LeavingDevicesPage();

    StrArrayDestroy(&DriveID);
    StrArrayDestroy(&DriveLabel);

    // delete image file entries related memory
    CString currentPath;
    pFileImageEntry_t currentEntry = NULL;
    for (POSITION position = m_imageFiles.GetStartPosition(); position != NULL; ) {
        m_imageFiles.GetNextAssoc(position, currentPath, currentEntry);
        delete currentEntry;
    }
    m_imageFiles.RemoveAll();

    // uninit localization memory
    exit_localization();

    safe_free(image_path);

    CLOSE_OPENED_LIBRARIES;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEndlessUsbToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDHtmlDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEndlessUsbToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CEndlessUsbToolDlg::PreTranslateMessage(MSG* pMsg)
{
    // disable any key
    if (pMsg->message == WM_KEYDOWN) {
        return TRUE;
    } else if(pMsg->message == WM_MOUSEWHEEL && (GetKeyState(VK_CONTROL) & 0x8000)) { // Scroll With Ctrl + Mouse Wheel
        return TRUE;
    }

    return CDHtmlDialog::PreTranslateMessage(pMsg);
}

void CEndlessUsbToolDlg::JavascriptDebug(LPCTSTR debugMsg)
{
    CComBSTR msg = debugMsg;
    uprintf("Javascript - [%ls]", msg);
}

/*
* Device Refresh Timer
*/
void CALLBACK CEndlessUsbToolDlg::RefreshTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    switch (idEvent) {
    case TID_REFRESH_TIMER:
        // DO NOT USE WM_DEVICECHANGE - IT MAY BE FILTERED OUT BY WINDOWS!
        ::PostMessage(hWnd, UM_MEDIA_CHANGE, 0, 0);
        break;
    case TID_UPDATE_FILES:
        ::PostMessage(hWnd, WM_FILES_CHANGED, 0, 0);
        break;
    default:
        uprintf("Timer not handled [%d]", idEvent);
        break;
    }
}

LRESULT CEndlessUsbToolDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message >= CB_GETEDITSEL && message < CB_MSGMAX) {
        CComPtr<IHTMLSelectElement> selectElement;
        CComPtr<IHTMLOptionElement> optionElement;
        HRESULT hr;

        hr = GetSelectElement(_T(ELEMENT_SELUSB_USB_DRIVES), selectElement);
        IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error returned from GetSelectElement.");

        switch (message) {
        case CB_RESETCONTENT:
        {
            hr = selectElement->put_length(0);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error clearing elements from USB drive list.");
            break;
        }

        case CB_ADDSTRING:
        {
            CComBSTR text = (wchar_t*)lParam;
            long index = 0;
            hr = AddEntryToSelect(selectElement, text, text, &index);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error adding item in USB drive list.");
            return index;
        }

        case CB_SETCURSEL:
        case CB_SETITEMDATA:
        case CB_GETITEMDATA:
        {
            CComPtr<IDispatch> pDispatch;
            CComBSTR valueBSTR;
            long index = (long)wParam;
            long value = (long)lParam;

            hr = selectElement->item(CComVariant(index), CComVariant(0), &pDispatch);
            IFFALSE_GOTOERROR(SUCCEEDED(hr) && pDispatch != NULL, "Error when querying for element at requested index.");

            hr = pDispatch.QueryInterface<IHTMLOptionElement>(&optionElement);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error when querying for IHTMLOptionElement interface");

            if (message == CB_SETITEMDATA) {
                hr = ::VarBstrFromI4(value, LOCALE_SYSTEM_DEFAULT, 0, &valueBSTR);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error converting int value to BSTR.");

                hr = optionElement->put_value(valueBSTR);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error setting value for requested element.");
            }
            else if (message == CB_GETITEMDATA) {
                hr = optionElement->get_value(&valueBSTR);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error getting value for requested element.");

                hr = ::VarI4FromStr(valueBSTR, LOCALE_SYSTEM_DEFAULT, 0, &value);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error converting BSTR to int value.");
                return value;
            }
            else if (message == CB_SETCURSEL) {
                hr = optionElement->put_selected(VARIANT_TRUE);
                IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error setting selected for requested element.");
            }

            break;
        }

        case CB_GETCURSEL:
        {
            long index = -1;
            hr = selectElement->get_selectedIndex(&index);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for selected index.");

            return index;
        }

        case CB_GETCOUNT:
        {
            long count = 0;
            hr = selectElement->get_length(&count);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for list count.");

            return count;
        }

        // Ignore
        case CB_SETDROPPEDWIDTH:
            break;

        default:
            luprintf("Untreated CB message %d(0x%X)", message, message);
            break;
        }

        return 0;

    error:
        uprintf("CEndlessUsbToolDlg::WindowProc called with %d[0x%X] (%d, %d); FAILURE %x", message, message, lParam, wParam, hr);
        return -1;
    } else if (m_enableLogDebugging && message >= EM_GETSEL && message < EM_GETIMESTATUS) {
        // Handle messages sent to the log window
        // Do not use any method that calls _uprintf as it will generate an infinite recursive loop
        // Use OutputDebugString instead.
        switch (message) {
            case EM_SETSEL:
            {
                static CStringA strMessage;
                char *logMessage = (char*)lParam;
                CStringA time(CTime::GetCurrentTime().Format(_T("%H:%M:%S - ")));
                strMessage = time + CStringA(logMessage);
                m_logFile.Write(strMessage, strMessage.GetLength());
                free(logMessage);
                break;
            }
        }
    } else {
        switch (message) {
        case WM_SETTEXT:
        {
            lParam = (LPARAM)mainWindowTitle;            
            break;
        }
        case WM_CLIENTSHUTDOWN:
        case WM_QUERYENDSESSION:
        case WM_ENDSESSION:
            // TODO: Do we want to use ShutdownBlockReasonCreate() in Vista and later to stop
            // forced shutdown? See https://msdn.microsoft.com/en-us/library/ms700677.aspx
            if (m_operationThread != INVALID_HANDLE_VALUE) {
                // WM_QUERYENDSESSION uses this value to prevent shutdown
                return (INT_PTR)TRUE;
            }
            PostMessage(WM_COMMAND, (WPARAM)IDCANCEL, (LPARAM)0);
            break;

        case UM_MEDIA_CHANGE:
            wParam = DBT_CUSTOMEVENT;
            // Fall through
        case WM_DEVICECHANGE:
            // The Windows hotplug subsystem sucks. Among other things, if you insert a GPT partitioned
            // USB drive with zero partitions, the only device messages you will get are a stream of
            // DBT_DEVNODES_CHANGED and that's it. But those messages are also issued when you get a
            // DBT_DEVICEARRIVAL and DBT_DEVICEREMOVECOMPLETE, and there's a whole slew of them so we
            // can't really issue a refresh for each one we receive
            // What we do then is arm a timer on DBT_DEVNODES_CHANGED, if it's been more than 1 second
            // since last refresh/arm timer, and have that timer send DBT_CUSTOMEVENT when it expires.
            // DO *NOT* USE WM_DEVICECHANGE AS THE MESSAGE FROM THE TIMER PROC, as it may be filtered!
            // For instance filtering will occur when (un)plugging in a FreeBSD UFD on Windows 8.
            // Instead, use a custom user message, such as UM_MEDIA_CHANGE, to set DBT_CUSTOMEVENT.
            
            // RADU: check to see if this happens durring download/verification phases and the selected disk dissapeared
            // we can still ask the user to select another disk after download/verification has finished
            if (m_operationThread == INVALID_HANDLE_VALUE) { 
                switch (wParam) {
                case DBT_DEVICEARRIVAL:
                case DBT_DEVICEREMOVECOMPLETE:
                case DBT_CUSTOMEVENT:	// Sent by our timer refresh function or for card reader media change
                    m_lastDevicesRefresh = _GetTickCount64();
                    KillTimer(TID_REFRESH_TIMER);
                    GetUSBDevices((DWORD)ComboBox_GetItemData(hDeviceList, ComboBox_GetCurSel(hDeviceList)));
                    OnSelectedUSBDiskChanged(NULL);
                    return (INT_PTR)TRUE;
                case DBT_DEVNODES_CHANGED:
                    // If it's been more than a second since last device refresh, arm a refresh timer
                    if (_GetTickCount64() > m_lastDevicesRefresh + 1000) {
                        m_lastDevicesRefresh = _GetTickCount64();
                        SetTimer(TID_REFRESH_TIMER, 1000, RefreshTimer);
                    }
                    break;
                default:
                    break;
                }
            }
            break;
        case UM_PROGRESS_INIT:
        {
            uprintf("Started to scan the provided image.");
            break;
        }
        case WM_FINISHED_IMG_SCANNING:
        {
            uprintf("Image scanning done.");
            m_operationThread = INVALID_HANDLE_VALUE;
            if (!img_report.is_bootable_img ||
                (img_report.compression_type != BLED_COMPRESSION_GZIP && img_report.compression_type != BLED_COMPRESSION_XZ)) {
                uprintf("FAILURE: selected image is not bootable and compresion is different frome what is expected: xz/gz");
                ErrorOccured(ErrorCause_t::ErrorCauseVerificationFailed);
            } else {
                uprintf("Bootable image selected with correct format.");

                // Start formatting
                FormatStatus = 0;

                int nDeviceIndex = ComboBox_GetCurSel(hDeviceList);
                if (nDeviceIndex != CB_ERR) {
                    DWORD DeviceNum = (DWORD)ComboBox_GetItemData(hDeviceList, nDeviceIndex);
                    StartOperationThread(OP_FLASHING_DEVICE, FormatThread, (LPVOID)(uintptr_t)DeviceNum);                    
                }
            }

            break;
        }

        case WM_UPDATE_PROGRESS:
        {
            HRESULT hr;
            int op = (int)wParam;
            int percent = (int)lParam;
            uprintf("Operation %ls(%d) with progress %d", OperationToStr(op), op, percent);
            
            // Ignore exFAT format progress.
            if (m_currentStep == OP_FILE_COPY && (op == OP_FORMAT || op == OP_CREATE_FS)) break;

            // Radu: pass all the files to be verified to verfication thread and do the percent calculation there
            // Not very happy about this but eh, needs refactoring
            if (op == OP_VERIFYING_SIGNATURE) {
				if (m_dualBootSelected) {
					// Radu: do we need to do anything special here?
					// Does it make sense to add the boot archive signature verification to the percentage calculation also?
					// We are talking about 6 MB compared to more than 2 GB
				} else {
					if (!m_liveInstall) {
						ULONGLONG totalSize = m_selectedFileSize + m_localInstallerImage.size;
						ULONGLONG currentSize = 0;
						bool isInstallerImage = (m_localFile == m_localInstallerImage.filePath);
						if (isInstallerImage) {
							currentSize = m_selectedFileSize + (m_localInstallerImage.size * percent / 100);
						}
						else {
							currentSize = m_selectedFileSize * percent / 100;
						}
						percent = (int)(currentSize * 100 / totalSize);
					}
				}
            }
            
            // Radu: maybe divide the progress bar also based on the size of the image to be copied to disk after format is complete
            if (op == OP_FORMAT && !m_liveInstall) {
                percent = percent / 2;
            } else if (op == OP_FILE_COPY) {
                percent = 50 + percent / 2;
            }

            if (percent >= 0 && percent <= 100) {
                hr = CallJavascript(_T(JS_SET_PROGRESS), CComVariant(percent));
                IFFALSE_BREAK(SUCCEEDED(hr), "Error when calling set progress.");
            }

            if (op == OP_VERIFYING_SIGNATURE || op == OP_FORMAT || op == OP_FILE_COPY || op == OP_SETUP_DUALBOOT) {
                CString downloadString;
                downloadString.Format(L"%d%%", percent);
                SetElementText(_T(ELEMENT_INSTALL_STATUS), CComBSTR(downloadString));
            }

            break;
        }
        case UM_PROGRESS_EXIT:
        case UM_NO_UPDATE:
            luprintf("Untreated Rufus UM message %d(0x%X)", message, message);
            break;
        case UM_FORMAT_COMPLETED:
        {
            m_operationThread = INVALID_HANDLE_VALUE;
            if (!IS_ERROR(FormatStatus) && !m_liveInstall) {
                StartOperationThread(OP_FILE_COPY, CEndlessUsbToolDlg::FileCopyThread);
            } else {
                if (IS_ERROR(FormatStatus) && m_lastErrorCause == ErrorCause_t::ErrorCauseNone) {
                    m_lastErrorCause = ErrorCause_t::ErrorCauseWriteFailed;
                }
                PostMessage(WM_FINISHED_ALL_OPERATIONS, 0, 0);
            }
            break;
        }
        case WM_FILES_CHANGED:
        {
            UpdateFileEntries();
            break;
        }

        case WM_FILE_DOWNLOAD_STATUS:
        {
            DownloadStatus_t *downloadStatus = (DownloadStatus_t *)wParam;
            IFFALSE_BREAK(downloadStatus != NULL, "downloadStatus is NULL");

            bool isReleaseJsonDownload = downloadStatus->jobName == DownloadManager::GetJobName(DownloadType_t::DownloadTypeReleseJson);
            // DO STUFF
            if (downloadStatus->error) {
                uprintf("Error on download.");
                if (isReleaseJsonDownload) {
                    JSONDownloadFailed();
                } else {
                    bool diskFullError = (downloadStatus->errorContext == BG_ERROR_CONTEXT_LOCAL_FILE);
                    diskFullError = diskFullError && (downloadStatus->errorCode == HRESULT_FROM_WIN32(ERROR_DISK_FULL));
                    m_lastErrorCause = diskFullError ? ErrorCause_t::ErrorCauseDownloadFailedDiskFull : ErrorCause_t::ErrorCauseDownloadFailed;
                    m_downloadManager.ClearExtraDownloadJobs();
                    ErrorOccured(m_lastErrorCause);
                }
            } else if (downloadStatus->done) {
                uprintf("Download done for %ls", downloadStatus->jobName);

                if (isReleaseJsonDownload) {
                    if (!m_jsonDownloadAttempted) {
                        m_jsonDownloadAttempted = true;
                        UpdateDownloadOptions();
                        AddDownloadOptionsToUI();
                    }
                } else {
                    StartOperationThread(OP_VERIFYING_SIGNATURE, CEndlessUsbToolDlg::FileVerificationThread);
                }
            } else {
				CStringA part, total;
				part = SizeToHumanReadable(downloadStatus->progress.BytesTransferred, FALSE, use_fake_units);
				total = SizeToHumanReadable(downloadStatus->progress.BytesTotal, FALSE, use_fake_units);
                uprintf("Download [%ls] progress %s of %s (%d of %d files)", downloadStatus->jobName,
                    part, total,
                    downloadStatus->progress.FilesTransferred, downloadStatus->progress.FilesTotal);

                if (!isReleaseJsonDownload) {
                    static ULONGLONG startedTickCount = 0;
                    static ULONGLONG startedBytes = 0;

                    RemoteImageEntry_t remote = m_remoteImages.GetAt(m_remoteImages.FindIndex(m_selectedRemoteIndex));
                    // we don't take the signature files into account but we are taking about ~2KB compared to >2GB
                    ULONGLONG totalSize = remote.compressedSize + (m_liveInstall || m_dualBootSelected ? 0 : m_installerImage.compressedSize);
                    ULONGLONG percent = downloadStatus->progress.BytesTransferred * 100 / totalSize;
                    PostMessage(WM_UPDATE_PROGRESS, (WPARAM)OP_DOWNLOADING_FILES, (LPARAM)percent);

                    // calculate speed
                    CStringA speed("---");
                    ULONGLONG currentTickCount = _GetTickCount64();
                    if (startedTickCount != 0) {
                        ULONGLONG diffBytes = downloadStatus->progress.BytesTransferred - startedBytes;
                        ULONGLONG diffMillis = currentTickCount - startedTickCount;
                        double diffSeconds = diffMillis / 1000.0;
                        speed = SizeToHumanReadable((ULONGLONG) (diffBytes / diffSeconds), FALSE, use_fake_units);
                    } else {
                        startedTickCount = currentTickCount;
                        startedBytes = downloadStatus->progress.BytesTransferred;
                    }

                    CStringA strDownloaded = SizeToHumanReadable(downloadStatus->progress.BytesTransferred, FALSE, use_fake_units);
                    CStringA strTotal = SizeToHumanReadable(totalSize, FALSE, use_fake_units);
                    // push to UI
                    CString downloadString = UTF8ToCString(lmprintf(MSG_302, strDownloaded, strTotal, speed));
                    SetElementText(_T(ELEMENT_INSTALL_STATUS), CComBSTR(downloadString));
                }
            }

            delete downloadStatus;
            break;
        }
        case WM_FINISHED_FILE_VERIFICATION:
        {
            BOOL result = (BOOL)wParam;
            m_operationThread = INVALID_HANDLE_VALUE;
            if (result) {
                uprintf("Verification passed.");

                bool verifiedInstallerImage = (m_localFile == m_localInstallerImage.filePath);
				bool verifiedBootFilesZip = (m_localFile == m_bootArchive);

				if (m_dualBootSelected) {
					if (verifiedBootFilesZip) {
						// we checked the boot archive signature, now check the image
						m_localFile = UTF8ToCString(image_path);
						m_localFileSig = m_localFile + SIGNATURE_FILE_EXT;
						StartOperationThread(OP_VERIFYING_SIGNATURE, CEndlessUsbToolDlg::FileVerificationThread);
					} else {
						m_cancelImageUnpack = 0;
						//StartOperationThread(OP_FLASHING_DEVICE, CEndlessUsbToolDlg::CreateUSBStick);
						StartOperationThread(OP_SETUP_DUALBOOT, CEndlessUsbToolDlg::SetupDualBoot);
					}
				} else if (!m_liveInstall && !verifiedInstallerImage) {
                    safe_free(image_path);
                    image_path = wchar_to_utf8(m_localInstallerImage.filePath);

                    // Radu: please make time to refactor this.
                    // store this to copy them
                    m_LiveFile = m_localFile;
                    m_LiveFileSig = m_localFileSig;

                    m_localFile = UTF8ToCString(image_path);
                    m_localFileSig = m_localFile + SIGNATURE_FILE_EXT;
                    StartOperationThread(OP_VERIFYING_SIGNATURE, CEndlessUsbToolDlg::FileVerificationThread);
                } else {
                    StartOperationThread(OP_FLASHING_DEVICE, CEndlessUsbToolDlg::RufusISOScanThread);
                }
            }
            else {
                uprintf("Signature verification failed.");
                m_currentStep = OP_NO_OPERATION_IN_PROGRESS;
                if (m_lastErrorCause == ErrorCause_t::ErrorCauseNone) {
                    m_lastErrorCause = ErrorCause_t::ErrorCauseVerificationFailed;
                    ErrorOccured(m_lastErrorCause);
                } else {
                    ErrorOccured(m_lastErrorCause);
                }
            }
            break;
        }
        case WM_FINISHED_FILE_COPY:
        {
            PostMessage(WM_FINISHED_ALL_OPERATIONS, 0, 0);
            break;
        }

        case WM_FINISHED_ALL_OPERATIONS:
        {
            m_operationThread = INVALID_HANDLE_VALUE;
            m_currentStep = OP_NO_OPERATION_IN_PROGRESS;

            EnableHibernate();

			if (!m_dualBootSelected) ChangeDriveAutoRunAndMount(false);

            switch (m_lastErrorCause) {
            case ErrorCause_t::ErrorCauseNone:
                PrintInfo(0, MSG_210);
                m_operationThread = INVALID_HANDLE_VALUE;
                ChangePage(_T(ELEMENT_SUCCESS_PAGE));
                break;
            default:
                ErrorOccured(m_lastErrorCause);
                break;
            }
            FormatStatus = 0;
            break;
        }
        case WM_INTERNET_CONNECTION_STATE:
        {
            bool connected = ((BOOL)wParam) == TRUE;
            m_isConnected = connected;

            if (m_isConnected) {
                StartJSONDownload();
            } else if(m_currentStep == OP_DOWNLOADING_FILES) {
                m_lastErrorCause = ErrorCause_t::ErrorCauseDownloadFailed;
                CancelRunningOperation();
            }

            CallJavascript(_T(JS_ENABLE_DOWNLOAD), CComVariant(m_remoteImages.GetCount() != 0), CComVariant(connected));
            CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_DOWNLOAD_LIGHT_BUTTON))), CComVariant(connected && (m_baseImageRemoteIndex != -1)));

            break;
        }

        case WM_POWERBROADCAST:
        {
            uprintf("Received WM_POWERBROADCAST with WPARAM 0x%X LPARAM 0x%X", wParam, lParam);
            bool shouldStopSuspend = m_currentStep != OP_NO_OPERATION_IN_PROGRESS;
            if (shouldStopSuspend && lParam & PBT_APMQUERYSUSPEND) {
                uprintf("Received WM_POWERBROADCAST with PBT_APMQUERYSUSPEND and trying to cancel it.");
                return BROADCAST_QUERY_DENY;
            } else {
                if (wParam == PBT_APMSUSPEND) {
                    uprintf("Received PBT_APMSUSPEND so canceling the operation.");
                    m_lastErrorCause = ErrorCause_t::ErrorCauseDownloadFailed;
                    CancelRunningOperation();
                }

                return TRUE;
            }
            break;
        }

        default:
            if (m_globalWndMessage == message) {
                SetForegroundWindow();
            }

            //luprintf("Untreated message %d(0x%X)", message, message);
            break;
        }
    }

    return CDHtmlDialog::WindowProc(message, wParam, lParam);
}

// Disable context menu
STDMETHODIMP CEndlessUsbToolDlg::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    FUNCTION_ENTER;
	return S_OK;
}

// prevent refresh
STDMETHODIMP CEndlessUsbToolDlg::TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID)
{
	if (lpMsg && lpMsg->message == WM_KEYDOWN &&
		(lpMsg->wParam == VK_F5 ||
			lpMsg->wParam == VK_CONTROL))
	{
		return S_OK;
	}
	return CDHtmlDialog::TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID);
}

// Drag window
HRESULT CEndlessUsbToolDlg::OnHtmlMouseDown(IHTMLElement* pElement)
{
	POINT pt;
	GetCursorPos(&pt);
	SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));

	return S_OK;
}

// Private methods
void CEndlessUsbToolDlg::LoadLocalizationData()
{
    FUNCTION_ENTER;

	const char* rufus_loc = "endless.loc";
    int lcid = GetUserDefaultUILanguage();
	BYTE *loc_data;
	DWORD loc_size, size;
	char tmp_path[MAX_PATH] = "", loc_file[MAX_PATH] = "";
	HANDLE hFile = NULL;

	init_localization();

	loc_data = (BYTE*)GetResource(hMainInstance, MAKEINTRESOURCEA(IDR_LC_ENDLESS_LOC), _RT_RCDATA, "embedded.loc", &loc_size, FALSE);
	if ((GetTempPathU(sizeof(tmp_path), tmp_path) == 0)
		|| (GetTempFileNameU(tmp_path, APPLICATION_NAME, 0, m_localizationFile) == 0)
		|| (m_localizationFile[0] == 0)) {
		// Last ditch effort to get a loc file - just extract it to the current directory
		safe_strcpy(m_localizationFile, sizeof(m_localizationFile), rufus_loc);
	}

	hFile = CreateFileU(m_localizationFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ((hFile == INVALID_HANDLE_VALUE) || (!WriteFileWithRetry(hFile, loc_data, loc_size, &size, WRITE_RETRIES))) {
		uprintf("localization: unable to extract '%s': %s", m_localizationFile, WindowsErrorString());
		safe_closehandle(hFile);
		goto error;
	}
	uprintf("localization: extracted data to '%s'", m_localizationFile);
	safe_closehandle(hFile);

	if ((!get_supported_locales(m_localizationFile))
		|| ((m_selectedLocale = get_locale_from_lcid(lcid, TRUE)) == NULL)) {
		uprintf("FATAL: Could not access locale!");
		goto error;
	}
	selected_langid = get_language_id(m_selectedLocale);

	if (get_loc_data_file(m_localizationFile, m_selectedLocale))
		uprintf("Save locale to settings?");

	return;

error:
	uprintf("Exiting with error");
	MessageBoxU(NULL, "The locale data is missing or invalid. This application will now exit.", "Fatal error", MB_ICONSTOP | MB_SYSTEMMODAL);

}

void CEndlessUsbToolDlg::ApplyRufusLocalization()
{
    FUNCTION_ENTER;

	loc_cmd* lcmd = NULL;
	int dlg_id = IDD_ENDLESSUSBTOOL_DIALOG;
	HWND hCtrl = NULL;
	HWND hDlg = GetSafeHwnd();

	if (this->m_spHtmlDoc == NULL) {
		uprintf("CEndlessUsbToolDlg::ApplyRufusLocalization m_spHtmlDoc==NULL");
		return;
	}

	HRESULT hr;
	CComPtr<IHTMLElement> pElement = NULL;

	// go through the ids and update the text
	list_for_each_entry(lcmd, &loc_dlg[dlg_id - IDD_DIALOG].list, loc_cmd, list) {
		if (lcmd->command <= LC_TEXT) {
			if (lcmd->ctrl_id == dlg_id) {
				luprint("Updating dialog title not implemented");
			} else {
				pElement = NULL;
				hr = m_spHtmlDoc3->getElementById(CComBSTR(lcmd->txt[0]), &pElement);
				if (FAILED(hr) || pElement == NULL) {
					luprintf("control '%s' is not part of dialog '%s'\n", lcmd->txt[0], control_id[dlg_id - IDD_DIALOG].name);
				}
			}
		}

		switch (lcmd->command) {
		case LC_TEXT:
			if (pElement != NULL) {
				if ((lcmd->txt[1] != NULL) && (lcmd->txt[1][0] != 0)) {
					hr = pElement->put_innerHTML(UTF8ToBSTR(lcmd->txt[1]));
					if (FAILED(hr)) {
						luprintf("error when updating control '%s', hr='%x'\n", lcmd->txt[0], hr);
					}
				}
			}
			break;
		case LC_MOVE:
			luprint("LC_MOVE not implemented");
			break;
		case LC_SIZE:
			luprint("LC_SIZE not implemented");
			break;
		}
	}

	return;
}

void CEndlessUsbToolDlg::AddLanguagesToUI()
{
    FUNCTION_ENTER;

	loc_cmd* lcmd = NULL;
	CComPtr<IHTMLSelectElement> selectElement;
	CComPtr<IHTMLSelectElement> selectElementDualBoot;
	HRESULT hr;

	hr = GetSelectElement(_T(ELEMENT_LANGUAGE_SELECT), selectElement);
	IFFALSE_RETURN(SUCCEEDED(hr) && selectElement != NULL, "Error returned from GetSelectElement.");

	hr = selectElement->put_length(0);

	hr = GetSelectElement(_T(ELEMENT_LANGUAGE_DUALBOOT), selectElementDualBoot);
	IFFALSE_RETURN(SUCCEEDED(hr) && selectElementDualBoot != NULL, "Error returned from GetSelectElement.");

	hr = selectElementDualBoot->put_length(0);

	int index = 0;
	// Add languages to dropdown and apply localization
	list_for_each_entry(lcmd, &locale_list, loc_cmd, list) {
		luprintf("Language available : %s", lcmd->txt[1]);

		hr = AddEntryToSelect(selectElement, UTF8ToBSTR(lcmd->txt[0]), UTF8ToBSTR(lcmd->txt[1]), NULL, m_selectedLocale == lcmd ? TRUE : FALSE);
		IFFALSE_RETURN(SUCCEEDED(hr), "Error adding the new option element to the select element");

		hr = AddEntryToSelect(selectElementDualBoot, UTF8ToBSTR(lcmd->txt[0]), UTF8ToBSTR(lcmd->txt[1]), NULL, m_selectedLocale == lcmd ? TRUE : FALSE);
		IFFALSE_RETURN(SUCCEEDED(hr), "Error adding the new option element to the select element on the dual boot page");
	}
}

void CEndlessUsbToolDlg::ChangePage(PCTSTR newPage)
{
    FUNCTION_ENTER;

    static CString currentPage(ELEMENT_DUALBOOT_PAGE);
    uprintf("ChangePage requested from %ls to %ls", currentPage, newPage);

    if (currentPage == newPage) {
        uprintf("ERROR: Already on that page.");
        return;
    }

    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(currentPage), CComVariant(FALSE));
    currentPage = newPage;
    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(currentPage), CComVariant(TRUE));

	Analytics::instance()->screenTracking(currentPage);

	return;
}

void CEndlessUsbToolDlg::ErrorOccured(ErrorCause_t errorCause)
{
    uint32_t buttonMsgId = 0, suggestionMsgId = 0;

    switch (errorCause) {
    case ErrorCause_t::ErrorCauseDownloadFailed:
        buttonMsgId = MSG_326;
        suggestionMsgId = MSG_323;
        break;
    case ErrorCause_t::ErrorCauseDownloadFailedDiskFull:
        buttonMsgId = MSG_326;
        // TODO: distinguish between "not enough space to download" and "not enough space to install",
        // use MSG_350 / MSG_351 in the latter case
        suggestionMsgId = MSG_334;
        break;
    case ErrorCause_t::ErrorCauseJSONDownloadFailed:
    case ErrorCause_t::ErrorCauseVerificationFailed:
        buttonMsgId = MSG_327;
        suggestionMsgId = MSG_324;
        break;
    case ErrorCause_t::ErrorCauseCanceled:
    case ErrorCause_t::ErrorCauseGeneric:
    case ErrorCause_t::ErrorCauseWriteFailed:
        buttonMsgId = MSG_328;
        suggestionMsgId = m_dualBootSelected ? MSG_358 : MSG_325;
        break;
    case ErrorCause_t::ErrorCauseNot64Bit:
        buttonMsgId = MSG_328;
        // TODO: headline should be MSG_354
        suggestionMsgId = MSG_355;
        break;
    case ErrorCause_t::ErrorCauseBitLocker:
        buttonMsgId = MSG_328;
        // TODO: headline should be MSG_356, with system drive letter
        suggestionMsgId = MSG_357;
        break;
    case ErrorCause_t::ErrorCauseNotNTFS:
        buttonMsgId = MSG_328;
        // TODO: headline should be MSG_352, with system drive letter
        suggestionMsgId = MSG_353;
        break;
    case ErrorCause_t::ErrorCauseNonWindowsMBR:
        buttonMsgId = MSG_328;
        // TODO: headline should be MSG_359
        suggestionMsgId = MSG_360;
        break;
    default:
        uprintf("Unhandled error cause %ls(%d)", ErrorCauseToStr(errorCause), errorCause);
        break;
    }

    // Update the error button text if it's a "recoverable" error case or hide it otherwise
    if (buttonMsgId != 0) {
        SetElementText(_T(ELEMENT_ERROR_BUTTON), UTF8ToBSTR(lmprintf(buttonMsgId)));
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_ERROR_BUTTON)), CComVariant(TRUE));
    } else {
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_ERROR_BUTTON)), CComVariant(FALSE));
    }

    // Ask user to delete the file that didn't pass signature verification
    // Trying again with the same file will result in the same verification error
    bool fileVerificationFailed = (errorCause == ErrorCause_t::ErrorCauseVerificationFailed);
    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_ERROR_DELETE_CHECKBOX), CComVariant(fileVerificationFailed));
    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_ERROR_BUTTON))), CComVariant(!fileVerificationFailed));
    CComBSTR deleteFilesText("");
    if (fileVerificationFailed) {
        bool isInstallerImage = (m_localFile == m_localInstallerImage.filePath);
        ULONGLONG invalidFileSize = isInstallerImage ? m_localInstallerImage.size : m_selectedFileSize;
        deleteFilesText = UTF8ToBSTR(lmprintf(MSG_336, SizeToHumanReadable(invalidFileSize, FALSE, use_fake_units)));
        CallJavascript(_T(JS_RESET_CHECK), CComVariant(_T(ELEMENT_ERROR_DELETE_CHECKBOX)));
    }
    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_ERROR_DELETE_CHECKBOX), CComVariant(fileVerificationFailed));
    SetElementText(_T(ELEMENT_ERROR_DELETE_TEXT), deleteFilesText);

    // Update the error description and "recovery" suggestion
    if (suggestionMsgId != 0) {
        CComBSTR message;
        if (suggestionMsgId == MSG_334) {
            POSITION p = m_remoteImages.FindIndex(m_selectedRemoteIndex);
            ULONGLONG size = 0;
            if (p != NULL) {
                RemoteImageEntry_t remote = m_remoteImages.GetAt(p);
                size = remote.compressedSize;
            }
            // we don't take the signature files into account but we are taking about ~2KB compared to >2GB
            ULONGLONG totalSize = size + (m_liveInstall || m_dualBootSelected ? 0 : m_installerImage.compressedSize);
            message = UTF8ToBSTR(lmprintf(suggestionMsgId, SizeToHumanReadable(totalSize, FALSE, use_fake_units)));
        } else {
            message = UTF8ToBSTR(lmprintf(suggestionMsgId));
        }
        SetElementText(_T(ELEMENT_ERROR_SUGGESTION), message);
    }

    ChangePage(_T(ELEMENT_ERROR_PAGE));

	bool fatal = FALSE;
	switch (errorCause) {
	case ErrorCause_t::ErrorCauseDownloadFailed:
	case ErrorCause_t::ErrorCauseVerificationFailed:
	case ErrorCause_t::ErrorCauseWriteFailed:
		fatal = FALSE;
	default:
		fatal = TRUE;
	}
	Analytics::instance()->exceptionTracking(ErrorCauseToStr(errorCause), fatal);
}

HRESULT CEndlessUsbToolDlg::GetSelectedOptionElementText(CComPtr<IHTMLSelectElement> selectElem, CString &text)
{
	IHTMLOptionElement *optionElem;
	HRESULT hr;
	IDispatch *dispOpt;
	long idx;
	_variant_t index;
	hr = selectElem->get_selectedIndex(&idx);
	IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error getting selected index");
	index = idx;
	hr = selectElem->item(index, index, &dispOpt);
	IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error accessing select element index");
	hr = dispOpt->QueryInterface(IID_IHTMLOptionElement, (void **) &optionElem);
	dispOpt->Release();
	IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error getting option element");
	BSTR bstrText;
	hr = optionElem->get_text(&bstrText);
	optionElem->Release();
	IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error getting option text");
	text = CString(bstrText);
	SysFreeString(bstrText);

error:
	return hr;
}

HRESULT CEndlessUsbToolDlg::GetSelectElement(PCTSTR selectId, CComPtr<IHTMLSelectElement> &selectElem)
{
    CComPtr<IHTMLElement> pElement;
    HRESULT hr;

    hr = m_spHtmlDoc3->getElementById(CComBSTR(selectId), &pElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr) && pElement != NULL, "Error when querying for select element.");

    hr = pElement.QueryInterface<IHTMLSelectElement>(&selectElem);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for IHTMLSelectElement interface");

error:
    return hr;
}

HRESULT CEndlessUsbToolDlg::ClearSelectElement(PCTSTR selectId)
{
    FUNCTION_ENTER;

    CComPtr<IHTMLSelectElement> selectElement;
    HRESULT hr;

    hr = GetSelectElement(selectId, selectElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr) && selectElement != NULL, "Error returned from GetSelectElement");

    hr = selectElement->put_length(0);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error removing all elements from HTML element");

error:
    return hr;
}

HRESULT CEndlessUsbToolDlg::AddEntryToSelect(PCTSTR selectId, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected)
{
    CComPtr<IHTMLSelectElement> selectElement;
    HRESULT hr;
    
    hr = GetSelectElement(selectId, selectElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr) && selectElement != NULL, "Error returned from GetSelectElement");

    return AddEntryToSelect(selectElement, value, text, outIndex, selected);

error:
    luprintf("AddEntryToSelect error on select [%ls] and entry (%s, %s)", selectId, text, value);
    return hr;
}

HRESULT CEndlessUsbToolDlg::AddEntryToSelect(CComPtr<IHTMLSelectElement> &selectElem, const CComBSTR &value, const CComBSTR &text, long *outIndex, BOOL selected)
{
    CComPtr<IHTMLElement> pElement;
    CComPtr<IHTMLOptionElement> optionElement;
    HRESULT hr;

    hr = m_spHtmlDoc->createElement(CComBSTR("option"), &pElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error when creating the option element");

    hr = pElement.QueryInterface<IHTMLOptionElement>(&optionElement);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error when querying for IHTMLOptionElement interface");

    optionElement->put_selected(selected);
    optionElement->put_value(value);
    optionElement->put_text(text);

    long length;
    hr = selectElem->get_length(&length);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying the select element length.");
    if (outIndex != NULL) {
        *outIndex = length;
    }

    hr = selectElem->add(pElement, CComVariant(length));
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error adding the new option element to the select element");

    return S_OK;
error:
    luprintf("AddEntryToSelect error on adding entry (%ls, %ls)", text, value);
    return hr;
}

bool CEndlessUsbToolDlg::IsButtonDisabled(IHTMLElement *pElement)
{
    FUNCTION_ENTER;

    CComPtr<IHTMLElement> parentElem;
    CComBSTR className;

    IFFALSE_RETURN_VALUE(pElement != NULL, "IsButtonDisabled pElement is NULL", false);
    IFFALSE_RETURN_VALUE(SUCCEEDED(pElement->get_parentElement(&parentElem)) && parentElem != NULL, "IsButtonDisabled Error querying for parent element", false);
    IFFALSE_RETURN_VALUE(SUCCEEDED(parentElem->get_className(&className)) && className != NULL, "IsButtonDisabled Error querying for class name", false);

    return wcsstr(className, _T(CLASS_BUTTON_DISABLED)) != NULL;    
}

//HRESULT CEndlessUsbToolDlg::OnHtmlSelectStart(IHTMLElement* pElement)
//{
//	POINT pt;
//	GetCursorPos(&pt);
//	::SendMessage(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
//
//	//do not pass the event to the IE server/JavaScript
//	return S_FALSE;
//}

// Dual Boot Page Handlers
HRESULT CEndlessUsbToolDlg::OnAdvancedOptionsClicked(IHTMLElement* pElement)
{
	FUNCTION_ENTER;

	m_dualBootSelected = false;
	m_liveInstall = true;
	ChangePage(_T(ELEMENT_FIRST_PAGE));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnInstallDualBootClicked(IHTMLElement* pElement)
{
	IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnInstallDualBootClicked: Button is disabled. ", S_OK);

	FUNCTION_ENTER;

	m_dualBootSelected = true;
	GoToSelectFilePage();

	return S_OK;
}

// First Page Handlers
HRESULT CEndlessUsbToolDlg::OnTryEndlessSelected(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

	Analytics::instance()->eventTracking(_T(ELEMENT_FIRST_PAGE), _T("USBType"), _T("Live"));

    m_liveInstall = true;
    GoToSelectFilePage();

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnInstallEndlessSelected(IHTMLElement* pElement)
{
    IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnInstallEndlessSelected: Button is disabled. ", S_OK);

    FUNCTION_ENTER;

	Analytics::instance()->eventTracking(_T(ELEMENT_FIRST_PAGE), _T("USBType"), _T("Installer"));

    m_liveInstall = false;
    GoToSelectFilePage();

	return S_OK;
}

void CEndlessUsbToolDlg::GoToSelectFilePage()
{
    FUNCTION_ENTER;

    CComPtr<IHTMLElement> selectElem;
    CComBSTR sizeText;
    RemoteImageEntry_t r;

    // Check locally available images
    UpdateFileEntries(true);

    // Update UI based on what was found
    bool canUseLocal = CanUseLocalFile();
    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_LOCAL_FILES_FOUND), CComVariant(canUseLocal));
    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_LOCAL_FILES_NOT_FOUND), CComVariant(!canUseLocal));
    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_SELFILE_NEXT_BUTTON)), CComVariant(canUseLocal));

    if (canUseLocal) {
        SetElementText(_T(ELEMENT_SET_FILE_TITLE), UTF8ToBSTR(lmprintf(MSG_321)));
        SetElementText(_T(ELEMENT_SET_FILE_SUBTITLE), UTF8ToBSTR(lmprintf(MSG_322)));
    } else if(CanUseRemoteFile()) {
        ApplyRufusLocalization();
    } else {
        uprintf("No remote images available and no local images available.");
        m_lastErrorCause = ErrorCause_t::ErrorCauseJSONDownloadFailed;
        CancelRunningOperation();
        return;
    }

    // Update page with no local files found
    POSITION p = m_remoteImages.FindIndex(m_baseImageRemoteIndex);
    if (p != NULL) {
        r = m_remoteImages.GetAt(p);

        // Update light download size
        ULONGLONG size = r.compressedSize + (m_liveInstall || m_dualBootSelected ? 0 : m_installerImage.compressedSize);
        sizeText = UTF8ToBSTR(lmprintf(MSG_315, SizeToHumanReadable(size, FALSE, use_fake_units)));
        SetElementText(_T(ELEMENT_DOWNLOAD_LIGHT_SIZE), sizeText);

        // Update full download size
        HRESULT hr = GetElement(_T(ELEMENT_REMOTE_SELECT), &selectElem);
        IFFALSE_GOTOERROR(SUCCEEDED(hr), "GoToSelectFilePage: querying for local select element.");
        bool useLocalFile = m_useLocalFile;
        OnSelectedRemoteFileChanged(selectElem);
        m_useLocalFile = useLocalFile;
    }

    ChangePage(_T(ELEMENT_FILE_PAGE));

    return;

error:
    ErrorOccured(ErrorCause_t::ErrorCauseGeneric);
}

HRESULT CEndlessUsbToolDlg::OnLinkClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    CComBSTR id;
    HRESULT hr;
    uint32_t msg_id = 0;
    char *url = NULL;

    IFFALSE_RETURN_VALUE(pElement != NULL, "OnLinkClicked: Error getting element id", S_OK);

    hr = pElement->get_id(&id);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "OnLinkClicked: Error getting element id", S_OK);

    if (id == _T(ELEMENT_COMPARE_OPTIONS)) {
        msg_id = MSG_312;
    } else if (id == _T(ELEMENT_ENDLESS_SUPPORT) || id == _T(ELEMENT_CONNECTED_SUPPORT_LINK) || id == _T(ELEMENT_STORAGE_SUPPORT_LINK)) {
        msg_id = MSG_314;
    } else if (id == _T(ELEMENT_CONNECTED_LINK)) {
        WinExec("c:\\windows\\system32\\control.exe ncpa.cpl", SW_NORMAL);
	} else if (id == _T(ELEMENT_USBBOOT_HOWTO)) {
		msg_id = MSG_329;
    } else {
        msg_id = 0;
        uprintf("Unknown link clicked %ls", id);
        return S_OK;
    }

    if (msg_id != 0) {
        // Radu: do we care about the errors?
        ShellExecuteA(NULL, "open", lmprintf(msg_id), NULL, NULL, SW_SHOWNORMAL);
    }

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnLanguageChanged(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

	CComPtr<IHTMLSelectElement> selectElement;
	CComBSTR selectedValue;

	HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement, (void**)&selectElement);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && selectElement != NULL, "Error querying for IHTMLSelectElement interface");

	hr = selectElement->get_value(&selectedValue);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && selectElement != NULL, "Error getting selected language value");

	char* p = _com_util::ConvertBSTRToString(selectedValue);
	Analytics::instance()->setLanguage(CString(p));
	m_selectedLocale = get_locale_from_name(p, TRUE);
	delete[] p;
	selected_langid = get_language_id(m_selectedLocale);

	reinit_localization();
	msg_table = NULL;

	if (get_loc_data_file(m_localizationFile, m_selectedLocale))
		uprintf("Save locale to settings?");

	ApplyRufusLocalization();

    m_selectedRemoteIndex = -1;
    AddDownloadOptionsToUI();

	return S_OK;

error:
	//RADU: what to do on error?
	return S_OK;
}

void CEndlessUsbToolDlg::UpdateFileEntries(bool shouldInit)
{
    FUNCTION_ENTER;

    CComPtr<IHTMLElement> pElement;
    CComPtr<IHTMLSelectElement> selectElement;
    HRESULT hr;
    WIN32_FIND_DATA findFileData;
    POSITION position;
    pFileImageEntry_t currentEntry = NULL;
    CString currentPath;
    BOOL fileAccessException = false;
    CString currentInstallerVersion;
    CString searchPath = GET_LOCAL_PATH(ALL_FILES);
    HANDLE findFilesHandle = FindFirstFile(searchPath, &findFileData);

    m_localFilesScanned = true;

    // get needed HTML elements
    hr = GetSelectElement(_T(ELEMENT_FILES_SELECT), selectElement);
    IFFALSE_RETURN(SUCCEEDED(hr), "Error returned from GetSelectElement");

    if (m_imageFiles.IsEmpty()) {
        hr = selectElement->put_length(0);
        IFFALSE_RETURN(SUCCEEDED(hr), "Error removing all elements from HTML element");
    }

    // mark all files as not present
    for (position = m_imageFiles.GetStartPosition(); position != NULL; ) {
        m_imageFiles.GetNextAssoc(position, currentPath, currentEntry);
        currentEntry->stillPresent = FALSE;
    }
    m_localInstallerImage.stillPresent = FALSE;

    if (findFilesHandle == INVALID_HANDLE_VALUE) {
        uprintf("UpdateFileEntries: No files found in current directory [%ls]", m_appDir);
        goto checkEntries;
    }

    do {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
        CString currentFile = findFileData.cFileName;
        CString fullPathFile = GET_LOCAL_PATH(findFileData.cFileName);
        CString extension = CSTRING_GET_LAST(currentFile, '.');
        if (extension != L"gz" && extension != L"xz") continue;

        if (!PathFileExists(fullPathFile)) continue; // file is present
        if (!PathFileExists(fullPathFile + SIGNATURE_FILE_EXT)) continue; // signature file is present

        try {
            CString displayName, personality, version;
            bool isInstallerImage = false;
            if (!ParseImgFileName(currentFile, personality, version, isInstallerImage)) continue;
            if (0 == GetExtractedSize(fullPathFile, isInstallerImage)) continue;
            CFile file(fullPathFile, CFile::modeRead);
            GetImgDisplayName(displayName, version, personality, file.GetLength());

            if (isInstallerImage) {
                if (version > currentInstallerVersion) {
                    currentInstallerVersion = version;
                    m_localInstallerImage.stillPresent = TRUE;
                    m_localInstallerImage.filePath = file.GetFilePath();
                    m_localInstallerImage.size = file.GetLength();
                }
            } else {
                // add entry to list or update it
                pFileImageEntry_t currentEntry = NULL;
                if (!m_imageFiles.Lookup(file.GetFilePath(), currentEntry)) {
                    currentEntry = new FileImageEntry_t;
                    currentEntry->autoAdded = TRUE;
                    currentEntry->filePath = file.GetFilePath();
                    currentEntry->size = file.GetLength();
					currentEntry->personality = personality;
                    AddEntryToSelect(selectElement, CComBSTR(currentEntry->filePath), CComBSTR(displayName), &currentEntry->htmlIndex, 0);
                    IFFALSE_RETURN(SUCCEEDED(hr), "Error adding item in image file list.");

                    m_imageFiles.SetAt(currentEntry->filePath, currentEntry);
                    m_imageIndexToPath.AddTail(currentEntry->filePath);
                }
                currentEntry->stillPresent = TRUE;
				CString basePath = CSTRING_GET_PATH(CSTRING_GET_PATH(fullPathFile, '.'), '.');

#if TEST_RELEASE_HARDCODED_STUFF
				CString hardcodedPath = GET_LOCAL_PATH(CSTRING_GET_LAST(CString(HARDCODED_PATH), '/'));
				currentEntry->hasBootArchive = PathFileExists(hardcodedPath + BOOT_ARCHIVE_SUFFIX);
				currentEntry->hasBootArchiveSig = PathFileExists(hardcodedPath + BOOT_ARCHIVE_SUFFIX + SIGNATURE_FILE_EXT);
#else
				currentEntry->hasBootArchive = PathFileExists(basePath + BOOT_ARCHIVE_SUFFIX);
				currentEntry->hasBootArchiveSig = PathFileExists(basePath + BOOT_ARCHIVE_SUFFIX + SIGNATURE_FILE_EXT);
#endif
				currentEntry->hasUnpackedImgSig = PathFileExists(basePath + IMAGE_FILE_EXT + SIGNATURE_FILE_EXT);
            }

			uprintf("Found local image '%ls'", file.GetFilePath());

            // RADU: do we need to care about the size?

        } catch (CFileException *ex) {
            uprintf("CFileException on file [%ls] with cause [%d] and OS error [%d]", findFileData.cFileName, ex->m_cause, ex->m_lOsError);
            ex->Delete();
            fileAccessException = TRUE;
        }
    } while (FindNextFile(findFilesHandle, &findFileData) != 0);

checkEntries:
    int htmlIndexChange = 0;
    for (position = m_imageIndexToPath.GetHeadPosition(); position != NULL; ) {
        POSITION currentPosition = position;
        currentPath = m_imageIndexToPath.GetNext(position);
        if (!m_imageFiles.Lookup(currentPath, currentEntry)) {
            uprintf("Error: path [%s] not found anymore, removing.");
            m_imageIndexToPath.RemoveAt(currentPosition);
            continue;
        }

        if (!currentEntry->autoAdded) {
            findFilesHandle = FindFirstFile(currentEntry->filePath, &findFileData);
            if (findFilesHandle != INVALID_HANDLE_VALUE) {
                currentEntry->stillPresent = TRUE;
            }
        }

        if (htmlIndexChange != 0) {
            uprintf("Changing HTML index with %d for %ls", htmlIndexChange, currentEntry->filePath);
            currentEntry->htmlIndex -= htmlIndexChange;
        }

        if (!currentEntry->stillPresent) {
            uprintf("Removing %ls", currentPath);
            selectElement->remove(currentEntry->htmlIndex);
            delete currentEntry;
            m_imageFiles.RemoveKey(currentPath);
            htmlIndexChange++;
            m_imageIndexToPath.RemoveAt(currentPosition);
        }
    }

    FindClose(findFilesHandle);

    // if there was an exception when accessing one of the files do a retry in 1 second
    if (fileAccessException) {
        SetTimer(TID_UPDATE_FILES, 1000, RefreshTimer);
    } else {
        KillTimer(TID_UPDATE_FILES);
    }

    bool hasLocalImages = m_imageFiles.GetCount() != 0;
    if (!hasLocalImages) {
        m_useLocalFile = false;
    }

    // set selected image for Rufus code
    CComBSTR selectedValue;
    hr = selectElement->get_value(&selectedValue);
    IFFALSE_RETURN(SUCCEEDED(hr) && selectElement != NULL, "Error getting selected file value");

    if (m_useLocalFile) {
        safe_free(image_path);
        if (selectedValue != NULL) {
            image_path = wchar_to_utf8(selectedValue);
        }
    }

    if (shouldInit) {
        //// start the change notifications thread
        //if (hasLocalImages && m_fileScanThread == INVALID_HANDLE_VALUE) {
        //    m_fileScanThread = CreateThread(NULL, 0, CEndlessUsbToolDlg::FileScanThread, (LPVOID)this, 0, NULL);
        //}
    }
}

DWORD WINAPI CEndlessUsbToolDlg::FileScanThread(void* param)
{
    FUNCTION_ENTER;

    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*)param;
    DWORD error = 0;
    HANDLE handlesToWaitFor[2];
    DWORD changeNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME;
    //changeNotifyFilter |= FILE_NOTIFY_CHANGE_SIZE;

    handlesToWaitFor[0] = dlg->m_closeFileScanThreadEvent;
    handlesToWaitFor[1] = FindFirstChangeNotification(dlg->m_appDir, FALSE, changeNotifyFilter);
    if (handlesToWaitFor[1] == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        uprintf("Error on FindFirstChangeNotificationA error=[%d]", error);
        return error;
    }

    while (TRUE)
    {
        DWORD dwWaitStatus = WaitForMultipleObjects(2, handlesToWaitFor, FALSE, INFINITE);
        switch (dwWaitStatus)
        {
        case WAIT_OBJECT_0:
            uprintf("FileScanThread: exit requested.");
            FindCloseChangeNotification(handlesToWaitFor[1]);
            return 0;
        case WAIT_OBJECT_0 + 1:
            ::PostMessage(hMainDialog, WM_FILES_CHANGED, 0, 0);
            if (FindNextChangeNotification(handlesToWaitFor[1]) == FALSE) {
                error = GetLastError();
                uprintf("FileScanThread ERROR: FindNextChangeNotification function failed with error [%d].", GetLastError());
                return error;
            }
            break;
        default:
            uprintf("FileScanThread: unhandled wait status [%d]", dwWaitStatus);
            return GetLastError();
        }
    }

    return 0;
}

void CEndlessUsbToolDlg::StartJSONDownload()
{
    FUNCTION_ENTER;

    char tmp_path[MAX_PATH] = "";
    bool success;

    if (!m_isConnected) {
        uprintf("Device not connected to internet");
        CallJavascript(_T(JS_ENABLE_DOWNLOAD), CComVariant(FALSE), CComVariant(FALSE));
        return;
    }

    if (m_remoteImages.GetCount() != 0) {
        uprintf("List of remote images already downloaded");
        return;
    }

    // Add both JSONs to download, maybe user goes back and switches between Try and Install
#ifdef ENABLE_JSON_COMPRESSION
    CString liveJson(JSON_PACKED(JSON_LIVE_FILE));
    CString installerJson(JSON_PACKED(JSON_INSTALLER_FILE));
#else
    CString liveJson(JSON_LIVE_FILE);
    CString installerJson(JSON_INSTALLER_FILE);
#endif // ENABLE_JSON_COMPRESSION

    liveJson = GET_LOCAL_PATH(liveJson);
    installerJson = GET_LOCAL_PATH(installerJson);

    ListOfStrings urls = { JSON_URL(JSON_LIVE_FILE), JSON_URL(JSON_INSTALLER_FILE) };
    ListOfStrings files = { liveJson, installerJson };

    success = m_downloadManager.AddDownload(DownloadType_t::DownloadTypeReleseJson, urls, files, false);

    if(!success) {
        JSONDownloadFailed();
    }
}

#define JSON_IMAGES             "images"
#define JSON_VERSION            "version"

#define JSON_IMG_PERSONALITIES  "personalities"
#define JSON_IMG_ARCH           "arch"
#define JSON_IMG_BRANCH         "branch"
#define JSON_IMG_PERS_IMAGES    "personality_images"
#define JSON_IMG_PRODUCT        "product"
#define JSON_IMG_PLATFORM       "platform"
#define JSON_IMG_VERSION        "version"
#define JSON_IMG_FULL           "full"

#define JSON_IMG_COMPRESSED_SIZE    "compressed_size"
#define JSON_IMG_EXTRACTED_SIZE     "extracted_size"
#define JSON_IMG_URL_FILE           "file"
#define JSON_IMG_URL_SIG            "signature"

#define CHECK_ENTRY(parentValue, tag) \
do { \
    IFFALSE_CONTINUE(!parentValue[tag].isNull(), CString("\t\t Elem is NULL - ") + tag); \
    uprintf("\t\t %s=%s", tag, parentValue[tag].toStyledString().c_str()); \
} while(false);

bool CEndlessUsbToolDlg::UnpackFile(LPCSTR archive, LPCSTR destination, int compressionType, void* progress_function, unsigned long* cancel_request)
{
    FUNCTION_ENTER;

    int64_t result = 0;

    // RADU: provide a progress function and move this from UI thread
    // For initial release this is ok as the operation should be very fast for the JSON
    // Unpack the file
    result = bled_init(_uprintf, (progress_t)progress_function, cancel_request);
    result = bled_uncompress(archive, destination, compressionType);
    bled_exit();
    return result >= 0;
}

bool CEndlessUsbToolDlg::ParseJsonFile(LPCTSTR filename, bool isInstallerJson)
{
    FUNCTION_ENTER;

    Json::Reader reader;
    Json::Value rootValue, imagesElem, jsonElem, personalities, persImages, persImage, fullImage, latestEntry;
    CString latestVersion("");

    std::ifstream jsonStream;

#if TEST_RELEASE_HARDCODED_STUFF
	const char *beginDoc = isInstallerJson ? eosinstaller_hardcoded_json : eos_hardcoded_json;
	const char *endDoc = beginDoc + strlen(isInstallerJson ? eosinstaller_hardcoded_json : eos_hardcoded_json);

	IFFALSE_GOTOERROR(reader.parse(beginDoc, endDoc, rootValue, false), "Parsing of JSON failed.");
#else // TEST_RELEASE_HARDCODED_STUFF
    jsonStream.open(filename);
    IFFALSE_GOTOERROR(!jsonStream.fail(), "Opening JSON file failed.");
    IFFALSE_GOTOERROR(reader.parse(jsonStream, rootValue, false), "Parsing of JSON failed.");
#endif //TEST_RELEASE_HARDCODED_STUFF

    // Print version
    jsonElem = rootValue[JSON_VERSION];
    if (!jsonElem.isString()) uprintf("JSON Version: %s", jsonElem.asString().c_str());

    // Go through the image entries
    imagesElem = rootValue[JSON_IMAGES];
    IFFALSE_GOTOERROR(!jsonElem.isNull(), "Missing element " JSON_IMAGES);

    for (Json::ValueIterator it = imagesElem.begin(); it != imagesElem.end(); it++) {
        uprintf("Found entry '%s'", it.memberName());
        CHECK_ENTRY((*it), JSON_IMG_ARCH);
        CHECK_ENTRY((*it), JSON_IMG_BRANCH);
        CHECK_ENTRY((*it), JSON_IMG_PRODUCT);
        CHECK_ENTRY((*it), JSON_IMG_PLATFORM);
        CHECK_ENTRY((*it), JSON_IMG_VERSION);
        CHECK_ENTRY((*it), JSON_IMG_PERSONALITIES);

        personalities = (*it)[JSON_IMG_PERSONALITIES];
        IFFALSE_CONTINUE(personalities.isArray(), "Error: No valid personality_images entry found.");

        persImages = (*it)[JSON_IMG_PERS_IMAGES];
        IFFALSE_CONTINUE(!persImages.isNull(), "Error: No personality_images entry found.");

        CString currentVersion((*it)[JSON_IMG_VERSION].asCString());
        if (currentVersion > latestVersion) {
            latestVersion = currentVersion;
            latestEntry = *it;
        }
    }

    IFFALSE_GOTOERROR(!latestEntry.isNull(), "No images found in the JSON.");

    if (!latestEntry.isNull()) {
        uprintf("Selected version '%ls'", latestVersion);
        m_downloadManager.SetLatestEosVersion(latestVersion);
        uint32_t personalityMsgId = 0;
        personalities = latestEntry[JSON_IMG_PERSONALITIES];
        for (Json::ValueIterator persIt = personalities.begin(); persIt != personalities.end(); persIt++) {
            IFFALSE_CONTINUE(persIt->isString(), "Entry is not string, continuing");
            IFFALSE_CONTINUE(!isInstallerJson || CString(persIt->asCString()) == PERSONALITY_BASE, "Installer JSON parsing: not base personality");
            IFFALSE_CONTINUE(m_personalityToLocaleMsg.Lookup(CString(persIt->asCString()), personalityMsgId), "Unknown personality. Continuing.");

            persImage = persImages[persIt->asString()];
            IFFALSE_CONTINUE(!persImage.isNull(), CString("Personality image entry not found - ") + persIt->asCString());

            fullImage = persImage[JSON_IMG_FULL];
            IFFALSE_CONTINUE(!fullImage.isNull(), CString("'full' entry not found for personality - ") + persIt->asCString());

            CHECK_ENTRY(fullImage, JSON_IMG_COMPRESSED_SIZE);
            CHECK_ENTRY(fullImage, JSON_IMG_EXTRACTED_SIZE);
            CHECK_ENTRY(fullImage, JSON_IMG_URL_FILE);
            CHECK_ENTRY(fullImage, JSON_IMG_URL_SIG);

            RemoteImageEntry_t remoteImageEntry;
            RemoteImageEntry_t &remoteImage = isInstallerJson ? m_installerImage : remoteImageEntry;

            remoteImage.compressedSize = fullImage[JSON_IMG_COMPRESSED_SIZE].asUInt64();
            remoteImage.extractedSize = fullImage[JSON_IMG_EXTRACTED_SIZE].asUInt64();
            // before releases extractedSize is not populated; add the compressed size so we don't show 0
            if(remoteImage.extractedSize == 0) remoteImage.extractedSize = remoteImage.compressedSize;            
            remoteImage.urlFile = fullImage[JSON_IMG_URL_FILE].asCString();
            remoteImage.urlSignature = fullImage[JSON_IMG_URL_SIG].asCString();
            remoteImage.personality = persIt->asCString();
            remoteImage.version = latestVersion;

            if(!isInstallerJson) {
                // Create display name
                GetImgDisplayName(remoteImage.displayName, remoteImage.version, remoteImage.personality, remoteImage.compressedSize);

                // Create dowloadJobName
                remoteImage.downloadJobName = latestVersion;
                remoteImage.downloadJobName += persIt->asCString();

                m_remoteImages.AddTail(remoteImage);
            }
        }
    }

    return true;
error:
    uprintf("JSON parsing failed. Parser error messages %s", reader.getFormattedErrorMessages().c_str());
    return false;
}

void CEndlessUsbToolDlg::UpdateDownloadOptions()
{
    FUNCTION_ENTER;

    CString filePath;
#ifdef ENABLE_JSON_COMPRESSION
    CString filePathGz;
#endif // ENABLE_JSON_COMPRESSION

    m_remoteImages.RemoveAll();

    // Parse JSON with normal images
    filePath = GET_LOCAL_PATH(CString(JSON_LIVE_FILE));
#ifdef ENABLE_JSON_COMPRESSION
    filePathGz = GET_LOCAL_PATH(CString(JSON_PACKED(JSON_LIVE_FILE)));
    IFFALSE_GOTOERROR(UnpackFile(ConvertUnicodeToUTF8(filePathGz), ConvertUnicodeToUTF8(filePath), BLED_COMPRESSION_GZIP), "Error uncompressing eos JSON file.");
#endif // ENABLE_JSON_COMPRESSION
    IFFALSE_GOTOERROR(ParseJsonFile(filePath, false), "Error parsing eos JSON file.");

    // Parse JSON with installer images
    filePath = GET_LOCAL_PATH(CString(JSON_INSTALLER_FILE));
#ifdef ENABLE_JSON_COMPRESSION
    filePathGz = GET_LOCAL_PATH(CString(JSON_PACKED(JSON_INSTALLER_FILE)));
    IFFALSE_GOTOERROR(UnpackFile(ConvertUnicodeToUTF8(filePathGz), ConvertUnicodeToUTF8(filePath), BLED_COMPRESSION_GZIP), "Error uncompressing eosinstaller JSON file.");
#endif // ENABLE_JSON_COMPRESSION
    IFFALSE_GOTOERROR(ParseJsonFile(filePath, true), "Error parsing eosinstaller JSON file.");

    return;

error:
    // RADU: disable downloading here I assume? Or retry download/parse?
    CallJavascript(_T(JS_ENABLE_DOWNLOAD), CComVariant(FALSE), CComVariant(m_isConnected));
    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_DOWNLOAD_LIGHT_BUTTON))), CComVariant(FALSE));
    return;
}

void CEndlessUsbToolDlg::AddDownloadOptionsToUI()
{
    CString languagePersonalty = PERSONALITY_ENGLISH;

    // remove all options from UI
    HRESULT hr = ClearSelectElement(_T(ELEMENT_REMOTE_SELECT));
    IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error clearing remote images select.");
    hr = ClearSelectElement(_T(ELEMENT_SELFILE_DOWN_LANG));
    IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error clearing remote images select.");

    // get selected language
    if (!m_localeToPersonality.Lookup(m_selectedLocale->txt[0], languagePersonalty)) {
        uprintf("ERROR: Selected language personality not found. Defaulting to English");
    }

    // add options to UI
    bool updatedFullSize = false;
    long selectIndex = -1;
    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_DOWNLOAD_LIGHT_BUTTON))), CComVariant(FALSE));
    for (POSITION pos = m_remoteImages.GetHeadPosition(); pos != NULL; ) {
        RemoteImageEntry_t imageEntry = m_remoteImages.GetNext(pos);
        bool matchesLanguage = languagePersonalty == imageEntry.personality;

        hr = AddEntryToSelect(_T(ELEMENT_REMOTE_SELECT), CComBSTR(""), CComBSTR(imageEntry.displayName), &selectIndex, matchesLanguage);
        IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error adding remote image to list.");

        // option size
        ULONGLONG size = imageEntry.compressedSize + (m_liveInstall || m_dualBootSelected ? 0 : m_installerImage.compressedSize);
        CComBSTR sizeText = UTF8ToBSTR(lmprintf(MSG_315, SizeToHumanReadable(size, FALSE, use_fake_units)));
        const wchar_t *htmlElemId = NULL;

        if (imageEntry.personality == PERSONALITY_BASE) {
            CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_DOWNLOAD_LIGHT_BUTTON))), CComVariant(TRUE));
            m_baseImageRemoteIndex = selectIndex;
            htmlElemId = _T(ELEMENT_DOWNLOAD_LIGHT_SIZE);
        }
        else {
            CString imageLanguage = UTF8ToCString(lmprintf(m_personalityToLocaleMsg[imageEntry.personality]));
            CString indexStr;
            indexStr.Format(L"%d", selectIndex);
            hr = AddEntryToSelect(_T(ELEMENT_SELFILE_DOWN_LANG), CComBSTR(indexStr), CComBSTR(imageLanguage), NULL, matchesLanguage);
            IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error adding remote image to full images list.");
            if (!updatedFullSize) {
                updatedFullSize = true;
                htmlElemId = _T(ELEMENT_DOWNLOAD_FULL_SIZE);
            }
        }
        if (htmlElemId != 0) {
            SetElementText(htmlElemId, sizeText);
        }
    }

    bool foundRemoteImages = m_remoteImages.GetCount() != 0;
    hr = CallJavascript(_T(JS_ENABLE_DOWNLOAD), CComVariant(foundRemoteImages), CComVariant(m_isConnected));
    IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error calling javascript to enable/disable download posibility.");

    if (m_imageFiles.GetCount() == 0) {
        m_selectedRemoteIndex = m_baseImageRemoteIndex;
    }
}

HRESULT CEndlessUsbToolDlg::OnFirstPagePreviousClicked(IHTMLElement* pElement)
{
	ChangePage(_T(ELEMENT_DUALBOOT_PAGE));

	return S_OK;
}

// Select File Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectFilePreviousClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    ChangePage(m_dualBootSelected ? _T(ELEMENT_DUALBOOT_PAGE) : _T(ELEMENT_FIRST_PAGE));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectFileNextClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    LPITEMIDLIST pidlDesktop = NULL;
    SHChangeNotifyEntry NotifyEntry;
    
    IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnSelectFileNextClicked: Button is disabled. ", S_OK);

	if (!m_dualBootSelected) {
		CallJavascript(_T(JS_RESET_CHECK), CComVariant(_T(ELEMENT_SELUSB_AGREEMENT)));
		m_usbDeleteAgreement = false;

		// RADU: move this to another thread
		GetUSBDevices(0);
		OnSelectedUSBDiskChanged(NULL);

		PF_INIT(SHChangeNotifyRegister, shell32);

		// Register MEDIA_INSERTED/MEDIA_REMOVED notifications for card readers
		if (pfSHChangeNotifyRegister && SUCCEEDED(SHGetSpecialFolderLocation(0, CSIDL_DESKTOP, &pidlDesktop))) {
			NotifyEntry.pidl = pidlDesktop;
			NotifyEntry.fRecursive = TRUE;
			// NB: The following only works if the media is already formatted.
			// If you insert a blank card, notifications will not be sent... :(
			m_shellNotificationsRegister = pfSHChangeNotifyRegister(m_hWnd, 0x0001 | 0x0002 | 0x8000,
				SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED, UM_MEDIA_CHANGE, 1, &NotifyEntry);
		}
	}

    // Get display name with actual image size, not compressed
    CString selectedImage, personality, version, selectedSize;
    ULONGLONG size = 0;
    bool isInstallerImage = false;
    if (m_useLocalFile) {
        selectedImage = UTF8ToCString(image_path);
        pFileImageEntry_t localEntry = NULL;
        if (!m_imageFiles.Lookup(selectedImage, localEntry)) {
            uprintf("ERROR: Selected local file not found.");
        } else {
            size = m_liveInstall ? GetExtractedSize(selectedImage, FALSE) : localEntry->size;
        }
        selectedImage = CSTRING_GET_LAST(selectedImage, '\\');

        m_selectedFileSize = localEntry->size;
    } else {
        RemoteImageEntry_t remote = m_remoteImages.GetAt(m_remoteImages.FindIndex(m_selectedRemoteIndex));
        //DownloadType_t downloadType = GetSelectedDownloadType();
        selectedImage = CSTRING_GET_LAST(remote.urlFile, '/');
        size = m_liveInstall ? remote.extractedSize : remote.compressedSize;
        
        m_selectedFileSize = remote.compressedSize;
    }

    // add the installer size if this is not a live image
    if (!m_liveInstall) {
        size += INSTALLER_DELTA_SIZE + m_installerImage.extractedSize;
    }    

    selectedSize = SizeToHumanReadable(size, FALSE, use_fake_units);
    if (ParseImgFileName(selectedImage, personality, version, isInstallerImage)) {
        if(isInstallerImage) uprintf("ERROR: An installer image has been selected.");

        uint32_t headlineMsg;
        if (m_dualBootSelected) {
            headlineMsg = MSG_320;
        }
        else if (m_liveInstall) {
            headlineMsg = MSG_343;
        }
        else {
            headlineMsg = MSG_344;
        }

        CString finalMessageStr = UTF8ToCString(lmprintf(headlineMsg));
        CString imageLanguage = UTF8ToCString(lmprintf(m_personalityToLocaleMsg[personality]));
        CString imageType = UTF8ToCString(lmprintf(personality == PERSONALITY_BASE ? MSG_400 : MSG_316)); // Basic or Full

        SetElementText(_T(ELEMENT_THANKYOU_MESSAGE), CComBSTR(finalMessageStr));

        SetElementText(_T(ELEMENT_INSTALLER_VERSION), CComBSTR(version));
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_INSTALLER_LANGUAGE_ROW), CComVariant(personality != PERSONALITY_BASE));
        SetElementText(_T(ELEMENT_INSTALLER_LANGUAGE), CComBSTR(imageLanguage));
        CString contentStr  = UTF8ToCString(lmprintf(MSG_319, imageType, SizeToHumanReadable(size, FALSE, use_fake_units)));
        SetElementText(_T(ELEMENT_INSTALLER_CONTENT), CComBSTR(contentStr));

        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_DUALBOOT_REMINDER), CComVariant(m_dualBootSelected));
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_LIVE_REMINDER), CComVariant(!m_dualBootSelected && m_liveInstall));
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_REFLASHER_REMINDER), CComVariant(!m_dualBootSelected && !m_liveInstall));
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_USBBOOT_HOWTO), CComVariant(!m_dualBootSelected));

        GetImgDisplayName(selectedImage, version, personality, 0);
    } else {
        uprintf("Cannot parse data from file name %ls; using default %s", selectedImage, ENDLESS_OS);
        selectedImage = _T(ENDLESS_OS);
    }

	if (!m_dualBootSelected) {
		SetElementText(_T(ELEMENT_SELUSB_NEW_DISK_NAME), CComBSTR(selectedImage));
		SetElementText(_T(ELEMENT_SELUSB_NEW_DISK_SIZE), CComBSTR(selectedSize));

		ChangePage(_T(ELEMENT_USB_PAGE));
	} else {
		GoToSelectStoragePage();
	}

	CString imageType = _T("RemoteImage");
	if (m_useLocalFile) imageType = _T("LocalImage");
	Analytics::instance()->eventTracking(_T(ELEMENT_FILE_PAGE), imageType, selectedImage);

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectFileButtonClicked(IHTMLElement* pElement)
{
    AfxMessageBox(_T("Not implemented yet."));
    //EXT_DECL(img_ext, NULL, __VA_GROUP__("*.img;*.gz;*.xz"), __VA_GROUP__(lmprintf(MSG_095)));

    //char *image_path = FileDialog(FALSE, NULL, &img_ext, 0);
    //CString selectedFilePath = Utf8ToCString(image_path);

    //CFile file(selectedFilePath, CFile::modeRead);

    //CString displayText = file.GetFileName() + " - ";
    //displayText += SizeToHumanReadable(file.GetLength(), FALSE, use_fake_units);
    //AddEntryToSelect(_T(ELEMENT_FILES_SELECT), CComBSTR(selectedFilePath), CComBSTR(displayText), NULL, 1);

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectedImageFileChanged(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    CComPtr<IHTMLSelectElement> selectElement;
    CComBSTR selectedValue;

    HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement, (void**)&selectElement);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectElement != NULL, "Error querying for IHTMLSelectElement interface", S_OK);

    hr = selectElement->get_value(&selectedValue);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectElement != NULL, "Error getting selected file value", S_OK);

    safe_free(image_path);
    image_path = wchar_to_utf8(selectedValue);
    uprintf("OnSelectedImageFileChanged to LOCAL [%s]", image_path);

    m_useLocalFile = true;

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectedRemoteFileChanged(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    CComPtr<IHTMLSelectElement> selectElement;
    long selectedIndex;

    HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement, (void**)&selectElement);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectElement != NULL, "Error querying for IHTMLSelectElement interface", S_OK);

    hr = selectElement->get_selectedIndex(&selectedIndex);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "Error getting selected index value", S_OK);

    m_selectedRemoteIndex = selectedIndex;
    POSITION p = m_remoteImages.FindIndex(selectedIndex);
    IFFALSE_RETURN_VALUE(p != NULL, "Index value not valid.", S_OK);
    RemoteImageEntry_t r = m_remoteImages.GetAt(p);
    uprintf("OnSelectedRemoteFileChanged to REMOTE [%ls]", r.displayName);

    if (r.personality != PERSONALITY_BASE) {
        ULONGLONG size = r.compressedSize + (m_liveInstall || m_dualBootSelected ? 0 : m_installerImage.compressedSize);
        CComBSTR sizeText = UTF8ToBSTR(lmprintf(MSG_315, SizeToHumanReadable(size, FALSE, use_fake_units)));
        SetElementText(_T(ELEMENT_DOWNLOAD_FULL_SIZE), sizeText);
    }

    m_useLocalFile = false;

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectedImageTypeChanged(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    CComBSTR id;
    HRESULT hr;
    CComPtr<IHTMLElement> selectElem;
    bool localFileSelected = false;

    hr = pElement->get_id(&id);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "OnSelectedImageTypeChanged: Error getting element id", S_OK);

    if (id == _T(ELEMENT_IMAGE_TYPE_LOCAL)) {
        hr = GetElement(_T(ELEMENT_FILES_SELECT), &selectElem);
        IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "OnSelectedImageTypeChanged: querying for local select element.", S_OK);
        OnSelectedImageFileChanged(selectElem);
    } else {
        hr = GetElement(_T(ELEMENT_REMOTE_SELECT), &selectElem);
        IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "OnSelectedImageTypeChanged: querying for local select element.", S_OK);
        OnSelectedRemoteFileChanged(selectElem);
    }

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnDownloadLightButtonClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    m_selectedRemoteIndex = m_baseImageRemoteIndex;
    OnSelectFileNextClicked(pElement);

    return S_OK;
}
HRESULT CEndlessUsbToolDlg::OnDownloadFullButtonClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    // trigger select onchange to update selected index
    CComPtr<IHTMLElement> pSelElem;
    GetElement(_T(ELEMENT_SELFILE_DOWN_LANG), &pSelElem);
    CComQIPtr<IHTMLElement3> spElem3(pSelElem);
    CComPtr<IHTMLEventObj> spEo;
    CComQIPtr<IHTMLDocument4> spDoc4(m_spHtmlDoc); // pDoc Document
    spDoc4->createEventObject(NULL, &spEo);
    CComQIPtr<IDispatch> spDisp(spEo);
    CComVariant var(spDisp);
    VARIANT_BOOL bCancel = VARIANT_FALSE;
    spElem3->fireEvent(L"onchange", &var, &bCancel);
 
    OnSelectFileNextClicked(pElement);

    return S_OK;
}

// Select USB Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectUSBPreviousClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

	LeavingDevicesPage();
	ChangePage(_T(ELEMENT_FILE_PAGE));

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectUSBNextClicked(IHTMLElement* pElement)
{
    IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnSelectUSBNextClicked: Button is disabled. ", S_OK);

    FUNCTION_ENTER;

	CComPtr<IHTMLSelectElement> selElem;
	HRESULT hr;

	hr = GetSelectElement(_T(ELEMENT_SELUSB_USB_DRIVES), selElem);
	if (hr == S_OK) {
		CString selUSBDrive;
		hr = GetSelectedOptionElementText(selElem, selUSBDrive);
		if (hr == S_OK)
			Analytics::instance()->eventTracking(_T(ELEMENT_USB_PAGE), _T("USBDisk"), selUSBDrive);
	}

	LeavingDevicesPage();
	StartInstallationProcess();

	return S_OK;
}

void CEndlessUsbToolDlg::StartInstallationProcess()
{
	SetElementText(_T(ELEMENT_INSTALL_STATUS), CComBSTR(""));

	EnableHibernate(false);

	if(!m_dualBootSelected) ChangeDriveAutoRunAndMount(true);

	// TODO: remove these once we parse proper JSON or discover local files
	m_bootArchive = GET_LOCAL_PATH(CSTRING_GET_LAST(hardcoded_BootPath, '/'));
	m_bootArchiveSig = GET_LOCAL_PATH(CSTRING_GET_LAST(hardcoded_BootPathAsc, '/'));
	// END REMOVE

	if (m_dualBootSelected && m_useLocalFile) {
		pFileImageEntry_t localEntry = NULL;
		CString selectedImage = UTF8ToCString(image_path);
		if (m_imageFiles.Lookup(selectedImage, localEntry)) {
			if(!localEntry->hasBootArchive || !localEntry->hasBootArchiveSig) m_useLocalFile = false;
		} else {
			m_useLocalFile = false;
		}
	}

	// Radu: we need to download an installer if only a live image is found and full install was selected
	if (m_useLocalFile) {
		if (m_dualBootSelected) {
			// get unpacked image signature file
			CString basePath = CSTRING_GET_PATH(CSTRING_GET_PATH(UTF8ToCString(image_path), '.'), '.');
			m_unpackedImageSig = basePath + IMAGE_FILE_EXT + SIGNATURE_FILE_EXT;

			m_localFile = m_bootArchive;
			m_localFileSig = m_bootArchiveSig;
		} else {
			m_localFile = UTF8ToCString(image_path);
			m_localFileSig = m_localFile + SIGNATURE_FILE_EXT;
		}

		StartOperationThread(OP_VERIFYING_SIGNATURE, CEndlessUsbToolDlg::FileVerificationThread);
	} else {
		UpdateCurrentStep(OP_DOWNLOADING_FILES);

		RemoteImageEntry_t remote = m_remoteImages.GetAt(m_remoteImages.FindIndex(m_selectedRemoteIndex));
		DownloadType_t downloadType = GetSelectedDownloadType();

		// live image file
		CString url = CString(RELEASE_JSON_URLPATH) + remote.urlFile;
		m_localFile = GET_LOCAL_PATH(CSTRING_GET_LAST(remote.urlFile, '/'));
		// live image signature file
		CString urlAsc = CString(RELEASE_JSON_URLPATH) + remote.urlSignature;
		m_localFileSig = GET_LOCAL_PATH(CSTRING_GET_LAST(remote.urlSignature, '/'));

		// get unpacked image signature file
		CString basePath = CSTRING_GET_PATH(CSTRING_GET_PATH(remote.urlFile, '.'), '.');
		m_unpackedImageSig = basePath + IMAGE_FILE_EXT + SIGNATURE_FILE_EXT;

		// add image file path for Rufus
		safe_free(image_path);
		image_path = wchar_to_utf8(m_localFile);

		// List of files to download
		ListOfStrings urls, files;
		CString urlInstaller, urlInstallerAsc, installerFile, installerAscFile;
		CString urlBootFiles, urlBootFilesAsc, urlImageSig;
		if (m_dualBootSelected) {
			urlBootFiles = CString(RELEASE_JSON_URLPATH) + hardcoded_BootPath;
			m_bootArchive = GET_LOCAL_PATH(CSTRING_GET_LAST(hardcoded_BootPath, '/'));

			urlBootFilesAsc = CString(RELEASE_JSON_URLPATH) + hardcoded_BootPathAsc;
			m_bootArchiveSig = GET_LOCAL_PATH(CSTRING_GET_LAST(hardcoded_BootPathAsc, '/'));

			urlImageSig = CString(RELEASE_JSON_URLPATH) + m_unpackedImageSig;
			m_unpackedImageSig = GET_LOCAL_PATH(CSTRING_GET_LAST(m_unpackedImageSig, '/'));

			urls = { url, urlAsc, urlBootFiles, urlBootFilesAsc, urlImageSig };
			files = { m_localFile, m_localFileSig, m_bootArchive, m_bootArchiveSig, m_unpackedImageSig };
		} else if (m_liveInstall) {
			urls = { url, urlAsc };
			files = { m_localFile, m_localFileSig };
		} else {
			// installer image file + signature
			urlInstaller = CString(RELEASE_JSON_URLPATH) + m_installerImage.urlFile;
			installerFile = GET_LOCAL_PATH(CSTRING_GET_LAST(m_installerImage.urlFile, '/'));

			urlInstallerAsc = CString(RELEASE_JSON_URLPATH) + m_installerImage.urlSignature;
			installerAscFile = GET_LOCAL_PATH(CSTRING_GET_LAST(m_installerImage.urlSignature, '/'));

			urls = { url, urlAsc, urlInstaller, urlInstallerAsc };
			files = { m_localFile, m_localFileSig, installerFile, installerAscFile };
		}

		// Try resuming the download if it exists
		bool status = m_downloadManager.AddDownload(downloadType, urls, files, true, remote.downloadJobName);
		if (!status) {
			// start the download again
			status = m_downloadManager.AddDownload(downloadType, urls, files, false, remote.downloadJobName);
			if (!status) {
				ChangePage(_T(ELEMENT_INSTALL_PAGE));
				// RADU: add custom error values for each of the errors so we can identify them and show a custom message for each
				uprintf("Error adding files for download");
				FormatStatus = FORMAT_STATUS_CANCEL;
				m_lastErrorCause = ErrorCause_t::ErrorCauseDownloadFailed;
				PostMessage(WM_FINISHED_ALL_OPERATIONS, 0, 0);
				return;
			}
		}

		// Create a thread to poll the download progress regularly as the event based BITS one is very irregular
		if (m_downloadUpdateThread == INVALID_HANDLE_VALUE) {
			m_downloadUpdateThread = CreateThread(NULL, 0, CEndlessUsbToolDlg::UpdateDownloadProgressThread, (LPVOID)this, 0, NULL);
		}

		// add remote installer data to local installer data
		m_localInstallerImage.stillPresent = TRUE;
		m_localInstallerImage.filePath = GET_LOCAL_PATH(CSTRING_GET_LAST(m_installerImage.urlFile, '/'));
		m_localInstallerImage.size = m_installerImage.compressedSize;

		if (m_dualBootSelected) {
			m_localFile = m_bootArchive;
			m_localFileSig = m_bootArchiveSig;
		}
	}

	ChangePage(_T(ELEMENT_INSTALL_PAGE));

	// RADU: wait for File scanning thread
	if (m_fileScanThread != INVALID_HANDLE_VALUE) {
		SetEvent(m_closeFileScanThreadEvent);
		uprintf("Waiting for scan files thread.");
		WaitForSingleObject(m_fileScanThread, 5000);
		m_fileScanThread = INVALID_HANDLE_VALUE;
		CloseHandle(m_closeFileScanThreadEvent);
		m_closeFileScanThreadEvent = INVALID_HANDLE_VALUE;
	}
}

void CEndlessUsbToolDlg::StartOperationThread(int operation, LPTHREAD_START_ROUTINE threadRoutine, LPVOID param)
{
    FUNCTION_ENTER;

    if (m_operationThread != INVALID_HANDLE_VALUE) {
        uprintf("StartThread: Another operation is in progress. Current operation %ls(%d), requested operation %ls(%d)", 
            OperationToStr(m_currentStep), m_currentStep, OperationToStr(operation), operation);
        return;
    }

    UpdateCurrentStep(operation);

    FormatStatus = 0;
    m_operationThread = CreateThread(NULL, 0, threadRoutine, param != NULL ? param : (LPVOID)this, 0, NULL);
    if (m_operationThread == NULL) {
        m_operationThread = INVALID_HANDLE_VALUE;
        uprintf("Error: Unable to start thread.");
        FormatStatus = ERROR_SEVERITY_ERROR | FAC(FACILITY_STORAGE) | APPERR(ERROR_CANT_START_THREAD);
        m_lastErrorCause = ErrorCause_t::ErrorCauseGeneric;
        PostMessage(WM_FINISHED_ALL_OPERATIONS, (WPARAM)FALSE, 0);
    }
}

HRESULT CEndlessUsbToolDlg::OnSelectedUSBDiskChanged(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    //int i;
    char fs_type[32];
    int deviceIndex = ComboBox_GetCurSel(hDeviceList);

    IFFALSE_GOTOERROR(deviceIndex >= 0, "Selected drive index is invalid.");

    memset(&SelectedDrive, 0, sizeof(SelectedDrive));
    SelectedDrive.DeviceNumber = (DWORD)ComboBox_GetItemData(hDeviceList, deviceIndex);
    GetDrivePartitionData(SelectedDrive.DeviceNumber, fs_type, sizeof(fs_type), FALSE);

    UpdateUSBSpeedMessage(deviceIndex);
    
    // Radu: same code found in OnSelectFileNextClicked, move to new method
    // check if final image will fit in the disk
    ULONGLONG size = 0;
    if (m_useLocalFile) {
        pFileImageEntry_t localEntry = NULL;
        CString selectedImage = UTF8ToCString(image_path);

        size = GetExtractedSize(selectedImage, FALSE);
        if (!m_liveInstall && m_imageFiles.Lookup(selectedImage, localEntry)) {
            size = localEntry->size;
        }
    } else {
        POSITION p = m_remoteImages.FindIndex(m_selectedRemoteIndex);
        if (p != NULL) {
            RemoteImageEntry_t remote = m_remoteImages.GetAt(p);
            size = m_liveInstall ? remote.extractedSize : remote.compressedSize;
        }
    }

    // add the installer size if this is not a live image
    if (!m_liveInstall) {
        size += INSTALLER_DELTA_SIZE + m_installerImage.extractedSize;
    }

    if (pElement != NULL) {
        CallJavascript(_T(JS_RESET_CHECK), CComVariant(_T(ELEMENT_SELUSB_AGREEMENT)));
        m_usbDeleteAgreement = false;
    }

    // RADU: should it be >= or should we take some more stuff into account like the partition size 
    BOOL isDiskBigEnough = (ULONGLONG)SelectedDrive.DiskSize > size;

    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_SELUSB_NEXT_BUTTON))), CComVariant(m_usbDeleteAgreement && isDiskBigEnough));
    CallJavascript(_T(JS_ENABLE_ELEMENT), CComVariant(_T(ELEMENT_SELUSB_USB_DRIVES)), CComVariant(TRUE));

    return S_OK;

error:
    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_SELUSB_NEXT_BUTTON))), CComVariant(FALSE));
    CallJavascript(_T(JS_ENABLE_ELEMENT), CComVariant(_T(ELEMENT_SELUSB_USB_DRIVES)), CComVariant(FALSE));
    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnAgreementCheckboxChanged(IHTMLElement *pElement)
{
    FUNCTION_ENTER;

    m_usbDeleteAgreement = !m_usbDeleteAgreement;
    OnSelectedUSBDiskChanged(NULL);
    
    return S_OK;
}


void CEndlessUsbToolDlg::LeavingDevicesPage()
{
    FUNCTION_ENTER;

    PF_INIT(SHChangeNotifyDeregister, Shell32);

    if (pfSHChangeNotifyDeregister != NULL && m_shellNotificationsRegister != 0) {
        pfSHChangeNotifyDeregister(m_shellNotificationsRegister);
        m_shellNotificationsRegister = 0;
    }
}

// Select Storage Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectStoragePreviousClicked(IHTMLElement* pElement)
{
	FUNCTION_ENTER;

	ChangePage(_T(ELEMENT_FILE_PAGE));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectStorageNextClicked(IHTMLElement *pElement)
{
	IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnSelectStorageNextClicked: Button is disabled. ", S_OK);

	FUNCTION_ENTER;

	CString selSpace;
	selSpace.Format(_T("%d Gb"), m_nrGigsSelected);
	Analytics::instance()->eventTracking(_T(ELEMENT_STORAGE_PAGE), _T("StorageSize"), selSpace);

	StartInstallationProcess();

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectedStorageSizeChanged(IHTMLElement* pElement)
{
	FUNCTION_ENTER;

	CComPtr<IHTMLSelectElement> selectElement;
	CComBSTR selectedValue;

	HRESULT hr = pElement->QueryInterface(IID_IHTMLSelectElement, (void**)&selectElement);
	IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectElement != NULL, "Error querying for IHTMLSelectElement interface", S_OK);

	hr = selectElement->get_value(&selectedValue);
	IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectElement != NULL, "Error getting selected file value", S_OK);

	m_nrGigsSelected = _wtoi(selectedValue);
	uprintf("Number of Gb selected for the endless OS file: %d", m_nrGigsSelected);

	return S_OK;
}

#define MIN_NO_OF_GIGS			8
#define MAX_NO_OF_GIGS			256
#define RECOMMENDED_GIGS_BASE	16
#define RECOMMENDED_GIGS_FULL	32
#define IS_MINIMUM_VALUE		1
#define IS_MAXIMUM_VALUE		2
#define IS_BASE_IMAGE			3

#define BYTES_IN_GIGABYTE		(1024 *  1024 * 1024)

void CEndlessUsbToolDlg::GoToSelectStoragePage()
{
	ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	CComPtr<IHTMLSelectElement> selectElement;
	HRESULT hr;
	int maxAvailableGigs = 0;
	CStringA freeSize, totalSize;
	bool isBaseImage = true;

	// figure out how much space we need
	ULONGLONG neededSize = 0;
	ULONGLONG bytesInGig = BYTES_IN_GIGABYTE;
	if (m_useLocalFile) {
		pFileImageEntry_t localEntry = NULL;
		CString selectedImage = UTF8ToCString(image_path);
		neededSize = GetExtractedSize(selectedImage, FALSE);

		if (!m_imageFiles.Lookup(selectedImage, localEntry)) {
			uprintf("ERROR: Selected local file not found.");
		} else {
			isBaseImage = (localEntry->personality == PERSONALITY_BASE);
		}
	} else {
		POSITION p = m_remoteImages.FindIndex(m_selectedRemoteIndex);
		if (p != NULL) {
			RemoteImageEntry_t remote = m_remoteImages.GetAt(p);
			neededSize = remote.extractedSize;
			isBaseImage = (remote.personality == PERSONALITY_BASE);
		}
	}
	neededSize += bytesInGig;
	int neededGigs = (int) (neededSize / bytesInGig);

	CStringW systemDrive = GetSystemDrive();
	CStringA systemDriveA = ConvertUnicodeToUTF8(systemDrive);

	// get available space on system drive
	IFFALSE_RETURN(GetDiskFreeSpaceEx(systemDrive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes) != 0, "Error on GetDiskFreeSpace");
	totalSize = SizeToHumanReadable(totalNumberOfBytes.QuadPart, FALSE, use_fake_units);
	freeSize = SizeToHumanReadable(freeBytesAvailable.QuadPart, FALSE, use_fake_units);
	uprintf("Available space on drive %s is %s out of %s; we need %s", systemDrive, freeSize, totalSize, SizeToHumanReadable(neededSize, FALSE, use_fake_units));
	maxAvailableGigs = (int) ((freeBytesAvailable.QuadPart - bytesInGig) / bytesInGig);

	bool enoughBytesAvailable = (freeBytesAvailable.QuadPart - bytesInGig) > neededSize;

	// Enable/disable ui elements based on space availability
	CallJavascript(_T(JS_ENABLE_ELEMENT), CComVariant(_T(ELEMENT_STORAGE_SELECT)), CComVariant(enoughBytesAvailable));

	// update messages with needed space based on selected version
	CStringA osVersion = lmprintf(isBaseImage ? MSG_400 : MSG_316);
	CStringA osSizeText = SizeToHumanReadable((isBaseImage ? RECOMMENDED_GIGS_BASE : RECOMMENDED_GIGS_FULL) * bytesInGig, FALSE, use_fake_units);
	CString message = UTF8ToCString(lmprintf(MSG_337, osVersion, osSizeText));
	SetElementText(_T(ELEMENT_STORAGE_DESCRIPTION), CComBSTR(message));

	message = UTF8ToCString(lmprintf(MSG_341, freeSize, totalSize, systemDriveA));
	SetElementText(_T(ELEMENT_STORAGE_AVAILABLE), CComBSTR(message));

	message = UTF8ToCString(lmprintf(MSG_342, systemDriveA));
	SetElementText(_T(ELEMENT_STORAGE_MESSAGE), CComBSTR(message));

	// Clear existing elements from the drop down
	hr = GetSelectElement(_T(ELEMENT_STORAGE_SELECT), selectElement);
	IFFALSE_RETURN(SUCCEEDED(hr) && selectElement != NULL, "Error returned from GetSelectElement.");
	hr = selectElement->put_length(0);

	if (!enoughBytesAvailable) {
		uprintf("Not enough bytes available.");
		ChangePage(_T(ELEMENT_STORAGE_PAGE));
		return;
	}

	// Add the entries
	m_nrGigsSelected = neededGigs;
	IFFALSE_RETURN(AddStorageEntryToSelect(selectElement, neededGigs, IS_MINIMUM_VALUE), "Error adding value to select.");

	for (int nrGigs = MIN_NO_OF_GIGS; nrGigs < min(maxAvailableGigs, MAX_NO_OF_GIGS); nrGigs = nrGigs * 2) {
		IFFALSE_CONTINUE(nrGigs > neededGigs, "");
		IFFALSE_RETURN(AddStorageEntryToSelect(selectElement, nrGigs, isBaseImage ? IS_BASE_IMAGE : 0), "Error adding value to select.");
	}

	IFFALSE_RETURN(AddStorageEntryToSelect(selectElement, maxAvailableGigs, IS_MAXIMUM_VALUE), "Error adding value to select.");

	ChangePage(_T(ELEMENT_STORAGE_PAGE));
}

BOOL CEndlessUsbToolDlg::AddStorageEntryToSelect(CComPtr<IHTMLSelectElement> &selectElement, int noOfGigs, uint8_t extraData)
{
	ULONGLONG size = noOfGigs;
	size = size * BYTES_IN_GIGABYTE;
	CStringA sizeText = SizeToHumanReadable(size, FALSE, use_fake_units);
	CString value, text;
	int locMsg = 0;
	value.Format(L"%d", noOfGigs);

	if (extraData == IS_MINIMUM_VALUE) locMsg = MSG_338;
	else if (extraData == IS_MAXIMUM_VALUE) locMsg = MSG_340;
	else if (extraData == IS_BASE_IMAGE && noOfGigs == RECOMMENDED_GIGS_BASE) locMsg = MSG_339;
	else if (extraData == 0 && noOfGigs == RECOMMENDED_GIGS_FULL) locMsg = MSG_339;

	if (locMsg != 0) {
		text = UTF8ToCString(lmprintf(locMsg, sizeText));
	} else {
		text = UTF8ToCString(sizeText);
	}

	if (locMsg == MSG_339) {
		m_nrGigsSelected = noOfGigs;
	}

	return SUCCEEDED(AddEntryToSelect(selectElement, CComBSTR(value), CComBSTR(text), NULL, locMsg == MSG_339));
}

// Install Page Handlers
HRESULT CEndlessUsbToolDlg::OnInstallCancelClicked(IHTMLElement* pElement)
{
    IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnInstallCancelClicked: Button is disabled. ", S_OK);

    FUNCTION_ENTER;

    if (!CancelInstall()) {
        return S_OK;
    }

    m_lastErrorCause = ErrorCause_t::ErrorCauseCanceled;
    CancelRunningOperation();

    return S_OK;
}

// Error/Thank You Page Handlers
HRESULT CEndlessUsbToolDlg::OnCloseAppClicked(IHTMLElement* pElement)
{
    Uninit();
    AfxPostQuitMessage(0);

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnRecoverErrorButtonClicked(IHTMLElement* pElement)
{
    ErrorCause_t errorCause = m_lastErrorCause;
    uprintf("OnRecoverErrorButtonClicked error cause %ls(%d)", ErrorCauseToStr(errorCause), errorCause);

    IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "Button is disabled. User didn't check the 'Delete invalid files' checkbox.", S_OK);

    // reset the state
    m_lastErrorCause = ErrorCause_t::ErrorCauseNone;
    m_localFilesScanned = false;
    m_jsonDownloadAttempted = false;
    ResetEvent(m_cancelOperationEvent);
    ResetEvent(m_closeFileScanThreadEvent);
    StartCheckInternetConnectionThread();
    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_INSTALL_CANCEL))), CComVariant(TRUE));

    // continue based on error cause
    switch (errorCause) {
    case ErrorCause_t::ErrorCauseDownloadFailed:
    case ErrorCause_t::ErrorCauseDownloadFailedDiskFull:
        StartInstallationProcess();
        break;
    case ErrorCause_t::ErrorCauseWriteFailed:
        if (!m_liveInstall) {
            safe_free(image_path);
            image_path = wchar_to_utf8(m_LiveFile);
        }
        OnSelectFileNextClicked(NULL);
        break;
    case ErrorCause_t::ErrorCauseVerificationFailed:
    {
        BOOL result = DeleteFile(m_localFile);
        uprintf("%s on deleting file '%ls' - %s", result ? "Success" : "Error", m_localFile, WindowsErrorString());
        SetLastError(ERROR_SUCCESS);
        result = DeleteFile(m_localFileSig);
        uprintf("%s on deleting file '%ls' - %s", result ? "Success" : "Error", m_localFileSig, WindowsErrorString());
        ChangePage(_T(ELEMENT_FIRST_PAGE));
        break;
    }
    case ErrorCause_t::ErrorCauseCanceled:
    case ErrorCause_t::ErrorCauseJSONDownloadFailed:
    default:
        ChangePage(_T(ELEMENT_DUALBOOT_PAGE));
        break;
    }

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnDeleteCheckboxChanged(IHTMLElement *pElement)
{
    FUNCTION_ENTER;
    CComPtr<IHTMLOptionButtonElement> checkboxElem;
    VARIANT_BOOL checked = VARIANT_FALSE;

    HRESULT hr = pElement->QueryInterface(&checkboxElem);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && checkboxElem != NULL, "Error querying for IHTMLOptionButtonElement.", S_OK);

    hr = checkboxElem->get_checked(&checked);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "Error querying for IHTMLOptionButtonElement.", S_OK);

    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_ERROR_BUTTON))), checked);

    return S_OK;
}

bool CEndlessUsbToolDlg::CancelInstall()
{
    bool operation_in_progress = m_currentStep == OP_FLASHING_DEVICE;

    uprintf("Cancel requested. Operation in progress: %s", operation_in_progress ? "YES" : "NO");
    if (operation_in_progress) {
        if (FormatStatus != FORMAT_STATUS_CANCEL) {
            int result = MessageBoxExU(hMainDialog, lmprintf(MSG_105), lmprintf(MSG_049), MB_YESNO | MB_ICONWARNING | MB_IS_RTL, selected_langid);
            if (result == IDYES) {
                uprintf("Cancel operation confirmed.");
                CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_INSTALL_CANCEL))), CComVariant(FALSE));
                FormatStatus = FORMAT_STATUS_CANCEL;
				m_cancelImageUnpack = 1;
                m_lastErrorCause = ErrorCause_t::ErrorCauseCanceled;
                uprintf("Cancelling current operation.");
                CString str = UTF8ToCString(lmprintf(MSG_201));
                SetElementText(_T(ELEMENT_INSTALL_STATUS), CComBSTR(""));
            }
        }
    }

    return !operation_in_progress;
}

DownloadType_t CEndlessUsbToolDlg::GetSelectedDownloadType()
{
	if (m_dualBootSelected) {
		return DownloadType_t::DownloadTypeDualBootFiles;
	} else {
		return m_liveInstall ? DownloadType_t::DownloadTypeLiveImage : DownloadType_t::DownloadTypeInstallerImage;
	}
}

void CEndlessUsbToolDlg::OnClose()
{
    FUNCTION_ENTER;

    if (!CancelInstall()) {
        m_closeRequested = true;
        return;
    }

    Uninit();
    CDHtmlDialog::OnClose();
}

HRESULT CEndlessUsbToolDlg::CallJavascript(LPCTSTR method, CComVariant parameter1, CComVariant parameter2)
{
    HRESULT hr;
    //uprintf("CallJavascript called with method %ls", method);
    if (m_spWindowElem == NULL) {
        hr = m_spHtmlDoc->get_parentWindow(&m_spWindowElem);
        IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && m_spWindowElem != NULL, "Error querying for parent window.", E_FAIL);
    }
    if (m_dispWindow == NULL) {
        hr = m_spWindowElem->QueryInterface(&m_dispWindow);
        IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && m_dispWindow != NULL, "Error querying for CComDispatchDriver.", E_FAIL);
    }
    if (m_dispexWindow == NULL) {
        hr = m_spWindowElem->QueryInterface(&m_dispexWindow);
        IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && m_dispexWindow != NULL, "Error querying for IDispatchEx.", E_FAIL);
    }

    DISPID dispidMethod = -1;
    hr = m_dispexWindow->GetDispID(CComBSTR(method), fdexNameCaseSensitive, &dispidMethod);
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "Error getting method dispid", E_FAIL);
    if (parameter2 == CComVariant()) {
        hr = m_dispWindow.Invoke1(dispidMethod, &parameter1);
    } else {
        hr = m_dispWindow.Invoke2(dispidMethod, &parameter1, &parameter2);
    }

    IFFALSE_RETURN_VALUE(SUCCEEDED(hr), "Error when calling method.", E_FAIL);

    return S_OK;
}
void CEndlessUsbToolDlg::UpdateCurrentStep(int currentStep)
{
    if (m_currentStep == currentStep) {
        uprintf("Already at step %ls(%d)", OperationToStr(currentStep), currentStep);
        return;
    }

    FUNCTION_ENTER;

    int nrSteps = m_useLocalFile ? 2 : 3;
    int nrCurrentStep;
    int locMsgIdTitle;
    m_currentStep = currentStep;
    
    switch (m_currentStep)
    {
    case OP_DOWNLOADING_FILES:
        locMsgIdTitle = MSG_304;
        nrCurrentStep = 1;
        break;
    case OP_VERIFYING_SIGNATURE:
        locMsgIdTitle = MSG_310;
        nrCurrentStep = m_useLocalFile ? 1 : 2;
        break;
    case OP_FLASHING_DEVICE:
    case OP_FILE_COPY:
		locMsgIdTitle = MSG_311;
		nrCurrentStep = m_useLocalFile ? 2 : 3;
		break;
	case OP_SETUP_DUALBOOT:
        locMsgIdTitle = MSG_309;
        nrCurrentStep = m_useLocalFile ? 2 : 3;
        break;
    default:
        uprintf("Unknown operation %ls(%d)", OperationToStr(currentStep), currentStep);
        break;
    }
    
	CString str;

    str = UTF8ToCString(lmprintf(MSG_305, nrCurrentStep));
    SetElementText(_T(ELEMENT_INSTALL_STEP), CComBSTR(str));

    str = UTF8ToCString(lmprintf(MSG_306, nrSteps));
    SetElementText(_T(ELEMENT_INSTALL_STEPS_TOTAL), CComBSTR(str));  

    str = UTF8ToCString(lmprintf(locMsgIdTitle));
    SetElementText(_T(ELEMENT_INSTALL_STEP_TEXT), CComBSTR(str));

    CallJavascript(_T(JS_SET_PROGRESS), CComVariant(0));
    SetElementText(_T(ELEMENT_INSTALL_STATUS), CComBSTR(""));
}

bool CEndlessUsbToolDlg::FileHashingCallback(__int64 currentSize, __int64 totalSize, LPVOID context)
{
    // RADU: do param verification
    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg *)context;    
    DWORD dwWaitStatus = WaitForSingleObject((HANDLE)dlg->m_cancelOperationEvent, 0);
    if(dwWaitStatus == WAIT_OBJECT_0) {
        uprintf("FileHashingCallback: Cancel requested.");
        return false;
    }
    
    float current = (float)(currentSize * 100 / totalSize);
    UpdateProgress(OP_VERIFYING_SIGNATURE, current);

    return true;
}

DWORD WINAPI CEndlessUsbToolDlg::FileVerificationThread(void* param)
{
    FUNCTION_ENTER;

    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*) param;
    CString filename = dlg->m_localFile;
    CString signatureFilename = dlg->m_localFileSig;
    BOOL verificationResult = FALSE;    

    signature_packet_t p_sig = { 0 };
    public_key_t p_pkey = { 0 };
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTHASH hHash = NULL;

    uprintf("Verifying file '%ls' with signature '%ls'", dlg->m_localFile, dlg->m_localFileSig);

    /*verificationResult = TRUE;
    goto error;*/

    IFFALSE_GOTOERROR(0 == LoadSignature(signatureFilename, &p_sig), "Error on LoadSignature");
    IFFALSE_GOTOERROR(0 == parse_public_key(endless_public_key, sizeof(endless_public_key), &p_pkey, nullptr), "Error on parse_public_key");
    IFFALSE_GOTOERROR(CryptAcquireContext(&hCryptProv, nullptr, nullptr, map_algo(p_pkey.key.algo), CRYPT_VERIFYCONTEXT), "Error on CryptAcquireContext");

    memcpy(p_pkey.longid, endless_public_key_longid, sizeof(endless_public_key_longid));
    IFFALSE_GOTOERROR(0 == memcmp(p_sig.issuer_longid, p_pkey.longid, 8), "Error: signature key id differs from Endless key id");

    IFFALSE_GOTOERROR(CryptCreateHash(hCryptProv, map_digestalgo(p_sig.digest_algo), 0, 0, &hHash), "Error on CryptCreateHash");

    IFFALSE_GOTOERROR(0 == hash_from_file(hHash, filename, &p_sig, FileHashingCallback, dlg), "Error on hash_from_file");
    IFFALSE_GOTOERROR(0 == check_hash(hHash, &p_sig), "Error on check_hash");
    IFFALSE_GOTOERROR(0 == verify_signature(hCryptProv, hHash, p_pkey, p_sig), "Error on verify_signature");

    verificationResult = TRUE;

error:
    ::PostMessage(hMainDialog, WM_FINISHED_FILE_VERIFICATION, (WPARAM)verificationResult, 0);

    // cleanup wincrypto
    if(hHash != NULL) CryptDestroyHash(hHash);
    if(hCryptProv != NULL) CryptReleaseContext(hCryptProv, 0);

    // cleanup key
    free(p_pkey.psz_username);
    // cleanup signature
    if (p_sig.version == 4)
    {
        free(p_sig.specific.v4.hashed_data);
        free(p_sig.specific.v4.unhashed_data);
    }

    return 0;
}

#if !defined(PARTITION_BASIC_DATA_GUID)
const GUID PARTITION_BASIC_DATA_GUID =
{ 0xebd0a0a2L, 0xb9e5, 0x4433,{ 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };
#endif
#if !defined(PARTITION_SYSTEM_GUID)
const GUID PARTITION_SYSTEM_GUID =
{ 0xc12a7328L, 0xf81f, 0x11d2,{ 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b } };
#endif
#if !defined(PARTITION_BIOS_BOOT_GUID)
const GUID PARTITION_BIOS_BOOT_GUID =
{ 0x21686148L, 0x6449, 0x6e6f, { 0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 } };
#endif

#define EXPECTED_NUMBER_OF_PARTITIONS	3
#define EXFAT_PARTITION_NAME_IMAGES		L"eosimages"
#define EXFAT_PARTITION_NAME_LIVE		L"eoslive"

DWORD WINAPI CEndlessUsbToolDlg::FileCopyThread(void* param)
{
    FUNCTION_ENTER;

    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*)param;
    
    HANDLE hPhysical = INVALID_HANDLE_VALUE;
    DWORD size;
    DWORD DriveIndex = SelectedDrive.DeviceNumber;
    BOOL result;

    BYTE geometry[256] = { 0 }, layout[4096] = { 0 };
    PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)(void*)layout;
    CString driveDestination, fileDestination;
    CStringA iniLanguage = INI_LOCALE_EN;

	// Radu: why do we do this?
    memset(&SelectedDrive, 0, sizeof(SelectedDrive));

    // Query for disk and partition data
    hPhysical = GetPhysicalHandle(DriveIndex, TRUE, TRUE);
    IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");
    
    result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, geometry, sizeof(geometry), &size, NULL);
    IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk geometry.");

    result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, layout, sizeof(layout), &size, NULL);
    IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk layout.");
    
    IFFALSE_GOTOERROR(DriveLayout->PartitionStyle == PARTITION_STYLE_GPT, "Unexpected partition type.");
    IFFALSE_GOTOERROR(DriveLayout->PartitionCount == EXPECTED_NUMBER_OF_PARTITIONS, "Error: Unexpected number of partitions.");

    //uprintf("Partition type: GPT, NB Partitions: %d\n", DriveLayout->PartitionCount);
    //uprintf("Disk GUID: %s\n", GuidToString(&DriveLayout->Gpt.DiskId));
    //uprintf("Max parts: %d, Start Offset: %I64i, Usable = %I64i bytes\n",
    //    DriveLayout->Gpt.MaxPartitionCount, DriveLayout->Gpt.StartingUsableOffset.QuadPart, DriveLayout->Gpt.UsableLength.QuadPart);

    // Move all partitions by one so we can add the exfat to the beginning of the partition table
    for (int index = EXPECTED_NUMBER_OF_PARTITIONS; index > 0 ; index--) {
        memcpy(&(DriveLayout->PartitionEntry[index]), &(DriveLayout->PartitionEntry[index - 1]), sizeof(PARTITION_INFORMATION_EX));
        DriveLayout->PartitionEntry[index].PartitionNumber = index + 1;
    }

    // Create the partition to occupy available disk space
    PARTITION_INFORMATION_EX *lastPartition = &(DriveLayout->PartitionEntry[DriveLayout->PartitionCount]);
    PARTITION_INFORMATION_EX *newPartition = &(DriveLayout->PartitionEntry[0]);
    newPartition->PartitionStyle = PARTITION_STYLE_GPT;
    newPartition->PartitionNumber = 1;;
    newPartition->StartingOffset.QuadPart = lastPartition->StartingOffset.QuadPart + lastPartition->PartitionLength.QuadPart;    
    newPartition->PartitionLength.QuadPart = DiskGeometry->DiskSize.QuadPart - newPartition->StartingOffset.QuadPart; //newPartition->PartitionLength.QuadPart = DriveLayout->Gpt.UsableLength.QuadPart - newPartition->StartingOffset.QuadPart;

    newPartition->Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;
    IGNORE_RETVAL(CoCreateGuid(&newPartition->Gpt.PartitionId));
    wcscpy(newPartition->Gpt.Name, EXFAT_PARTITION_NAME_IMAGES);

    DriveLayout->PartitionCount += 1;

    size = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + DriveLayout->PartitionCount * sizeof(PARTITION_INFORMATION_EX);
    IFFALSE_GOTOERROR(DeviceIoControl(hPhysical, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, layout, size, NULL, 0, &size, NULL), "Could not set drive layout.");
    result = RefreshDriveLayout(hPhysical);
    safe_closehandle(hPhysical);

    // Check if user canceled
    IFFALSE_GOTOERROR(WaitForSingleObject(dlg->m_cancelOperationEvent, 0) == WAIT_TIMEOUT, "User cancel.");
	// Format the partition
	IFFALSE_GOTOERROR(FormatFirstPartitionOnDrive(DriveIndex, FS_EXFAT, dlg->m_cancelOperationEvent, EXFAT_PARTITION_NAME_IMAGES), "Error on FormatFirstPartitionOnDrive");
    // Mount it
	IFFALSE_GOTOERROR(MountFirstPartitionOnDrive(DriveIndex, driveDestination), "Error on MountFirstPartitionOnDrive");

    // Copy Files
    fileDestination = driveDestination + CSTRING_GET_LAST(dlg->m_LiveFile, L'\\');
    result = CopyFileEx(dlg->m_LiveFile, fileDestination, CEndlessUsbToolDlg::CopyProgressRoutine, dlg, NULL, 0);
    IFFALSE_GOTOERROR(result, "Copying live image failed/canceled.");

    fileDestination = driveDestination + CSTRING_GET_LAST(dlg->m_LiveFileSig, L'\\');
    result = CopyFileEx(dlg->m_LiveFileSig, fileDestination, NULL, NULL, NULL, 0);
    IFFALSE_GOTOERROR(result, "Copying live image signature failed.");

    // Create settings file
    if (!m_localeToIniLocale.Lookup(dlg->m_selectedLocale->txt[0], iniLanguage)) {
        uprintf("ERROR: Selected language not found %s. Defaulting to English", dlg->m_selectedLocale->txt[0]);
    }
    fileDestination = driveDestination + L"install.ini";
    FILE *iniFile;
    if (0 == _wfopen_s(&iniFile, fileDestination, L"w")) {
        CStringA line = "[EndlessOS]\n";
        fwrite((const char*)line, 1, line.GetLength(), iniFile);
        line.Format("locale=%s\n", iniLanguage);
        fwrite((const char*)line, 1, line.GetLength(), iniFile);
        fclose(iniFile);
    } else {
        uprintf("Could not open settings file %ls", fileDestination);
    }

    // Unmount
    if (!DeleteVolumeMountPoint(driveDestination)) {
        uprintf("Failed to unmount volume: %s", WindowsErrorString());
    }

    FormatStatus = 0;
    safe_closehandle(hPhysical);
    dlg->PostMessage(WM_FINISHED_FILE_COPY, 0, 0);
    return 0;

error:
    safe_closehandle(hPhysical);

    uprintf("FileCopyThread exited with error.");
    if (dlg->m_lastErrorCause == ErrorCause_t::ErrorCauseNone) {
        dlg->m_lastErrorCause = ErrorCause_t::ErrorCauseWriteFailed;
    }
    dlg->PostMessage(WM_FINISHED_FILE_COPY, 0, 0);
    return 0;
}

DWORD CALLBACK CEndlessUsbToolDlg::CopyProgressRoutine(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD         dwStreamNumber,
    DWORD         dwCallbackReason,
    HANDLE        hSourceFile,
    HANDLE        hDestinationFile,
    LPVOID        lpData
)
{
    //FUNCTION_ENTER;

    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*)lpData;

    DWORD dwWaitStatus = WaitForSingleObject((HANDLE)dlg->m_cancelOperationEvent, 0);
    if (dwWaitStatus == WAIT_OBJECT_0) {
        uprintf("CopyProgressRoutine: Cancel requested.");
        return PROGRESS_CANCEL;
    }

    float current = (float)(TotalBytesTransferred.QuadPart * 100 / TotalFileSize.QuadPart);
    UpdateProgress(OP_FILE_COPY, current);

    return PROGRESS_CONTINUE;
}



bool CEndlessUsbToolDlg::ParseImgFileName(const CString& filename, CString &personality, CString &version, bool &installerImage)
{
    FUNCTION_ENTER;

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
        case 4: lastPart = resToken; break;
        }
        resToken = filename.Tokenize(t1, pos);
        elemIndex++;
    };

    version.Replace(_T(EOS_PRODUCT_TEXT), _T(""));
    IFFALSE_GOTOERROR(!version.IsEmpty() && !lastPart.IsEmpty(), "");
    installerImage = product == _T(EOS_INSTALLER_PRODUCT_TEXT);
    IFFALSE_GOTOERROR(product == _T(EOS_PRODUCT_TEXT) || product == _T(EOS_NONFREE_PRODUCT_TEXT) || product == _T(EOS_OEM_PRODUCT_TEXT) || installerImage, "");

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
    uint32_t msgId;
    IFFALSE_GOTOERROR(!personality.IsEmpty() && m_personalityToLocaleMsg.Lookup(personality, msgId), "");

    return true;
error:
    return false;
}

void CEndlessUsbToolDlg::GetImgDisplayName(CString &displayName, const CString &version, const CString &personality, ULONGLONG size)
{
    FUNCTION_ENTER;

    ULONGLONG actualsize = m_liveInstall || m_dualBootSelected ? size : (size + m_installerImage.compressedSize);
    // Create display name
    displayName = _T(ENDLESS_OS);
    displayName += " ";
    displayName += version;
    displayName += " ";
    displayName += UTF8ToCString(lmprintf(m_personalityToLocaleMsg[personality]));
    if (personality != PERSONALITY_BASE) {
        displayName += " ";
        displayName += UTF8ToCString(lmprintf(MSG_316));
    }
    if (size != 0) {
        displayName += " - ";
        displayName += SizeToHumanReadable(actualsize, FALSE, use_fake_units);
    }
}

ULONGLONG CEndlessUsbToolDlg::GetExtractedSize(const CString& filename, BOOL isInstallerImage)
{
    FUNCTION_ENTER;

    CString ext = CSTRING_GET_LAST(filename, '.');
    int compression_type;
    if (ext == "gz") compression_type = BLED_COMPRESSION_GZIP;
    else if (ext == "xz") compression_type = BLED_COMPRESSION_XZ;
    else return 0;

    CStringA asciiFileName = ConvertUnicodeToUTF8(filename);
    return get_eos_archive_disk_image_size(asciiFileName, compression_type, isInstallerImage);
}

void CEndlessUsbToolDlg::GetIEVersion()
{
    FUNCTION_ENTER;

    CRegKey registryKey;
    LSTATUS result;
    wchar_t versionValue[256];
    ULONG size = sizeof(versionValue) / sizeof(versionValue[0]);

    result = registryKey.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Internet Explorer", KEY_QUERY_VALUE);
    IFFALSE_RETURN(result == ERROR_SUCCESS, "Error opening IE registry key.");

    result = registryKey.QueryStringValue(L"Version", versionValue, &size);
    IFFALSE_RETURN(result == ERROR_SUCCESS, "Error Querying for version value.");

    uprintf("%ls", versionValue);

    CString version = versionValue;
    version = version.Left(version.Find(L'.'));
    m_ieVersion = _wtoi(version);
}


DWORD WINAPI CEndlessUsbToolDlg::UpdateDownloadProgressThread(void* param)
{
    FUNCTION_ENTER;

    CComPtr<IBackgroundCopyManager> bcManager;
    CComPtr<IBackgroundCopyJob> currentJob;
    DownloadStatus_t *downloadStatus = NULL;
    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*)param;
    RemoteImageEntry_t remote;
    DownloadType_t downloadType = dlg->GetSelectedDownloadType();
    POSITION pos = dlg->m_remoteImages.FindIndex(dlg->m_selectedRemoteIndex);
    CString jobName;

    IFFALSE_RETURN_VALUE(pos != NULL, "Index value not valid.", 0);
    remote = dlg->m_remoteImages.GetAt(pos);

    // Specify the appropriate COM threading model for your application.
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IFFALSE_GOTO(SUCCEEDED(hr), "Error on calling CoInitializeEx", done);

    hr = CoCreateInstance(__uuidof(BackgroundCopyManager), NULL, CLSCTX_LOCAL_SERVER, __uuidof(IBackgroundCopyManager), (void**)&bcManager);
    IFFALSE_GOTO(SUCCEEDED(hr), "Error creating instance of BackgroundCopyManager.", done);

    jobName = DownloadManager::GetJobName(downloadType);
    jobName += remote.downloadJobName;
    IFFALSE_GOTO(SUCCEEDED(DownloadManager::GetExistingJob(bcManager, jobName, currentJob)), "No download job found", done);

    while (TRUE)
    {
        DWORD dwStatus = WaitForSingleObject(dlg->m_cancelOperationEvent, UPDATE_DOWNLOAD_PROGRESS_TIME);
        switch (dwStatus) {
            case WAIT_OBJECT_0:
            {
                uprintf("CEndlessUsbToolDlg::UpdateDownloadProgressThread cancel requested");
                goto done;
                break;
            }
            case WAIT_TIMEOUT:
            {
                downloadStatus = new DownloadStatus_t;
                downloadStatus->done = false;
                downloadStatus->error = false;
                bool result = dlg->m_downloadManager.GetDownloadProgress(currentJob, downloadStatus, jobName);
                if (!result || downloadStatus->done || downloadStatus->error) {
                    uprintf("CEndlessUsbToolDlg::UpdateDownloadProgressThread - Exiting");
                    delete downloadStatus;
                    goto done;
                }
                ::PostMessage(dlg->m_hWnd, WM_FILE_DOWNLOAD_STATUS, (WPARAM)downloadStatus, 0);
                break;
            }
        }
    }

done:

    CoUninitialize();
    return 0;
}

DWORD WINAPI CEndlessUsbToolDlg::CheckInternetConnectionThread(void* param)
{
    FUNCTION_ENTER;

    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*)param;
    DWORD flags;
    BOOL result = FALSE, connected = FALSE, firstTime = TRUE;

    while (TRUE) {
        DWORD dwStatus = WaitForSingleObject(dlg->m_cancelOperationEvent, firstTime ? 0 : CHECK_INTERNET_CONNECTION_TIME);
        switch (dwStatus) {
            case WAIT_OBJECT_0:
            {
                uprintf("CEndlessUsbToolDlg::CheckInternetConnectionThread cancel requested");
                goto done;
            }
            case WAIT_TIMEOUT:
            {
                flags = 0;
                result = InternetGetConnectedState(&flags, 0);
                IFFALSE_BREAK(result, "Device not connected to internet.");

                result = InternetCheckConnection(JSON_URL(JSON_LIVE_FILE), FLAG_ICC_FORCE_CONNECTION, 0);
                IFFALSE_BREAK(result, "Cannot connect to server hosting the JSON file.");

                result = TRUE;
                break;
            }
        }

        if (firstTime || result != connected) {
            firstTime = FALSE;
            connected = result;
            ::PostMessage(dlg->m_hWnd, WM_INTERNET_CONNECTION_STATE, (WPARAM)connected, 0);
        }
    }

done:
    dlg->m_checkConnectionThread = INVALID_HANDLE_VALUE;
    return 0;
}

void CEndlessUsbToolDlg::InitLogging()
{
    TCHAR *path = m_appDir.GetBufferSetLength(MAX_PATH + 1);
    // Retrieve the current application directory
    if (GetModuleFileName(NULL, path, MAX_PATH) == 0) {
        uprintf("Could not get current directory    : %s", WindowsErrorString());
        app_dir[0] = 0;
        m_appDir = _T("");
    } else {
        m_appDir = CSTRING_GET_PATH(m_appDir, _T('\\'));
        strcpy_s(app_dir, sizeof(app_dir) - 1, ConvertUnicodeToUTF8(m_appDir));
        app_dir[sizeof(app_dir) - 1] = 0;
    }
    m_appDir.ReleaseBuffer();

    // Set the Windows version
    GetWindowsVersion();

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
        } else {
            hLog = m_hWnd;
        }

        uprintf("Log original date time %ls\n", s);
        uprintf("Application version: %s\n", RELEASE_VER_STR);
        uprintf("Windows version: %s\n", WindowsVersionStr);
        uprintf("Windows version number: 0x%X\n", nWindowsVersion);
        uprintf("-----------------------------------\n", s);
    } catch (CFileException *ex) {
        m_enableLogDebugging = false;
        uprintf("CFileException on file [%ls] with cause [%d] and OS error [%d]", fileName, ex->m_cause, ex->m_lOsError);
        ex->Delete();
    }
}

void CEndlessUsbToolDlg::EnableHibernate(bool enable)
{
    FUNCTION_ENTER;

    EXECUTION_STATE flags;

    if (!enable) {
        if (nWindowsVersion == WINDOWS_XP) {
            flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
        } else {
            flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED;
        }
    } else {
        flags = ES_CONTINUOUS;
    }

    SetThreadExecutionState(flags);
}

void CEndlessUsbToolDlg::CancelRunningOperation()
{
    FUNCTION_ENTER;

    SetEvent(m_cancelOperationEvent);
	m_cancelImageUnpack = 1;

    FormatStatus = FORMAT_STATUS_CANCEL;
    if (m_currentStep != OP_FLASHING_DEVICE) {
        m_downloadManager.ClearExtraDownloadJobs();
        PostMessage(WM_FINISHED_ALL_OPERATIONS, (WPARAM)FALSE, 0);
    }
}

void CEndlessUsbToolDlg::StartCheckInternetConnectionThread()
{
    if (m_checkConnectionThread == INVALID_HANDLE_VALUE) {
        m_checkConnectionThread = CreateThread(NULL, 0, CEndlessUsbToolDlg::CheckInternetConnectionThread, (LPVOID)this, 0, NULL);
    }
}

bool CEndlessUsbToolDlg::CanUseLocalFile()
{
    //RADU: check if installer version matches local images version and display only the images that match?
    bool hasLocalInstaller = m_liveInstall || (m_localInstallerImage.stillPresent == TRUE);
    bool hasLocalImages = (m_imageFiles.GetCount() != 0);

	// If we have a local entry with all needed files
	bool hasFilesForDualBoot = false;
	if (m_dualBootSelected && hasLocalImages) {
		pFileImageEntry_t currentEntry = NULL;
		CString path;
		for (POSITION position = m_imageFiles.GetStartPosition(); position != NULL; ) {
			m_imageFiles.GetNextAssoc(position, path, currentEntry);
			if (currentEntry->hasBootArchive && currentEntry->hasBootArchiveSig) {
				hasFilesForDualBoot = true;
				break;
			}
		}
	}

    return !m_localFilesScanned || (hasLocalInstaller && hasLocalImages) || hasFilesForDualBoot;
}

bool CEndlessUsbToolDlg::CanUseRemoteFile()
{
    return !m_jsonDownloadAttempted || m_remoteImages.GetCount() != 0;
}

void CEndlessUsbToolDlg::FindMaxUSBSpeed()
{
    SP_DEVICE_INTERFACE_DATA         DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA                  DevData;
    HDEVINFO hDevInfo;
    DWORD dwMemberIdx, dwSize;

    m_maximumUSBVersion = USB_SPEED_UNKNOWN;

    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_HUB, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    IFFALSE_RETURN(hDevInfo != INVALID_HANDLE_VALUE, "Error on SetupDiGetClassDevs call");

    DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dwMemberIdx = 0;
    SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_HUB, dwMemberIdx, &DevIntfData);

    while (GetLastError() != ERROR_NO_MORE_ITEMS) {
        DevData.cbSize = sizeof(DevData);
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

        DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
        DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, DevIntfDetailData, dwSize, &dwSize, &DevData)) {
            CheckUSBHub(DevIntfDetailData->DevicePath);
        }

        HeapFree(GetProcessHeap(), 0, DevIntfDetailData);
        SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_HUB, ++dwMemberIdx, &DevIntfData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}


void CEndlessUsbToolDlg::CheckUSBHub(LPCTSTR devicePath)
{
    HANDLE usbHubHandle = CreateFile(devicePath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (usbHubHandle != INVALID_HANDLE_VALUE) {
        USB_HUB_CAPABILITIES hubCapabilities;
        DWORD size = sizeof(hubCapabilities);
        memset(&hubCapabilities, 0, sizeof(USB_HUB_CAPABILITIES));

        // Windows XP check
        if (nWindowsVersion == WINDOWS_XP) {
            if (!DeviceIoControl(usbHubHandle, IOCTL_USB_GET_HUB_CAPABILITIES, &hubCapabilities, size, &hubCapabilities, size, &size, NULL)) {
                uprintf("Could not get hub capabilites for %ls: %s", devicePath, WindowsErrorString());
            } else {
                uprintf("%ls HubIs2xCapable=%d", devicePath, hubCapabilities.HubIs2xCapable);
                m_maximumUSBVersion = max(m_maximumUSBVersion, hubCapabilities.HubIs2xCapable ? USB_SPEED_HIGH : USB_SPEED_LOW);
            }
        }

        // Windows Vista and 7 check
        if (nWindowsVersion >= WINDOWS_VISTA) {
            USB_HUB_CAPABILITIES_EX hubCapabilitiesEx;
            size = sizeof(hubCapabilitiesEx);
            memset(&hubCapabilitiesEx, 0, sizeof(USB_HUB_CAPABILITIES_EX));
            if (!DeviceIoControl(usbHubHandle, IOCTL_USB_GET_HUB_CAPABILITIES_EX, &hubCapabilitiesEx, size, &hubCapabilitiesEx, size, &size, NULL)) {
                uprintf("Could not get hub capabilites for %ls: %s", devicePath, WindowsErrorString());
            } else {
                uprintf("Vista+ %ls HubIsHighSpeedCapable=%d, HubIsHighSpeed=%d",
                    devicePath, hubCapabilitiesEx.CapabilityFlags.HubIsHighSpeedCapable, hubCapabilitiesEx.CapabilityFlags.HubIsHighSpeed);
                m_maximumUSBVersion = max(m_maximumUSBVersion, hubCapabilitiesEx.CapabilityFlags.HubIsHighSpeed ? USB_SPEED_HIGH : USB_SPEED_LOW);
            }
        }

        // Windows 8 and later check
        if (nWindowsVersion >= WINDOWS_8) {
            USB_HUB_INFORMATION_EX hubInformation;
            size = sizeof(hubInformation);
            memset(&hubInformation, 0, sizeof(USB_HUB_INFORMATION_EX));
            if (!DeviceIoControl(usbHubHandle, IOCTL_USB_GET_HUB_INFORMATION_EX, &hubInformation, size, &hubInformation, size, &size, NULL)) {
                uprintf("Could not get hub capabilites for %ls: %s", devicePath, WindowsErrorString());
            } else {
                USB_NODE_CONNECTION_INFORMATION_EX_V2 conn_info_v2;
                for (int index = 1; index <= hubInformation.HighestPortNumber; index++) {
                    memset(&conn_info_v2, 0, sizeof(conn_info_v2));
                    size = sizeof(conn_info_v2);
                    conn_info_v2.ConnectionIndex = (ULONG)index;
                    conn_info_v2.Length = size;
                    conn_info_v2.SupportedUsbProtocols.Usb300 = 1;
                    if (!DeviceIoControl(usbHubHandle, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX_V2, &conn_info_v2, size, &conn_info_v2, size, &size, NULL)) {
                        uprintf("Could not get node connection information (V2) for hub '%ls' with port '%d': %s", devicePath, index, WindowsErrorString());
                        continue;
                    }
                    uprintf("%d) USB 3.0(%d), 2.0(%d), 1.0(%d), OperatingAtSuperSpeedOrHigher (%d), SuperSpeedCapableOrHigher (%d)", index,
                        conn_info_v2.SupportedUsbProtocols.Usb300, conn_info_v2.SupportedUsbProtocols.Usb200, conn_info_v2.SupportedUsbProtocols.Usb110,
                        conn_info_v2.Flags.DeviceIsOperatingAtSuperSpeedOrHigher, conn_info_v2.Flags.DeviceIsSuperSpeedCapableOrHigher);
                    m_maximumUSBVersion = max(m_maximumUSBVersion, conn_info_v2.SupportedUsbProtocols.Usb300 == 1 ? USB_SPEED_SUPER_OR_LATER : USB_SPEED_HIGH);
                }
            }
        }

    } else {
        uprintf("Could not open hub %ls: %s", devicePath, WindowsErrorString());
    }

    safe_closehandle(usbHubHandle);
}

void CEndlessUsbToolDlg::UpdateUSBSpeedMessage(DWORD deviceIndex)
{
    DWORD speed = usbDeviceSpeed[deviceIndex];
    BOOL isLowerSpeed = usbDeviceSpeedIsLower[deviceIndex];
    DWORD msgId = 0;

    if (speed < USB_SPEED_HIGH) { // smaller than USB 2.0
        if (m_maximumUSBVersion == USB_SPEED_SUPER_OR_LATER) { // we have USB 3.0 ports
            msgId = MSG_330;
        } else { // we have USB 2.0 ports
            msgId = MSG_331;
        }
    } else if (speed == USB_SPEED_HIGH && m_maximumUSBVersion == USB_SPEED_SUPER_OR_LATER) { // USB 2.0 device (or USB 3.0 device in USB 2.0 port) and we have USB 3.0 ports
        msgId = isLowerSpeed ? MSG_333 : MSG_332;
    }

    if (msgId == 0) {
        SetElementText(_T(ELEMENT_SELUSB_SPEEDWARNING), CComBSTR(""));
    } else {
        SetElementText(_T(ELEMENT_SELUSB_SPEEDWARNING), UTF8ToBSTR(lmprintf(msgId)));
    }
}

void CEndlessUsbToolDlg::JSONDownloadFailed()
{
    m_jsonDownloadAttempted = true;
    if (!CanUseLocalFile()) {
        uprintf("Going to error page as no local files were found either.");
        m_lastErrorCause = ErrorCause_t::ErrorCauseJSONDownloadFailed;
        CancelRunningOperation();
    }
}

#define GPT_MAX_PARTITION_COUNT		128
#define ESP_PART_STARTING_SECTOR	2048
#define ESP_PART_LENGTH_BYTES		65011712
#define MBR_PART_STARTING_SECTOR	129024
#define MBR_PART_LENGTH_BYTES		1048576
#define EXFAT_PART_STARTING_SECTOR	131072

#define EFI_BOOT_SUBDIRECTORY			L"EFI\\BOOT"
#define ENDLESS_BOOT_SUBDIRECTORY		L"EFI\\Endless"
#define PATH_ENDLESS_SUBDIRECTORY		L"endless\\"
#define ENDLESS_IMG_FILE_NAME			L"endless.img"
#define EXFAT_ENDLESS_LIVE_FILE_NAME	L"live"
#define GRUB_BOOT_SUBDIRECTORY			L"grub"
#define LIVE_BOOT_IMG_FILE				L"live\\boot.img"
#define LIVE_CORE_IMG_FILE				L"live\\core.img"

/* The offset of a magic number used by Windows NT.  */
#define MBR_WINDOWS_NT_MAGIC 0x1b8

/* The offset of the start of the partition table.  */
#define MBR_PART_START    0x1be

/* The size of boot.img produced by the Endless OS image builder */
#define MAX_BOOT_IMG_FILE_SIZE			MBR_PART_START
#define NTFS_BOOT_IMG_FILE				L"ntfs\\boot.img"
#define NTFS_CORE_IMG_FILE				L"ntfs\\core.img"
#define ENDLESS_BOOT_EFI_FILE			"bootx64.efi"
#define BACKUP_BOOTTRACK_IMG			L"boottrack.img"

/* The offset of BOOT_DRIVE_CHECK.  */
#define GRUB_BOOT_MACHINE_DRIVE_CHECK	0x66

#define CHECK_IF_CANCELED IFFALSE_GOTOERROR(dlg->m_cancelImageUnpack == 0 && WaitForSingleObject((HANDLE)dlg->m_cancelOperationEvent, 0) != WAIT_OBJECT_0, "Operation has been canceled")

DWORD WINAPI CEndlessUsbToolDlg::CreateUSBStick(LPVOID param)
{
	FUNCTION_ENTER;

	CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*)param;
	DWORD DriveIndex = SelectedDrive.DeviceNumber;
	BOOL result;
	DWORD size;
	HANDLE hPhysical = INVALID_HANDLE_VALUE;
	BYTE geometry[256] = { 0 }, layout[4096] = { 0 };
	CREATE_DISK createDiskData;
	CString driveLetter;
	CString bootFilesPathGz = GET_LOCAL_PATH(dlg->m_bootArchive);
	CString bootFilesPath = GET_LOCAL_PATH(CString(BOOT_COMPONENTS_FOLDER)) + L"\\";
	CString usbFilesPath;
	PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;

	// Unpack boot components
	IFFALSE_GOTOERROR(UnpackBootComponents(bootFilesPathGz, bootFilesPath), "Error unpacking boot components.");

	CHECK_IF_CANCELED;

	// initialize create disk data
	memset(&createDiskData, 0, sizeof(createDiskData));
	createDiskData.PartitionStyle = PARTITION_STYLE_GPT;
	createDiskData.Gpt.MaxPartitionCount = GPT_MAX_PARTITION_COUNT;

	// get disk handle
	hPhysical = GetPhysicalHandle(DriveIndex, TRUE, TRUE);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

	// initialize the disk
	result = DeviceIoControl(hPhysical, IOCTL_DISK_CREATE_DISK, &createDiskData, sizeof(createDiskData), NULL, 0, NULL, NULL);
	IFFALSE_GOTOERROR(result != 0, "Error when calling IOCTL_DISK_CREATE_DISK");

	// get disk geometry information
	result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, geometry, sizeof(geometry), &size, NULL);
	IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk geometry.");

	// set initial drive layout data
	memset(layout, 0, sizeof(layout));
	IFFALSE_GOTOERROR(CreateFakePartitionLayout(hPhysical, layout, geometry), "Error on CreateFakePartitionLayout");

	CHECK_IF_CANCELED;

	// Write MBR and SBR to disk
	IFFALSE_GOTOERROR(WriteMBRAndSBRToUSB(hPhysical, bootFilesPath, DiskGeometry->Geometry.BytesPerSector), "Error on WriteMBRAndSBRToUSB");

	safe_closehandle(hPhysical);

	CHECK_IF_CANCELED;

	// Format and mount ESP
	IFFALSE_GOTOERROR(FormatFirstPartitionOnDrive(DriveIndex, FS_FAT32, dlg->m_cancelOperationEvent, L""), "Error on FormatFirstPartitionOnDrive");

	CHECK_IF_CANCELED;

	IFFALSE_GOTOERROR(MountFirstPartitionOnDrive(DriveIndex, driveLetter), "Error on MountFirstPartitionOnDrive");	

	// Copy files to the ESP partition
	IFFALSE_GOTOERROR(CopyFilesToESP(bootFilesPath, driveLetter), "Error when trying to copy files to ESP partition.");

	// Unmount ESP
	if (!DeleteVolumeMountPoint(driveLetter)) uprintf("Failed to unmount volume: %s", WindowsErrorString());

	CHECK_IF_CANCELED;

	// get disk handle again
	hPhysical = GetPhysicalHandle(DriveIndex, TRUE, TRUE);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

	// fix partition types and layout
	IFFALSE_GOTOERROR(CreateCorrectPartitionLayout(hPhysical, layout, geometry), "Error on CreateFakePartitionLayout");
	safe_closehandle(hPhysical);

	CHECK_IF_CANCELED;

	// Format and mount exFAT
	IFFALSE_GOTOERROR(FormatFirstPartitionOnDrive(DriveIndex, FS_EXFAT, dlg->m_cancelOperationEvent, EXFAT_PARTITION_NAME_LIVE), "Error on FormatFirstPartitionOnDrive");

	CHECK_IF_CANCELED;

	IFFALSE_GOTOERROR(MountFirstPartitionOnDrive(DriveIndex, driveLetter), "Error on MountFirstPartitionOnDrive");

	// Copy files to the exFAT partition
	IFFALSE_GOTOERROR(CopyFilesToexFAT(dlg, bootFilesPath, driveLetter), "Error on CopyFilesToexFAT");

	// Unmount exFAT
	if (!DeleteVolumeMountPoint(driveLetter)) uprintf("Failed to unmount volume: %s", WindowsErrorString());

	CHECK_IF_CANCELED;

	goto done;

error:
	uprintf("CreateUSBStick exited with error.");
	if (dlg->m_lastErrorCause == ErrorCause_t::ErrorCauseNone) {
		dlg->m_lastErrorCause = ErrorCause_t::ErrorCauseWriteFailed;
	}

	// Unmount exFAT
	if (!DeleteVolumeMountPoint(driveLetter)) uprintf("Failed to unmount volume: %s", WindowsErrorString());

done:
	RemoveNonEmptyDirectory(bootFilesPath);
	safe_closehandle(hPhysical);
	dlg->PostMessage(WM_FINISHED_ALL_OPERATIONS, 0, 0);
	return 0;
}


bool CEndlessUsbToolDlg::CreateFakePartitionLayout(HANDLE hPhysical, PBYTE layout, PBYTE geometry)
{
	FUNCTION_ENTER;

	PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)(void*)layout;
	PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;
	PARTITION_INFORMATION_EX *currentPartition;
	bool returnValue = false;

	DriveLayout->PartitionStyle = PARTITION_STYLE_GPT;
	DriveLayout->PartitionCount = EXPECTED_NUMBER_OF_PARTITIONS;
	IGNORE_RETVAL(CoCreateGuid(&DriveLayout->Gpt.DiskId));

	LONGLONG partitionStart[EXPECTED_NUMBER_OF_PARTITIONS] = {
		ESP_PART_STARTING_SECTOR * DiskGeometry->Geometry.BytesPerSector,
		MBR_PART_STARTING_SECTOR * DiskGeometry->Geometry.BytesPerSector,
		EXFAT_PART_STARTING_SECTOR * DiskGeometry->Geometry.BytesPerSector
	};
	LONGLONG partitionSize[EXPECTED_NUMBER_OF_PARTITIONS] = { ESP_PART_LENGTH_BYTES, MBR_PART_LENGTH_BYTES, DiskGeometry->DiskSize.QuadPart - partitionStart[2] };
	GUID partitionType[EXPECTED_NUMBER_OF_PARTITIONS] = { PARTITION_BASIC_DATA_GUID/*PARTITION_SYSTEM_GUID*/, PARTITION_BIOS_BOOT_GUID, PARTITION_BASIC_DATA_GUID };

	for (int partIndex = 0; partIndex < EXPECTED_NUMBER_OF_PARTITIONS; partIndex++) {
		currentPartition = &(DriveLayout->PartitionEntry[partIndex]);
		currentPartition->PartitionStyle = PARTITION_STYLE_GPT;
		currentPartition->StartingOffset.QuadPart = partitionStart[partIndex];
		currentPartition->PartitionLength.QuadPart = partitionSize[partIndex];
		currentPartition->PartitionNumber = partIndex + 1; // partition numbers start from 1
		currentPartition->RewritePartition = TRUE;
		currentPartition->Gpt.PartitionType = partitionType[partIndex];
		IGNORE_RETVAL(CoCreateGuid(&currentPartition->Gpt.PartitionId));

		if (partIndex == EXPECTED_NUMBER_OF_PARTITIONS - 1) wcscpy(currentPartition->Gpt.Name, EXFAT_PARTITION_NAME_LIVE);
	}

	// push partition information to drive
	DWORD size = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + DriveLayout->PartitionCount * sizeof(PARTITION_INFORMATION_EX);
	IFFALSE_GOTOERROR(DeviceIoControl(hPhysical, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, layout, size, NULL, 0, &size, NULL), "Could not set drive layout.");
	DWORD result = RefreshDriveLayout(hPhysical);
	IFFALSE_PRINTERROR(result != 0, "Warning: failure on IOCTL_DISK_UPDATE_PROPERTIES"); // not returning here, maybe formatting will still work although IOCTL_DISK_UPDATE_PROPERTIES failed

	returnValue = true;
error:
	return returnValue;
}

bool CEndlessUsbToolDlg::CreateCorrectPartitionLayout(HANDLE hPhysical, PBYTE layout, PBYTE geometry)
{
	FUNCTION_ENTER;

	PARTITION_INFORMATION_EX exfatPartition;
	PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)(void*)layout;
	PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;
	bool returnValue = false;

	// save the exFAT partition data
	memcpy(&exfatPartition, &(DriveLayout->PartitionEntry[2]), sizeof(PARTITION_INFORMATION_EX));
	exfatPartition.PartitionNumber = 1;
	// move ESP and MBR
	memcpy(&(DriveLayout->PartitionEntry[2]), &(DriveLayout->PartitionEntry[1]), sizeof(PARTITION_INFORMATION_EX));
	DriveLayout->PartitionEntry[2].PartitionNumber = 3;
	memcpy(&(DriveLayout->PartitionEntry[1]), &(DriveLayout->PartitionEntry[0]), sizeof(PARTITION_INFORMATION_EX));
	DriveLayout->PartitionEntry[1].PartitionNumber = 2;
	DriveLayout->PartitionEntry[1].Gpt.PartitionType = PARTITION_SYSTEM_GUID;
	// copy exFAT to first position
	memcpy(&(DriveLayout->PartitionEntry[0]), &exfatPartition, sizeof(PARTITION_INFORMATION_EX));

	// push partition information to drive
	DWORD size = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + DriveLayout->PartitionCount * sizeof(PARTITION_INFORMATION_EX);
	IFFALSE_GOTOERROR(DeviceIoControl(hPhysical, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, layout, size, NULL, 0, &size, NULL), "Could not set drive layout.");
	DWORD result = RefreshDriveLayout(hPhysical);
	IFFALSE_PRINTERROR(result != 0, "Warning: failure on IOCTL_DISK_UPDATE_PROPERTIES"); // not returning here, maybe formatting will still work although IOCTL_DISK_UPDATE_PROPERTIES failed

	returnValue = true;
error:
	return returnValue;
}

bool CEndlessUsbToolDlg::FormatFirstPartitionOnDrive(DWORD DriveIndex, int fsToUse, HANDLE cancelEvent, const wchar_t *partLabel)
{
	FUNCTION_ENTER;

	BOOL result;
	int formatRetries = 5;
	bool returnValue = false;

	// Wait for the logical drive we just created to appear
	uprintf("Waiting for logical drive to reappear...\n");
	Sleep(200); // Radu: check if this is needed, that's what rufus does; I hate sync using sleep
	IFFALSE_PRINTERROR(WaitForLogical(DriveIndex), "Warning: Logical drive was not found!"); // We try to continue even if this fails, just in case

	while (formatRetries-- > 0 && !(result = FormatDrive(DriveIndex, fsToUse, partLabel))) {
		Sleep(200); // Radu: check if this is needed, that's what rufus does; I hate sync using sleep
		// Check if user canceled
		IFFALSE_GOTOERROR(WaitForSingleObject(cancelEvent, 0) == WAIT_TIMEOUT, "User cancel.");
	}
	IFFALSE_GOTOERROR(result, "Format error.");

	Sleep(200); // Radu: check if this is needed, that's what rufus does; I hate sync using sleep
	IFFALSE_PRINTERROR(WaitForLogical(DriveIndex), "Warning: Logical drive was not found after format!");

	returnValue = true;

error:
	return returnValue;
}

bool CEndlessUsbToolDlg::MountFirstPartitionOnDrive(DWORD DriveIndex, CString &driveLetter)
{
	FUNCTION_ENTER;

	char *guid_volume = NULL;
	bool returnValue = false;

	guid_volume = GetLogicalName(DriveIndex, TRUE, TRUE);
	IFFALSE_GOTOERROR(guid_volume != NULL, "Could not get GUID volume name\n");
	uprintf("Found volume GUID %s\n", guid_volume);

	// Mount partition
	char drive_name[] = "?:\\";
	drive_name[0] = GetUnusedDriveLetter();
	IFFALSE_GOTOERROR(drive_name[0] != 0, "Could not find an unused drive letter");
	IFFALSE_GOTOERROR(MountVolume(drive_name, guid_volume), "Could not mount volume.");
	driveLetter = drive_name;

	returnValue = true;

error:
	safe_free(guid_volume);
	return returnValue;
}

bool CEndlessUsbToolDlg::UnpackZip(const CComBSTR source, const CComBSTR dest)
{
	FUNCTION_ENTER;

	HRESULT					hResult;
	CComPtr<IShellDispatch>	pISD;
	CComPtr<Folder>			pToFolder, pFromFolder;
	CComPtr<FolderItems>	folderItems;
	bool					returnValue = false;

	IFFALSE_GOTOERROR(SUCCEEDED(CoInitialize(NULL)), "Error on CoInitialize");
	hResult = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&pISD);
	IFFALSE_GOTOERROR(SUCCEEDED(hResult), "Error on CoCreateInstance with IID_IShellDispatch");
	IFFALSE_GOTOERROR(SUCCEEDED(pISD->NameSpace(CComVariant(dest), &pToFolder)), "Error on pISD->NameSpace for destination.");
	IFFALSE_GOTOERROR(SUCCEEDED(pISD->NameSpace(CComVariant(source), &pFromFolder)), "Error on pISD->NameSpace for source.");

	IFFALSE_GOTOERROR(SUCCEEDED(pFromFolder->Items(&folderItems)), "Error on pFromFolder->Items.");
	IFFALSE_GOTOERROR(SUCCEEDED(pToFolder->CopyHere(CComVariant(folderItems), CComVariant(FOF_NO_UI))), "Error on pToFolder->CopyHere");

	returnValue = true;
error:
	CoUninitialize();
	return returnValue;
}

void CEndlessUsbToolDlg::RemoveNonEmptyDirectory(const CString directoryPath)
{
	FUNCTION_ENTER;

	SHFILEOPSTRUCT fileOperation;
	wchar_t dir[MAX_PATH + 1];
	memset(dir, 0, sizeof(dir));
	wsprintf(dir, L"%ls", directoryPath);

	fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
	fileOperation.pFrom = dir;
	fileOperation.pTo = NULL;
	fileOperation.hwnd = NULL;
	fileOperation.wFunc = FO_DELETE;

	int result = SHFileOperation(&fileOperation);
	uprintf("Removing directory '%ls' result=%d", directoryPath, result);
}

bool CEndlessUsbToolDlg::CopyFilesToESP(const CString &fromFolder, const CString &driveLetter)
{
	FUNCTION_ENTER;

	CString espFolder = driveLetter + EFI_BOOT_SUBDIRECTORY;
	SHFILEOPSTRUCT fileOperation;
	wchar_t fromPath[MAX_PATH + 1], toPath[MAX_PATH + 1];
	bool retResult = false;

	int createDirResult = SHCreateDirectoryExW(NULL, espFolder, NULL);
	IFFALSE_GOTOERROR(createDirResult == ERROR_SUCCESS || createDirResult == ERROR_FILE_EXISTS, "Error creating EFI directory in ESP partition.");

	memset(fromPath, 0, sizeof(fromPath));
	wsprintf(fromPath, L"%ls%ls\\%ls", fromFolder, EFI_BOOT_SUBDIRECTORY, ALL_FILES);
	memset(toPath, 0, sizeof(fromPath));
	wsprintf(toPath, L"%ls", espFolder);

	fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
	fileOperation.pFrom = fromPath;
	fileOperation.pTo = toPath;
	fileOperation.hwnd = NULL;
	fileOperation.wFunc = FO_COPY;

	int result = SHFileOperation(&fileOperation);

	retResult = result == ERROR_SUCCESS;

error:
	return retResult;
}

void CEndlessUsbToolDlg::ImageUnpackCallback(const uint64_t read_bytes)
{
	static int oldPercent = 0;
	static int oldOp = -1;

	float perecentageUnpacked = 100.0f * read_bytes / CEndlessUsbToolDlg::ImageUnpackFileSize;
	float percentage = (float)CEndlessUsbToolDlg::ImageUnpackPercentStart;
	percentage += (CEndlessUsbToolDlg::ImageUnpackPercentEnd - CEndlessUsbToolDlg::ImageUnpackPercentStart) * perecentageUnpacked / 100;

	bool change = false;
	if (CEndlessUsbToolDlg::ImageUnpackOperation != oldOp) {
		oldOp = CEndlessUsbToolDlg::ImageUnpackOperation;
		oldPercent = (int)floor(percentage);
		change = true;
	} else if (oldPercent != (int)floor(percentage)) {
		oldPercent = (int)floor(percentage);
		change = true;
	}

	if (change) {
		uprintf("Operation %ls(%d) - unpacked %s",
			OperationToStr(CEndlessUsbToolDlg::ImageUnpackOperation),
			CEndlessUsbToolDlg::ImageUnpackOperation,
			SizeToHumanReadable(read_bytes, FALSE, use_fake_units));
	}
	UpdateProgress(OP_SETUP_DUALBOOT, percentage);
}

bool CEndlessUsbToolDlg::CopyFilesToexFAT(CEndlessUsbToolDlg *dlg, const CString &fromFolder, const CString &driveLetter)
{
	FUNCTION_ENTER;

	bool retResult = false;

	CString usbFilesPath = driveLetter + PATH_ENDLESS_SUBDIRECTORY;

	int createDirResult = SHCreateDirectoryExW(NULL, usbFilesPath, NULL);
	IFFALSE_GOTOERROR(createDirResult == ERROR_SUCCESS || createDirResult == ERROR_FILE_EXISTS, "Error creating directory on USB drive.");

	bool unpackResult = dlg->UnpackFile(ConvertUnicodeToUTF8(dlg->m_localFile), ConvertUnicodeToUTF8(usbFilesPath + ENDLESS_IMG_FILE_NAME), BLED_COMPRESSION_GZIP, ImageUnpackCallback, &dlg->m_cancelImageUnpack);
	IFFALSE_GOTOERROR(unpackResult, "Error unpacking image to USB drive");

	IFFALSE_GOTOERROR(0 != CopyFile(dlg->m_unpackedImageSig, usbFilesPath + CSTRING_GET_LAST(dlg->m_unpackedImageSig, '\\'), FALSE), "Error copying image signature file to drive.");

	FILE *liveFile;
	IFFALSE_GOTOERROR(0 == _wfopen_s(&liveFile, usbFilesPath + EXFAT_ENDLESS_LIVE_FILE_NAME, L"w"), "Error creating empty live file.");
	fclose(liveFile);

	// copy grub to USB drive
	IFFALSE_GOTOERROR(CopyMultipleItems(fromFolder + GRUB_BOOT_SUBDIRECTORY, usbFilesPath), "Error copying grub folder to USB drive.");

	retResult = true;

error:
	return retResult;
}

bool CEndlessUsbToolDlg::CopyMultipleItems(const CString &from, const CString &to)
{
	FUNCTION_ENTER;

	SHFILEOPSTRUCT fileOperation;
	wchar_t fromPath[MAX_PATH + 1], toPath[MAX_PATH + 1];

	uprintf("Copying '%ls' to '%ls'", from, to);

	memset(fromPath, 0, sizeof(fromPath));
	wsprintf(fromPath, L"%ls", from);
	memset(toPath, 0, sizeof(toPath));
	wsprintf(toPath, L"%ls", to);

	fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
	fileOperation.pFrom = fromPath;
	fileOperation.pTo = toPath;
	fileOperation.hwnd = NULL;
	fileOperation.wFunc = FO_COPY;

	int result = SHFileOperation(&fileOperation);

	return result == 0;
}

bool CEndlessUsbToolDlg::WriteMBRAndSBRToUSB(HANDLE hPhysical, const CString &bootFilesPath, DWORD bytesPerSector)
{
	FUNCTION_ENTER;

	FAKE_FD fake_fd = { 0 };
	FILE* fp = (FILE*)&fake_fd;
	FILE *bootImgFile = NULL, *coreImgFile = NULL;
	CString bootImgFilePath = bootFilesPath + LIVE_BOOT_IMG_FILE;
	CString coreImgFilePath = bootFilesPath + LIVE_CORE_IMG_FILE;
	unsigned char endlessMBRData[MAX_BOOT_IMG_FILE_SIZE + 1];
	unsigned char *endlessSBRData = NULL;
	bool retResult = false;
	size_t countRead, coreImgSize;
	size_t mbrPartitionStart = bytesPerSector * MBR_PART_STARTING_SECTOR;

	// Load boot.img from file
	IFFALSE_GOTOERROR(0 == _wfopen_s(&bootImgFile, bootImgFilePath, L"rb"), "Error opening boot.img file");
	countRead = fread(endlessMBRData, 1, sizeof(endlessMBRData), bootImgFile);
	IFFALSE_GOTOERROR(countRead == MAX_BOOT_IMG_FILE_SIZE, "Size of boot.img is not what is expected.");

	// write boot.img to USB drive
	fake_fd._handle = (char*)hPhysical;
	set_bytes_per_sector(SelectedDrive.Geometry.BytesPerSector);
	IFFALSE_GOTOERROR(write_data(fp, 0x0, endlessMBRData, MAX_BOOT_IMG_FILE_SIZE) != 0, "Error on write_data with boot.img contents.");

	// Read core.img data and write it to USB drive
	IFFALSE_GOTOERROR(0 == _wfopen_s(&coreImgFile, coreImgFilePath, L"rb"), "Error opening core.img file");
	fseek(coreImgFile, 0L, SEEK_END);
	coreImgSize = ftell(coreImgFile);
	rewind(coreImgFile);
	uprintf("Size of SBR is %d bytes from %ls", coreImgSize, coreImgFilePath);
	IFFALSE_GOTOERROR(coreImgSize <= MBR_PART_LENGTH_BYTES, "Error: SBR found in core.img is too big.");

	endlessSBRData = (unsigned char*)malloc(bytesPerSector);
	coreImgSize = 0;
	while (!feof(coreImgFile) && coreImgSize < MBR_PART_LENGTH_BYTES) {
		countRead = fread(endlessSBRData, 1, bytesPerSector, coreImgFile);
		IFFALSE_GOTOERROR(write_data(fp, mbrPartitionStart + coreImgSize, endlessSBRData, countRead) != 0, "Error on write data with core.img contents.");
		coreImgSize += countRead;
		uprintf("Wrote %d bytes", coreImgSize);
	}

	retResult = true;

	DWORD size;
	// Tell the system we've updated the disk properties
	if (!DeviceIoControl(hPhysical, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &size, NULL))
		uprintf("Failed to notify system about disk properties update: %s\n", WindowsErrorString());

error:
	safe_closefile(bootImgFile);
	safe_closefile(coreImgFile);

	if (endlessSBRData != NULL) {
		safe_free(endlessSBRData);
	}

	return retResult;
}

// Below defines need to be in this order
#define DB_PROGRESS_UNPACK_BOOT_ZIP		1
#define DB_PROGRESS_CHECK_PARTITION		1
#define DB_PROGRESS_FINISHED_UNPACK		95
#define DB_PROGRESS_COPY_GRUB_FOLDER	98
#define DB_PROGRESS_MBR_OR_EFI_SETUP	100

#define REGKEY_UTC_TIME_PATH	L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation"
#define REGKEY_UTC_TIME			L"RealTimeIsUniversal"
#define REGKEY_FASTBOOT_PATH	L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"
#define REGKEY_FASTBOOT			L"HiberbootEnabled"

DWORD WINAPI CEndlessUsbToolDlg::SetupDualBoot(LPVOID param)
{
	FUNCTION_ENTER;

	CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg*)param;
	CString systemDriveLetter;
	CString endlessFilesPath;
	CString endlessImgPath;
	CString bootFilesPath = GET_LOCAL_PATH(CString(BOOT_COMPONENTS_FOLDER)) + L"\\";
	wchar_t fileSystemType[MAX_PATH + 1];
	CStringW exeFilePath = GetExePath();

	systemDriveLetter = GetSystemDrive();
	endlessFilesPath = systemDriveLetter + PATH_ENDLESS_SUBDIRECTORY;
	endlessImgPath = endlessFilesPath + ENDLESS_IMG_FILE_NAME;

	// Verify that this is an NTFS partition
	IFFALSE_GOTOERROR(GetVolumeInformation(systemDriveLetter, NULL, 0, NULL, NULL, NULL, fileSystemType, MAX_PATH + 1) != 0, "Error on GetVolumeInformation.");
	uprintf("File system type '%ls'", fileSystemType);
	IFFALSE_GOTO(0 == wcscmp(fileSystemType, L"NTFS"), "File system type is not NTFS", not_ntfs);

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_CHECK_PARTITION);

	IFFALSE_PRINTERROR(ChangeAccessPermissions(endlessFilesPath, false), "Error on granting Endless files permissions.");

	// Unpack boot components
	IFFALSE_GOTOERROR(UnpackBootComponents(dlg->m_bootArchive, bootFilesPath), "Error unpacking boot components.");

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_UNPACK_BOOT_ZIP);
	CHECK_IF_CANCELED;

	// Create endless folder
	int createDirResult = SHCreateDirectoryExW(NULL, endlessFilesPath, NULL);
	IFFALSE_GOTOERROR(createDirResult == ERROR_SUCCESS || createDirResult == ERROR_ALREADY_EXISTS, "Error creating directory on USB drive.");

	// Copy ourseleves to the <SYSDRIVE>:\endless directory
	IFFALSE_GOTOERROR(0 != CopyFile(exeFilePath, endlessFilesPath + ENDLESS_UNINSTALLER_NAME, FALSE), "Error copying exe uninstaller file.");

	// Unpack img file
	CEndlessUsbToolDlg::ImageUnpackOperation = OP_SETUP_DUALBOOT;
	CEndlessUsbToolDlg::ImageUnpackPercentStart = DB_PROGRESS_CHECK_PARTITION;
	CEndlessUsbToolDlg::ImageUnpackPercentEnd = DB_PROGRESS_FINISHED_UNPACK;
	CEndlessUsbToolDlg::ImageUnpackFileSize = dlg->m_selectedFileSize;
	bool unpackResult = dlg->UnpackFile(ConvertUnicodeToUTF8(dlg->m_localFile), ConvertUnicodeToUTF8(endlessImgPath), BLED_COMPRESSION_GZIP, ImageUnpackCallback, &dlg->m_cancelImageUnpack);
	IFFALSE_GOTOERROR(unpackResult, "Error unpacking image to endless folder.");

	// extend this file so it reaches the required size
	IFFALSE_GOTOERROR(ExtendImageFile(endlessImgPath, dlg->m_nrGigsSelected), "Error extending Endless image file");

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_FINISHED_UNPACK);
	CHECK_IF_CANCELED;

	// Copy grub
	IFFALSE_GOTOERROR(CopyMultipleItems(bootFilesPath + GRUB_BOOT_SUBDIRECTORY, endlessFilesPath), "Error copying grub folder to USB drive.");
	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_COPY_GRUB_FOLDER);
	CHECK_IF_CANCELED;

	if (IsLegacyBIOSBoot()) {
		IFFALSE_GOTOERROR(WriteMBRAndSBRToWinDrive(dlg, systemDriveLetter, bootFilesPath, endlessFilesPath), "Error on WriteMBRAndSBRToWinDrive");
	} else {
		IFFALSE_GOTOERROR(SetupEndlessEFI(systemDriveLetter, bootFilesPath), "Error on SetupEndlessEFI");
	}

	// set Endless file permissions
	IFFALSE_PRINTERROR(ChangeAccessPermissions(endlessFilesPath, true), "Error on setting Endless files permissions.");

	// set the registry key for Windows clock in UTC
	IFFALSE_PRINTERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UTC_TIME_PATH, REGKEY_UTC_TIME, CComVariant(1)), "Error on setting Windows clock to UTC.");

	// disable Fast Start (aka Hiberboot) so that the NTFS partition is always cleanly unmounted at shutdown:
	IFFALSE_PRINTERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FASTBOOT_PATH, REGKEY_FASTBOOT, CComVariant(1)), "Error on disabling fastboot.");

	// Add uninstall entry for Control Panel
	IFFALSE_GOTOERROR(AddUninstallRegistryKeys(endlessFilesPath + ENDLESS_UNINSTALLER_NAME, endlessFilesPath, 3, 0), "Error on AddUninstallRegistryKeys.");

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_MBR_OR_EFI_SETUP);

	goto done;

not_ntfs:
    dlg->m_lastErrorCause = ErrorCause_t::ErrorCauseNotNTFS;

error:
	uprintf("SetupDualBoot exited with error.");
	if (dlg->m_lastErrorCause == ErrorCause_t::ErrorCauseNone) {
		dlg->m_lastErrorCause = ErrorCause_t::ErrorCauseWriteFailed;
	}

	RemoveNonEmptyDirectory(endlessFilesPath);

done:
	RemoveNonEmptyDirectory(bootFilesPath);
	dlg->PostMessage(WM_FINISHED_ALL_OPERATIONS, 0, 0);
	return 0;
}


bool CEndlessUsbToolDlg::ExtendImageFile(const CString &endlessImgPath, ULONGLONG selectedGigs)
{
	FUNCTION_ENTER;
	HANDLE endlessImage = INVALID_HANDLE_VALUE;
	HANDLE hToken = INVALID_HANDLE_VALUE;
	bool retResult = false;

	// We need to have the MANAGE_VOLUME privilege to use SetFileValidData.
	// This lets us mark the extra blocks allocated by SetEndOfFile as
	// initialized, without actually writing 100Gb of zeroes.
	IFFALSE_GOTOERROR(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken) != 0, "OpenProcessToken failed");
	IFFALSE_GOTOERROR(SetPrivilege(hToken, SE_MANAGE_VOLUME_NAME, TRUE), "Can't take MANAGE_VOLUME privilege. You must be logged on as Administrator.");

	endlessImage = CreateFile(endlessImgPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	IFFALSE_GOTOERROR(endlessImage != INVALID_HANDLE_VALUE, "Error opening Endless image file.");
	LARGE_INTEGER lCurrentSize;
	IFFALSE_GOTOERROR(GetFileSizeEx(endlessImage, &lCurrentSize), "Error getting Endless image file size");
	LARGE_INTEGER lsize;
	lsize.QuadPart = selectedGigs * BYTES_IN_GIGABYTE;
	
	uprintf("Trying to extend Endless image from %I64i bytes to %I64i bytes which is about %s", lCurrentSize.QuadPart, lsize.QuadPart, SizeToHumanReadable(lsize.QuadPart, FALSE, use_fake_units));
	IFFALSE_GOTOERROR(SetFilePointerEx(endlessImage, lsize, NULL, FILE_BEGIN) != 0, "Error on SetFilePointerEx");
	IFFALSE_GOTOERROR(SetEndOfFile(endlessImage), "Error on SetEndOfFile");
	IFFALSE_GOTOERROR(SetFileValidData(endlessImage, lsize.QuadPart), "Error on SetFileValidData");
	uprintf("Extended Endless image");

	IFFALSE_PRINTERROR(SetPrivilege(hToken, SE_MANAGE_VOLUME_NAME, FALSE), "Can't release MANAGE_VOLUME privilege.");

	retResult = true;

error:
	safe_closehandle(endlessImage);
	safe_closehandle(hToken);
	return retResult;
}

bool CEndlessUsbToolDlg::UnpackBootComponents(const CString &bootFilesPathGz, const CString &bootFilesPath)
{
	FUNCTION_ENTER;
	bool retResult = false;

	RemoveNonEmptyDirectory(bootFilesPath);
	int createDirResult = SHCreateDirectoryExW(NULL, bootFilesPath, NULL);
	IFFALSE_GOTOERROR(createDirResult == ERROR_SUCCESS || createDirResult == ERROR_ALREADY_EXISTS, "Error creating local directory to unpack boot components.");
	IFFALSE_GOTOERROR(UnpackZip(CComBSTR(bootFilesPathGz), CComBSTR(bootFilesPath)), "Error unpacking archive to local folder.");

	retResult = true;
error:
	return retResult;
}

bool CEndlessUsbToolDlg::IsLegacyBIOSBoot()
{
	FUNCTION_ENTER;
	// From https://msdn.microsoft.com/en-us/library/windows/desktop/ms724325(v=vs.85).aspx
	// On a legacy BIOS-based system, or on a system that supports both legacy BIOS and UEFI where Windows was installed using legacy BIOS,
	// the function will fail with ERROR_INVALID_FUNCTION. On a UEFI-based system, the function will fail with an error specific to the firmware,
	// such as ERROR_NOACCESS, to indicate that the dummy GUID namespace does not exist.
	DWORD result = GetFirmwareEnvironmentVariable(L"", L"{00000000-0000-0000-0000-000000000000}", NULL, 0);
	return result == 0 && GetLastError() == ERROR_INVALID_FUNCTION;
}

// Returns TRUE if the drive has a Windows MBR, FALSE otherwise
bool CEndlessUsbToolDlg::IsWindowsMBR(FILE* fpDrive, const CString &TargetName)
{
	FUNCTION_ENTER;

    const struct {int (*fn)(FILE *fp); char* str;} windows_mbr[] = {
	    { is_2000_mbr, "Windows 2000/XP/2003" },
	    { is_vista_mbr, "Windows Vista" },
	    { is_win7_mbr, "Windows 7" },
    };

	int i;

	set_bytes_per_sector(SelectedDrive.Geometry.BytesPerSector);

	for (i=0; i < ARRAYSIZE(windows_mbr); i++) {
		if (windows_mbr[i].fn(fpDrive)) {
			uprintf("%ls has a %s MBR\n", TargetName, windows_mbr[i].str);
			return TRUE;
		}
	}

	uprintf("%ls has a non-Windows MBR\n", TargetName);
	return FALSE;
}

/* Handle nonstandard sector sizes (such as 4K) by writing
the boot marker at every 512-2 bytes location */
static int write_bootmark(FILE *fp, unsigned long ulBytesPerSector)
{
    unsigned char aucRef[] = { 0x55, 0xAA };
    unsigned long pos = 0x1FE;

    for (pos = 0x1FE; pos < ulBytesPerSector; pos += 0x200) {
        if (!write_data(fp, pos, aucRef, sizeof(aucRef)))
            return 0;
    }
    return 1;
}


bool CEndlessUsbToolDlg::WriteMBRAndSBRToWinDrive(CEndlessUsbToolDlg *dlg, const CString &systemDriveLetter, const CString &bootFilesPath, const CString &endlessFilesPath)
{
	FUNCTION_ENTER;

	bool retResult = false;
	HANDLE hPhysical = INVALID_HANDLE_VALUE;
	BYTE geometry[256] = { 0 };
	PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;
	BYTE layout[4096] = { 0 };
	PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)(void*)layout;
	DWORD size;
	BOOL result;
	CString coreImgFilePath = bootFilesPath + NTFS_CORE_IMG_FILE;
	FILE *bootImgFile = NULL, *boottrackImgFile = NULL;
	FAKE_FD fake_fd = { 0 };
	FILE* fp = (FILE*)&fake_fd;
	CString bootImgFilePath = bootFilesPath + NTFS_BOOT_IMG_FILE;
	unsigned char endlessMBRData[MAX_BOOT_IMG_FILE_SIZE];
	size_t countRead;
	unsigned char *boottrackData = NULL;	

	// Get system disk handle
	hPhysical = GetPhysicalFromDriveLetter(systemDriveLetter);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

	// Make sure there already is a Windows MBR on this disk
	fake_fd._handle = (char*)hPhysical;
	if (!IsWindowsMBR(fp, systemDriveLetter)) {
		uprintf("Error: no Windows MBR detected, unsupported configuration.");
		dlg->m_lastErrorCause = ErrorCause_t::ErrorCauseNonWindowsMBR;
		goto error;
	}

	// get disk geometry information
	result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, geometry, sizeof(geometry), &size, NULL);
	IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk geometry.");
	set_bytes_per_sector(DiskGeometry->Geometry.BytesPerSector);

	// Check that SBR will "fit" before writing to disk
	// Jira issue: https://movial.atlassian.net/browse/EOIFT-158
	// This will be done in grub_util_bios_setup, here just get the last available sector for writing the SBR between MBR and first partition
	result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, layout, sizeof(layout), &size, NULL);
	IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk layout.");
	IFFALSE_GOTOERROR(DriveLayout->PartitionCount > 0, "We don't have any partitions?");
	uprintf("BytesPerSector=%d, PartitionEntry[0].StartingOffset=%I64i", DiskGeometry->Geometry.BytesPerSector, DriveLayout->PartitionEntry[0].StartingOffset.QuadPart);

	// preserve boot track before writing anything to disk
	// Jira https://movial.atlassian.net/browse/EOIFT-169
	IFFALSE_GOTOERROR(0 == _wfopen_s(&boottrackImgFile, endlessFilesPath + BACKUP_BOOTTRACK_IMG, L"wb"), "Error opening boottrack.img file");
	boottrackData = (unsigned char*)malloc(DiskGeometry->Geometry.BytesPerSector);
	LONGLONG boottrackNrSectors = DriveLayout->PartitionEntry[0].StartingOffset.QuadPart / DiskGeometry->Geometry.BytesPerSector;
	uprintf("Trying to save %I64i sectors in file %ls", boottrackNrSectors, endlessFilesPath + BACKUP_BOOTTRACK_IMG);
	for (LONGLONG i = 0; i < boottrackNrSectors; i++) {
		int64_t result = read_sectors(hPhysical, DiskGeometry->Geometry.BytesPerSector, i, 1, boottrackData);
		IFFALSE_GOTOERROR(result == DiskGeometry->Geometry.BytesPerSector, "Error reading from disk.");
		IFFALSE_GOTOERROR(fwrite(boottrackData, 1, DiskGeometry->Geometry.BytesPerSector, boottrackImgFile) == DiskGeometry->Geometry.BytesPerSector, "Error writing data to boottrack.img.");
	}
	safe_closefile(boottrackImgFile);

	// Write core.img
	IFFALSE_GOTOERROR(grub_util_bios_setup(coreImgFilePath, hPhysical, DriveLayout->PartitionEntry[0].StartingOffset.QuadPart / DiskGeometry->Geometry.BytesPerSector), "Error writing SBR to disk");

	// Load boot.img from file
	IFFALSE_GOTOERROR(0 == _wfopen_s(&bootImgFile, bootImgFilePath, L"rb"), "Error opening boot.img file");
	countRead = fread(endlessMBRData, 1, MAX_BOOT_IMG_FILE_SIZE, bootImgFile);
	IFFALSE_GOTOERROR(countRead == MAX_BOOT_IMG_FILE_SIZE, "Size of boot.img is not what is expected. Not enough data to read");

	// Also, ensure we apply the workaround for stupid BIOSes: https://github.com/endlessm/grub/blob/master/util/setup.c#L384
	// use boot.img from boot.zip rather than rufus - https://movial.atlassian.net/browse/EOIFT-170
	unsigned char *boot_drive_check = (unsigned char *)(endlessMBRData + GRUB_BOOT_MACHINE_DRIVE_CHECK);
	/* Replace the jmp (2 bytes) with double nop's.  */
	boot_drive_check[0] = 0x90;
	boot_drive_check[1] = 0x90;

	// write boot.img to disk
	fake_fd._handle = (char*)hPhysical;
	set_bytes_per_sector(SelectedDrive.Geometry.BytesPerSector);
	IFFALSE_GOTOERROR(write_data(fp, 0x0, endlessMBRData, MBR_WINDOWS_NT_MAGIC) != 0, "Error on write_data with boot.img contents.");
    IFFALSE_GOTOERROR(write_bootmark(fp, DiskGeometry->Geometry.BytesPerSector), "Error on write_bootmark");

	retResult = true;

error:
	safe_closehandle(hPhysical);
	safe_closefile(bootImgFile);
	safe_closefile(boottrackImgFile);

	if (boottrackData != NULL) safe_free(boottrackData);

	return retResult;
}

bool CEndlessUsbToolDlg::SetupEndlessEFI(const CString &systemDriveLetter, const CString &bootFilesPath)
{
	FUNCTION_ENTER;

	HANDLE hPhysical;
	bool retResult = false;
	BYTE layout[4096] = { 0 };
	PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)(void*)layout;
	CStringA systemDriveA;
	CString windowsEspDriveLetter;
	const char *espMountLetter = NULL;

	hPhysical = GetPhysicalFromDriveLetter(systemDriveLetter);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

	IFFALSE_GOTOERROR(MountESPFromDrive(hPhysical, &espMountLetter, systemDriveLetter), "Error on MountESPFromDrive");
	windowsEspDriveLetter = UTF8ToCString(espMountLetter);

	IFFALSE_GOTOERROR(CopyMultipleItems(bootFilesPath + EFI_BOOT_SUBDIRECTORY + L"\\" + ALL_FILES, windowsEspDriveLetter + L"\\" + ENDLESS_BOOT_SUBDIRECTORY), "Error copying EFI folder to Windows ESP partition.");

	IFFALSE_PRINTERROR(EFIRequireNeededPrivileges(), "Error on EFIRequireNeededPrivileges.");
	IFFALSE_GOTOERROR(EFICreateNewEntry(windowsEspDriveLetter, CString(L"\\") + ENDLESS_BOOT_SUBDIRECTORY + L"\\" + ENDLESS_BOOT_EFI_FILE, ENDLESS_OS_NAME), "Error on EFICreateNewEntry");

	retResult = true;

error:
	safe_closehandle(hPhysical);
	if (espMountLetter != NULL) AltUnmountVolume(espMountLetter);

	return retResult;
}

HANDLE CEndlessUsbToolDlg::GetPhysicalFromDriveLetter(const CString &driveLetter)
{
	FUNCTION_ENTER;

	HANDLE hPhysical = INVALID_HANDLE_VALUE;
	CStringA logical_drive;

	logical_drive.Format("\\\\.\\%lc:", driveLetter[0]);
	hPhysical = CreateFileA(logical_drive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "CreateFileA call returned invalid handle.");

	int drive_number = GetDriveNumber(hPhysical, logical_drive.GetBuffer());
	drive_number += DRIVE_INDEX_MIN;
	safe_closehandle(hPhysical);

	hPhysical = GetPhysicalHandle(drive_number, TRUE, TRUE);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

error:
	return hPhysical;
}

bool CEndlessUsbToolDlg::Has64BitSupport()
{
	// MSDN: https://msdn.microsoft.com/en-us/library/hskdteyh%28v=vs.100%29.aspx?f=255&MSPPError=-2147217396
	int CPUInfo[4];

	// first check if extended data is available, which is always true on 64-bit
	__cpuid(CPUInfo, 0x80000000);
	if (!(CPUInfo[0] & 0x80000000))
		return false;

	// request extended data, bit 29 is set on 64-bit systems
	__cpuid(CPUInfo, 0x80000001);
	if (CPUInfo[3] & 0x20000000)
		return true;

	return false;
}

BOOL CEndlessUsbToolDlg::SetAttributesForFilesInFolder(CString path, bool addAttributes)
{
	WIN32_FIND_DATA findFileData;
	HANDLE findFilesHandle = FindFirstFile(path + ALL_FILES, &findFileData);
	BOOL retValue = TRUE;
	BOOL result;

	uprintf("RestrictAccessToFilesInFolder called with '%ls'", path);

	IFFALSE_PRINTERROR(SetFileAttributes(path, addAttributes ? FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_NORMAL) != 0, "Error on SetFileAttributes");

	if (findFilesHandle == INVALID_HANDLE_VALUE) {
		uprintf("UpdateFileEntries: No files found in current directory [%ls]", path);
	} else {
		do {
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if ((0 == wcscmp(findFileData.cFileName, L".") || 0 == wcscmp(findFileData.cFileName, L".."))) continue;

				CString newFolder = path + findFileData.cFileName + L"\\";
				IFFALSE_PRINTERROR(result = SetAttributesForFilesInFolder(newFolder, addAttributes), "Error on setting attributes.");
				retValue = retValue && result;
			} else {
				if (0 != wcscmp(findFileData.cFileName, ENDLESS_UNINSTALLER_NAME)) {
					IFFALSE_PRINTERROR(SetFileAttributes(path + findFileData.cFileName, addAttributes ? FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL) != 0, "Error on SetFileAttributes");
				}
			}
		} while (FindNextFile(findFilesHandle, &findFileData) != 0);

		FindClose(findFilesHandle);
	}

	return retValue;
}

BOOL CEndlessUsbToolDlg::SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	TOKEN_PRIVILEGES tp, tpOld;
	DWORD tpOldLen = sizeof(TOKEN_PRIVILEGES);
	LUID luid;
	BOOL retResult = FALSE;

	IFFALSE_GOTOERROR(LookupPrivilegeValue(NULL, lpszPrivilege, &luid) != 0, "LookupPrivilegeValue error");

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

	IFFALSE_GOTOERROR(AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpOld, &tpOldLen) != 0, "AdjustTokenPrivileges error");
	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
		IFFALSE_GOTOERROR(
			tpOld.PrivilegeCount == 1 &&
			tpOld.Privileges[0].Luid.LowPart == luid.LowPart &&
			tpOld.Privileges[0].Luid.HighPart == luid.HighPart,
			"AdjustTokenPrivileges returned ERROR_NOT_ALL_ASSIGNED, and the requested privilege was indeed not altered");
	}
	
	retResult = TRUE;
error:
	return retResult;
}


BOOL CEndlessUsbToolDlg::ChangeAccessPermissions(CString path, bool restrictAccess)
{
	LPWSTR pFilename = path.GetBuffer();
	BOOL retResult = FALSE;
	PSID pSIDEveryone = NULL;
	PSID pSIDAdmin = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	const int NUM_ACES = 2;
	EXPLICIT_ACCESS ea[NUM_ACES];
	PACL pACL = NULL;
	HANDLE hToken = NULL;

	uprintf("RestrictFileAccess called with '%ls'", path);

	// Mark files as system and read-only
	if(restrictAccess) IFFALSE_PRINTERROR(SetAttributesForFilesInFolder(path, restrictAccess), "Error on SetFileAttributes");

	// Specify the DACL to use.
	// Create a SID for the Everyone group.
	IFFALSE_GOTOERROR(AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSIDEveryone), "AllocateAndInitializeSid (Everyone) error");
	// Create a SID for the BUILTIN\Administrators group.
	IFFALSE_GOTOERROR(AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSIDAdmin), "AllocateAndInitializeSid (Admin) error");

	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	// Deny access for Everyone.
	ea[0].grfAccessPermissions = restrictAccess ? DELETE | FILE_DELETE_CHILD | FILE_WRITE_ATTRIBUTES : STANDARD_RIGHTS_ALL;
	ea[0].grfAccessMode = restrictAccess ? DENY_ACCESS : SET_ACCESS;
	ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR)pSIDEveryone;

	// Set full control for Administrators.
	ea[1].grfAccessPermissions = GENERIC_ALL;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR)pSIDAdmin;

	IFFALSE_GOTOERROR(ERROR_SUCCESS == SetEntriesInAcl(NUM_ACES, ea, NULL, &pACL), "Failed SetEntriesInAcl");

	// Try to modify the object's DACL.
	DWORD dwRes = SetNamedSecurityInfo(
		pFilename,					// name of the object
		SE_FILE_OBJECT,              // type of object
		DACL_SECURITY_INFORMATION,   // change only the object's DACL
		NULL, NULL,                  // do not change owner or group
		pACL,                        // DACL specified
		NULL);                       // do not change SACL
	uprintf("Return value for first SetNamedSecurityInfo call %u", dwRes);
	IFTRUE_GOTO(ERROR_SUCCESS == dwRes, "Successfully changed DACL", done);
	IFFALSE_GOTOERROR(dwRes == ERROR_ACCESS_DENIED, "First SetNamedSecurityInfo call failed");

	// If the preceding call failed because access was denied,
	// enable the SE_TAKE_OWNERSHIP_NAME privilege, create a SID for
	// the Administrators group, take ownership of the object, and
	// disable the privilege. Then try again to set the object's DACL.

	// Open a handle to the access token for the calling process.
	IFFALSE_GOTOERROR(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken) != 0, "OpenProcessToken failed");
	// Enable the SE_TAKE_OWNERSHIP_NAME privilege.
	IFFALSE_GOTOERROR(SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE), "You must be logged on as Administrator.");
	// Set the owner in the object's security descriptor.
	dwRes = SetNamedSecurityInfo(
		pFilename,					// name of the object
		SE_FILE_OBJECT,              // type of object
		OWNER_SECURITY_INFORMATION,  // change only the object's owner
		pSIDAdmin,                   // SID of Administrator group
		NULL,
		NULL,
		NULL);
	uprintf("Return value for second SetNamedSecurityInfo call %u", dwRes);
	IFFALSE_GOTOERROR(ERROR_SUCCESS == dwRes, "Could not set owner.");

	// Disable the SE_TAKE_OWNERSHIP_NAME privilege.
	IFFALSE_GOTOERROR(SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, FALSE), "Failed SetPrivilege call unexpectedly.");

	// Try again to modify the object's DACL, now that we are the owner.
	dwRes = SetNamedSecurityInfo(
		pFilename,					 // name of the object
		SE_FILE_OBJECT,              // type of object
		DACL_SECURITY_INFORMATION,   // change only the object's DACL
		NULL, NULL,                  // do not change owner or group
		pACL,                        // DACL specified
		NULL);                       // do not change SACL
	uprintf("Return value for third SetNamedSecurityInfo call %u", dwRes);
	IFFALSE_GOTOERROR(ERROR_SUCCESS == dwRes, "Third SetNamedSecurityInfo call failed.");

done:
	// Remove system and read-only from files
	if (!restrictAccess) IFFALSE_PRINTERROR(SetAttributesForFilesInFolder(path, restrictAccess), "Error on SetFileAttributes");

	retResult = TRUE;
error:
	if (pSIDAdmin) FreeSid(pSIDAdmin);
	if (pSIDEveryone) FreeSid(pSIDEveryone);
	if (pACL) LocalFree(pACL);
	if (hToken) CloseHandle(hToken);

	return retResult;
}

CStringW CEndlessUsbToolDlg::GetSystemDrive()
{
	CStringW systemDrive(L"C:\\");
	wchar_t windowsPath[MAX_PATH];
	HRESULT hr = SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, windowsPath);

	if (hr == S_OK) {
		systemDrive = windowsPath;
		systemDrive = systemDrive.Left(3);
	} else {
		uprintf("SHGetFolderPath returned %X", hr);
	}

	return systemDrive;
}

#define REGKEY_BACKUP_PREFIX	L"EndlessBackup"
#define REGKEY_DWORD_MISSING	DWORD_MAX

BOOL CEndlessUsbToolDlg::SetEndlessRegistryKey(HKEY parentKey, const CString &keyPath, const CString &keyName, CComVariant keyValue, bool createBackup)
{
	CRegKey registryKey;
	LSTATUS result;
	CComVariant backupValue;

	uprintf("SetEndlessRegistryKey called with path '%ls' and key '%ls'", keyPath, keyName);

	result = registryKey.Open(parentKey, keyPath, KEY_ALL_ACCESS);
	IFFALSE_RETURN_VALUE(result == ERROR_SUCCESS, "Error opening registry key.", FALSE);

	backupValue.ChangeType(VT_NULL);
	switch (keyValue.vt) {
		case VT_I4:
		case VT_UI4:
			if (createBackup) {
				DWORD dwValue;
				result = registryKey.QueryDWORDValue(CString(REGKEY_BACKUP_PREFIX) + keyName, dwValue);
				if (result == ERROR_SUCCESS) {
					uprintf("There already is a backup created for '%ls' in '%ls'. Aborting creating a second backup", keyName, keyPath);
					createBackup = false;
					break;
				}

				result = registryKey.QueryDWORDValue(keyName, dwValue);
				if (result == ERROR_SUCCESS) backupValue = dwValue;
				else backupValue = REGKEY_DWORD_MISSING;
			}
			result = registryKey.SetDWORDValue(keyName, keyValue.intVal);
			break;
		case VT_BSTR:
			if (createBackup) {
				uprintf("Error: backup requested and not implemented for type VT_BSTR.");
				return FALSE;
			}
			result = registryKey.SetStringValue(keyName, keyValue.bstrVal);
			break;
		default:
			uprintf("ERROR: variant type %d not handled", keyValue.vt);
			return FALSE;
	}

	IFFALSE_RETURN_VALUE(result == ERROR_SUCCESS, "Error on setting key value", FALSE);

	if (createBackup && backupValue.vt != VT_NULL) {
		backupValue.vt = keyValue.vt;
		IFFALSE_PRINTERROR(SetEndlessRegistryKey(parentKey, keyPath, CString(REGKEY_BACKUP_PREFIX) + keyName, backupValue, false), "Error on creating backup key");
	}

	return TRUE;
}

#pragma comment(lib, "wbemuuid.lib")

BOOL CEndlessUsbToolDlg::IsBitlockedDrive(const CString &drive)
{
	HRESULT hres;
	BOOL retResult = FALSE;
	CComPtr<IWbemLocator> pLoc;
	CComPtr<IWbemServices> pSvc;
	CComPtr<IEnumWbemClassObject> pEnumerator;
	CComPtr<IWbemClassObject> pclsObj;
	CString bitlockerQuery;

	hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	IFFALSE_PRINTERROR(SUCCEEDED(hres), "Error on CoInitializeEx.");

	hres = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities
		NULL                         // Reserved
	);
	IFFALSE_PRINTERROR(SUCCEEDED(hres), "Error on CoInitializeSecurity.");

	// Obtain the initial locator to WMI
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
	IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on CoCreateInstance.");

	// Connect to WMI through the IWbemLocator::ConnectServer method
	// Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2\\Security\\MicrosoftVolumeEncryption"), // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (for example, Kerberos)
		0,                       // Context object
		&pSvc                    // pointer to IWbemServices proxy
	);
	IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on pLoc->ConnectServer.");

	// Set security levels on the proxy
	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities
	);
	IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on CoSetProxyBlanket.");

	bitlockerQuery.Format(L"Select * from Win32_EncryptableVolume where ProtectionStatus = 1 and DriveLetter = '%ls'", drive);

	// Use the IWbemServices pointer to make requests of WMI
	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(bitlockerQuery), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	IFFALSE_GOTOERROR(SUCCEEDED(hres), "Error on pSvc->ExecQuery.");

	// Retrieve the objects in the result set.
	ULONG uReturned = 0;
	hres = pEnumerator->Next(0, 1, &pclsObj, &uReturned);

	retResult = (hres == S_OK) && (uReturned == 1);

error:
	uprintf("IsBitlockedDrive done with hres=%x and retResult=%d", hres, retResult);
	CoUninitialize();

	return retResult;
}

CStringW CEndlessUsbToolDlg::GetExePath()
{
	wchar_t *exePath = NULL;
	DWORD neededSize = MAX_PATH;
	CStringW retValue = L"";

	do {
		if (exePath != NULL) safe_free(exePath);
		exePath = (wchar_t*)malloc((neededSize + 1) * sizeof(wchar_t));
		memset(exePath, 0, (neededSize + 1) * sizeof(wchar_t));
		DWORD result = GetModuleFileNameW(NULL, exePath, neededSize);
		IFFALSE_GOTOERROR(result != 0 && (GetLastError() == ERROR_SUCCESS || GetLastError() == ERROR_INSUFFICIENT_BUFFER), "Could not get needed size for module path");
		neededSize = result;
		IFFALSE_CONTINUE(GetLastError() != ERROR_INSUFFICIENT_BUFFER, "Not enough memory allocated for buffer. Trying again.");
		break;
	} while (TRUE);

	retValue = exePath;
error:
	if (exePath != NULL) safe_free(exePath);

	return retValue;
}

#define REGKEY_WIN_UNINSTALL	L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
#define REGKEY_ENDLESS_OS		ENDLESS_OS_NAME
#define REGKEY_UNINSTALLENDLESS (REGKEY_WIN_UNINSTALL REGKEY_ENDLESS_OS)
#define REGKEY_DISPLAYNAME		L"DisplayName"
#define REGKEY_DISPLAYVERSION	L"DisplayVersion"
#define REGKEY_UNINSTALL_STRING	L"UninstallString"
#define REGKEY_INSTALL_LOCATION	L"InstallLocation"
#define REGKEY_PUBLISHER		L"Publisher"
#define REGKEY_VERSION_MAJOR	L"VersionMajor"
#define REGKEY_VERSION_MINOR	L"VersionMinor"
#define REGKEY_HELP_LINK		L"HelpLink"
#define REGKEY_NOCHANGE			L"NoChange"
#define REGKEY_NOMODIFY			L"NoModify"

#define REGKEY_DISPLAYNAME_TEXT	ENDLESS_OS_NAME
#define REGKEY_PUBLISHER_TEXT	L"Endless Mobile, Inc."

BOOL CEndlessUsbToolDlg::AddUninstallRegistryKeys(const CStringW &uninstallExePath, const CStringW &installPath, DWORD versionMajor, DWORD versionMinor)
{
	CRegKey registryKey;
	LSTATUS result;
	BOOL retResult = FALSE;
	CStringW displayVersion;

	result = registryKey.Create(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS);
	uprintf("Result %d on creating key '%ls'", result, REGKEY_UNINSTALLENDLESS);
	IFFALSE_GOTOERROR(result == ERROR_SUCCESS, "Error on CRegKey::Create.");

	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_DISPLAYNAME, REGKEY_DISPLAYNAME_TEXT, false), "Error on REGKEY_DISPLAYNAME");
	displayVersion.Format(L"%d.%d", versionMajor, versionMinor);
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_DISPLAYVERSION, CComBSTR(displayVersion), false), "Error on REGKEY_DISPLAYVERSION");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_HELP_LINK, UTF8ToBSTR(lmprintf(MSG_314)), false), "Error on REGKEY_HELP_LINK");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_UNINSTALL_STRING, CComBSTR(uninstallExePath), false), "Error on REGKEY_UNINSTALL_STRING");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_INSTALL_LOCATION, CComBSTR(installPath), false), "Error on REGKEY_INSTALL_LOCATION");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_PUBLISHER, REGKEY_PUBLISHER_TEXT, false), "Error on REGKEY_PUBLISHER");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_VERSION_MAJOR, versionMajor, false), "Error on REGKEY_VERSION_MAJOR");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_VERSION_MINOR, versionMinor, false), "Error on REGKEY_VERSION_MINOR");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_NOCHANGE, 1, false), "Error on REGKEY_NOCHANGE");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_NOMODIFY, 1, false), "Error on REGKEY_NOMODIFY");

	retResult = TRUE;
error:

	return retResult;
}

BOOL CEndlessUsbToolDlg::UninstallDualBoot()
{
	FUNCTION_ENTER;

	BOOL retResult = FALSE;
	uint32_t popupMsgId = MSG_363;
	UINT popupStyle = MB_OK | MB_ICONERROR;
	FILE *boottrackImgFile = NULL;

	CString systemDriveLetter = GetSystemDrive();
	CString endlessFilesPath = systemDriveLetter + PATH_ENDLESS_SUBDIRECTORY;
	HANDLE hPhysical = GetPhysicalFromDriveLetter(systemDriveLetter);
	const char *espMountLetter = NULL;
	CString exePath = GetExePath();

	if (IsLegacyBIOSBoot()) { // remove MBR entry
		FAKE_FD fake_fd = { 0 };
		FILE* fp = (FILE*)&fake_fd;
		fake_fd._handle = (char*)hPhysical;
		IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");
		if (nWindowsVersion >= WINDOWS_7) {
			IFFALSE_GOTOERROR(write_win7_mbr(fp), "Error on write_win7_mbr");
		} else if(nWindowsVersion == WINDOWS_VISTA) {
			IFFALSE_GOTOERROR(write_vista_mbr(fp), "Error on write_vista_mbr");
		} else if (nWindowsVersion == WINDOWS_2003 || nWindowsVersion == WINDOWS_XP) {
			IFFALSE_GOTOERROR(write_2000_mbr(fp), "Error on write_2000_mbr");
		} else {
			uprintf("Restore MBR not supported for this windows version '%s', ver %d", WindowsVersionStr, nWindowsVersion);
			goto error;
		}
	} else { // remove EFI entry
		CString windowsEspDriveLetter;
		IFFALSE_PRINTERROR(EFIRequireNeededPrivileges(), "Error on EFIRequireNeededPrivileges.")
		IFFALSE_GOTOERROR(EFIRemoveEntry(ENDLESS_OS_NAME), "Error on EFIRemoveEntry");

		IFFALSE_GOTOERROR(MountESPFromDrive(hPhysical, &espMountLetter, systemDriveLetter), "Error on MountESPFromDrive");
		windowsEspDriveLetter = UTF8ToCString(espMountLetter);
		RemoveNonEmptyDirectory(windowsEspDriveLetter + L"\\" + ENDLESS_BOOT_SUBDIRECTORY);
	}

	// restore the registry key for Windows clock in UTC
	IFFALSE_PRINTERROR(ResetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UTC_TIME_PATH, REGKEY_UTC_TIME), "Error on restoring Windows clock to UTC.");

	// restore Fast Start (aka Hiberboot)
	IFFALSE_PRINTERROR(ResetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FASTBOOT_PATH, REGKEY_FASTBOOT), "Error on restoring fastboot.");

	// remove uninstall entry from registry
	do {
		CRegKey uninstallKey;
		LSTATUS result = uninstallKey.Open(HKEY_LOCAL_MACHINE, REGKEY_WIN_UNINSTALL, KEY_ALL_ACCESS);
		IFFALSE_CONTINUE(result == ERROR_SUCCESS, "Error opening uninstall key");

		result = uninstallKey.DeleteSubKey(REGKEY_ENDLESS_OS);
		IFFALSE_CONTINUE(result == ERROR_SUCCESS, "Error on DeleteSubKey");
	} while (FALSE);


	// remove <SYSDRIVE>:\Endless
	IFFALSE_PRINTERROR(ChangeAccessPermissions(endlessFilesPath, false), "Error on granting Endless files permissions.");
	if (exePath.Find(endlessFilesPath) == 0) {
		DelayDeleteFolder(endlessFilesPath); // maybe we should do the delay delete evrytime to make sure the folder will get deleted?
	} else {
		RemoveNonEmptyDirectory(endlessFilesPath);
	}

	// set success message and icon
	popupMsgId = MSG_362;
	popupStyle = MB_OK | MB_ICONINFORMATION;
	retResult = TRUE;

error:
	safe_closehandle(hPhysical);
	if (espMountLetter != NULL) AltUnmountVolume(espMountLetter);

	AfxMessageBox(UTF8ToCString(lmprintf(popupMsgId)), popupStyle);
	ExitProcess(0);

	return retResult;
}

BOOL CEndlessUsbToolDlg::ResetEndlessRegistryKey(HKEY parentKey, const CString &keyPath, const CString &keyName)
{
	BOOL retResult = FALSE;
	CRegKey registryKey;
	LSTATUS result;
	LONG queryResult;
	DWORD keyType;
	ULONG dataSize = 0;
	uprintf("ResetEndlessRegistryKey called with path '%ls' and key '%ls'", keyPath, keyName);

	result = registryKey.Open(parentKey, keyPath, KEY_ALL_ACCESS);
	IFFALSE_GOTO(result == ERROR_SUCCESS, "Error opening registry key.", done);

	queryResult = registryKey.QueryValue(CString(REGKEY_BACKUP_PREFIX) + keyName, &keyType, NULL, &dataSize);
	IFFALSE_GOTO(queryResult == ERROR_SUCCESS, "Error on QueryValue. Backup value doesn't exist", done);

	switch (keyType) {
	case VT_R4:
		DWORD dwValue;
		result = registryKey.QueryDWORDValue(CString(REGKEY_BACKUP_PREFIX) + keyName, dwValue);
		IFFALSE_BREAK(result == ERROR_SUCCESS, "QueryDWORDValue failed for backup value");

		result = registryKey.DeleteValue(CString(REGKEY_BACKUP_PREFIX) + keyName);
		IFFALSE_PRINTERROR(result == ERROR_SUCCESS, "Error on deleting backup value.");
		if (dwValue == REGKEY_DWORD_MISSING) {
			result = registryKey.DeleteValue(keyName);
			IFFALSE_PRINTERROR(result == ERROR_SUCCESS, "Error on deleting Endless added value.");
		} else {
			result = registryKey.SetDWORDValue(keyName, dwValue);
			IFFALSE_PRINTERROR(result == ERROR_SUCCESS, "Error on setting key to original value.");
		}
		break;
	default:
		uprintf("ERROR: registry key type %d not handled", keyType);
		goto error;
	}

done:
	retResult = TRUE;
error:
	return retResult;

}

BOOL CEndlessUsbToolDlg::MountESPFromDrive(HANDLE hPhysical, const char **espMountLetter, const CString &systemDriveLetter)
{
	FUNCTION_ENTER;

	BOOL retResult = FALSE;
	BYTE layout[4096] = { 0 };
	PDRIVE_LAYOUT_INFORMATION_EX DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)(void*)layout;
	DWORD size;
	BOOL result;

	// get partition layout
	result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, layout, sizeof(layout), &size, NULL);
	IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk layout.");
	IFFALSE_GOTOERROR(DriveLayout->PartitionStyle == PARTITION_STYLE_GPT, "Unexpected partition type. Partition style is not GPT");

	DWORD efiPartitionNumber = -1;
	PARTITION_INFORMATION_EX *partition = NULL;
	for (DWORD index = 0; index < DriveLayout->PartitionCount; index++) {
		partition = &(DriveLayout->PartitionEntry[index]);

		if (partition->Gpt.PartitionType == PARTITION_SYSTEM_GUID) {
			uprintf("Found ESP\r\nPartition %d:\r\n  Type: %s\r\n  Name: '%ls'\r\n ID: %s",
				index + 1, GuidToString(&partition->Gpt.PartitionType), partition->Gpt.Name, GuidToString(&partition->Gpt.PartitionId));
			efiPartitionNumber = partition->PartitionNumber;
			break;
		}
	}
	IFFALSE_GOTOERROR(efiPartitionNumber != -1, "ESP not found.");
	// Fail if EFI partition number is bigger than we can fit in the
	// uin8_t that AltMountVolume receives as parameter for partition number
	IFFALSE_GOTOERROR(efiPartitionNumber <= 0xFF, "EFI partition number is bigger than 255.");

	*espMountLetter = AltMountVolume(ConvertUnicodeToUTF8(systemDriveLetter.Left(2)), (uint8_t)efiPartitionNumber);
	IFFALSE_GOTOERROR(*espMountLetter != NULL, "Error assigning a letter to the ESP.");

	retResult = TRUE;
error:
	return retResult;
}

// A combination of 2 of the options found here: http://www.catch22.net/tuts/self-deleting-executables
// This needs to be tested on all supported OS versions; I tested on Windows 8 and XP and it works
void CEndlessUsbToolDlg::DelayDeleteFolder(const CString &folder)
{
	wchar_t *shortPath = NULL, cmd[MAX_PATH], params[MAX_PATH];
	int timeoutSeconds = 5;

	long  result = GetShortPathName(folder, shortPath, 0);
	IFFALSE_GOTOERROR(result != 0, "Error on getting size needed for GetShortPathName");
	shortPath = new wchar_t[result];

	result = GetShortPathName(folder, shortPath, result);
	IFFALSE_RETURN(result != 0, "Error on GetShortPathName");

	// cmd.exe /q /c "for /l %N in () do timeout 5 >> NUL & @rmdir /S /Q C:\endless >> NUL & if not exist C:\endless exit"
	swprintf_s(params, L"/q /c \"for /l %%N in () do timeout %d >> NUL & @rmdir /S /Q %ls >> NUL & if not exist %ls exit\"", timeoutSeconds, shortPath, shortPath);

	IFFALSE_GOTOERROR(GetEnvironmentVariableW(L"ComSpec", cmd, MAX_PATH) != 0, "Error on GetEnvironmentVariableW");
	uprintf("Running '%ls' with '%ls'", cmd, params);
	INT execResult = (INT)ShellExecute(0, 0, cmd, params, 0, SW_HIDE);
	uprintf("ShellExecute returned %d", execResult);
	IFFALSE_GOTOERROR(execResult > 32, "Error returned by ShellExecute");

error:
	if (shortPath != NULL) {
		delete [] shortPath;
		shortPath = NULL;
	}
}