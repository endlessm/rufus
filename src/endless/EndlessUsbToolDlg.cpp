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

#include "json/json.h"
#include <algorithm>
#include <fstream>
#include "Version.h"
#include "WindowsUsbDefines.h"
#include "StringHelperMethods.h"
#include "Images.h"
#include "WMI.h"

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

char system_dir[MAX_PATH], sysnative_dir[MAX_PATH];
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
PF_TYPE_DECL(WINAPI, BOOL, ChangeWindowMessageFilterEx, (HWND, UINT, DWORD, PCHANGEFILTERSTRUCT));

BOOL FormatDrive(DWORD DriveIndex, int fsToUse, const wchar_t *partLabel);

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
#define ELEMENT_ADVANCED_PAGE           "AdvancedPage"
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
#define ELEMENT_DUALBOOT_ADVANCED_TEXT	"AdvancedOptionsText"
#define ELEMENT_DUALBOOT_ADVANCED_LINK  "AdvancedOptionsLink"
#define ELEMENT_DUALBOOT_INSTALL_BUTTON "DualBootInstallButton"
#define ELEMENT_DUALBOOT_TITLE			"DualBootContentTitle"
#define ELEMENT_DUALBOOT_DESCRIPTION	"DualBootContentDescription"
#define ELEMENT_DUALBOOT_RECOMMENDATION	"DualBootRecommendation"

//Advanced page elements
#define ELEMENT_LIVE_USB_BUTTON         "LiveUsbButton"
#define ELEMENT_REFORMATTER_USB_BUTTON  "ReformatterUsbButton"
#define ELEMENT_COMPARE_OPTIONS         "CompareOptionsLink"
#define ELEMENT_ADVANCED_CLOSE_BUTTON   "AdvancedPageCloseButton"
#define ELEMENT_ADVANCED_PREV_BUTTON    "AdvancedPagePreviousButton"

#define ELEMENT_ADVOPT_SUBTITLE			"AdvOptSubtitleContainer"
#define ELEMENT_COMBINED_USB_BUTTON		"CombinedUsbButton"
#define ELEMENT_USB_LEARN_MORE			"USBLearnMore"

//Select File page elements
#define ELEMENT_SELFILE_PREV_BUTTON     "SelectFilePreviousButton"
#define ELEMENT_SELFILE_NEXT_BUTTON     "SelectFileNextButton"
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
#define ELEMENT_STORAGE_AGREEMENT_TEXT	"StorageAgreementText"
#define ELEMENT_STORAGE_SPACE_WARNING   "StorageSpaceWarning"
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

#define ELEMENT_VERSION_LINK            "VersionLink"

// Javascript methods
#define JS_SET_PROGRESS                 "setProgress"
#define JS_ENABLE_DOWNLOAD              "enableDownload"
#define JS_ENABLE_ELEMENT               "enableElement"
#define JS_ENABLE_BUTTON                "enableButton"
#define JS_SHOW_ELEMENT                 "showElement"
#define JS_RESET_CHECK                  "resetCheck"
#define JS_SET_CODING_MODE              "setCodingMode"

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
#define PERSONALITY_SPANISH_MX          L"es_MX"
#define PERSONALITY_INDONESIAN          L"id"
#define PERSONALITY_THAI                L"th"
#define PERSONALITY_VIETNAMESE          L"vi"

// For OEM images, this is the union of id, th and vi. Historically it was
// available for download users too, but as of 3.1.4 we build separate images.
#define PERSONALITY_SOUTHEAST_ASIA      L"sea"

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
    PERSONALITY_SPANISH_MX,
    PERSONALITY_INDONESIAN,
    PERSONALITY_THAI,
    PERSONALITY_VIETNAMESE,
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
#define RUFUS_LOCALE_VI     "vi-VN"
#define RUFUS_LOCALE_TH     "th-TH"
#define RUFUS_LOCALE_ID     "id-ID"

// INI file language codes
#define INI_LOCALE_EN       "en_US.utf8"
#define INI_LOCALE_ES       "es_MX.utf8"
#define INI_LOCALE_PT       "pt_BR.utf8"
#define INI_LOCALE_SA       "ar_AE.utf8"
#define INI_LOCALE_FR       "fr_FR.utf8"
#define INI_LOCALE_ZH       "zh_CN.utf8"
#define INI_LOCALE_BN       "bn_BD.utf8"
#define INI_LOCALE_VI       "vi_VN.utf8"
#define INI_LOCALE_TH       "th_TH.utf8"
#define INI_LOCALE_ID       "id_ID.utf8"

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
	OP_NEW_LIVE_CREATION,
    OP_NO_OPERATION_IN_PROGRESS,
    OP_ENDLESS_MAX
};

#define TID_UPDATE_FILES                TID_REFRESH_TIMER + 1

#define ENABLE_JSON_COMPRESSION 1

#define BOOT_COMPONENTS_FOLDER	"EndlessBoot"

#define FILE_GZ_EXTENSION	"gz"
#define FILE_XZ_EXTENSION	"xz"

#define RELEASE_JSON_URLPATH       _T("https://d1anzknqnc1kmb.cloudfront.net/")
#define PRIVATE_JSON_URLPATH       _T("http://images.endlessm-sf.com/")

#define JSON_LIVE_FILE             "releases-eos-3.json"
#define JSON_CODING_LIVE_FILE      "releases-coding-3.json"
#define JSON_INSTALLER_FILE        "releases-eosinstaller-3.json"
#ifdef ENABLE_JSON_COMPRESSION
# define JSON_SUFFIX               "." FILE_GZ_EXTENSION
#else
# define JSON_SUFFIX               ""
#endif // ENABLE_JSON_COMPRESSION

#define SIGNATURE_FILE_EXT         L".asc"
#define	BOOT_ARCHIVE_SUFFIX		   L".boot.zip"
#define	IMAGE_FILE_EXT			   L".img"

#define ENDLESS_OS "Endless OS"
const wchar_t* mainWindowTitle = L"Endless Installer";

#define ALL_FILES					L"*.*"

#define ENDLESS_OS_NAME L"Endless OS"

// Radu: How much do we need to reserve for the exfat partition header?
// reserve 10 mb for now; this will also include the signature file
#define INSTALLER_DELTA_SIZE (10*1024*1024)

#define BYTES_IN_MEGABYTE		(1024 *  1024)
#define BYTES_IN_GIGABYTE		(1024 *  1024 * 1024)

#define UPDATE_DOWNLOAD_PROGRESS_TIME       2000
#define CHECK_INTERNET_CONNECTION_TIME      2000

#define MIN_SUPPORTED_IE_VERSION		8

#define FORMAT_STATUS_CANCEL (ERROR_SEVERITY_ERROR | FAC(FACILITY_STORAGE) | ERROR_CANCELLED)

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
	TOSTR(OP_NEW_LIVE_CREATION);
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
        TOSTR(ErrorCauseCancelled);
        TOSTR(ErrorCauseDownloadFailed);
        TOSTR(ErrorCauseDownloadFailedDiskFull);
        TOSTR(ErrorCauseVerificationFailed);
        TOSTR(ErrorCauseWriteFailed);
        TOSTR(ErrorCauseNot64Bit);
        TOSTR(ErrorCause32BitEFI);
        TOSTR(ErrorCauseBitLocker);
        TOSTR(ErrorCauseNotNTFS);
        TOSTR(ErrorCauseNonWindowsMBR);
        TOSTR(ErrorCauseNonEndlessMBR);
        TOSTR(ErrorCauseCantCheckMBR);
        TOSTR(ErrorCauseInstallFailedDiskFull);
        TOSTR(ErrorCauseSuspended);
        TOSTR(ErrorCauseNone);
        default: return _T("Error Cause Unknown");
    }
}

static LPCTSTR InstallMethodToStr(InstallMethod_t installMethod)
{
    switch (installMethod)
    {
        TOSTR(None);
        TOSTR(LiveUsb);
        TOSTR(ReformatterUsb);
        TOSTR(CombinedUsb);
        TOSTR(InstallDualBoot);
        TOSTR(UninstallDualBoot);
        default: return _T("Unknown");
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
    DHTML_EVENT_ONCLICK(_T(ELEMENT_VERSION_LINK), OnLinkClicked)

	// Advanced Page Handlers
    DHTML_EVENT_ONCLICK(_T(ELEMENT_LIVE_USB_BUTTON), OnLiveUsbClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_REFORMATTER_USB_BUTTON), OnReformatterUsbClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_COMPARE_OPTIONS), OnLinkClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_ADVANCED_CLOSE_BUTTON), OnCloseAppClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_ADVANCED_PREV_BUTTON), OnAdvancedPagePreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_COMBINED_USB_BUTTON), OnCombinedUsbButtonClicked)
    DHTML_EVENT_ONCLICK(_T(ELEMENT_USB_LEARN_MORE), OnLinkClicked)

	// Select File Page handlers
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_PREV_BUTTON), OnSelectFilePreviousClicked)
	DHTML_EVENT_ONCLICK(_T(ELEMENT_SELFILE_NEXT_BUTTON), OnSelectFileNextClicked)
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
CList<CString> CEndlessUsbToolDlg::m_seaPersonalities;
CMap<CStringA, LPCSTR, CStringA, LPCSTR> CEndlessUsbToolDlg::m_localeToIniLocale;

int CEndlessUsbToolDlg::ImageUnpackOperation;
int CEndlessUsbToolDlg::ImageUnpackPercentStart;
int CEndlessUsbToolDlg::ImageUnpackPercentEnd;
ULONGLONG CEndlessUsbToolDlg::ImageUnpackFileSize;

const UINT CEndlessUsbToolDlg::m_uTaskbarBtnCreatedMsg = RegisterWindowMessage(_T("TaskbarButtonCreated"));

CEndlessUsbToolDlg::CEndlessUsbToolDlg(UINT globalMessage, CWnd* pParent /*=NULL*/)
    : CDHtmlDialog(IDD_ENDLESSUSBTOOL_DIALOG, IDR_HTML_ENDLESSUSBTOOL_DIALOG, pParent),
    m_initialized(false),
    m_selectedLocale(NULL),
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
    m_lastErrorCause(ErrorCause_t::ErrorCauseNone),
    m_localFilesScanned(false),
    m_jsonDownloadState(JSONDownloadState::Pending),
	m_selectedInstallMethod(InstallMethod_t::None)
{
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
    m_localeToPersonality[RUFUS_LOCALE_ID] = PERSONALITY_INDONESIAN;
    m_localeToPersonality[RUFUS_LOCALE_TH] = PERSONALITY_THAI;
    m_localeToPersonality[RUFUS_LOCALE_VI] = PERSONALITY_VIETNAMESE;

    m_seaPersonalities.AddTail(PERSONALITY_INDONESIAN);
    m_seaPersonalities.AddTail(PERSONALITY_THAI);
    m_seaPersonalities.AddTail(PERSONALITY_VIETNAMESE);

    m_localeToIniLocale[RUFUS_LOCALE_EN] = INI_LOCALE_EN;
    m_localeToIniLocale[RUFUS_LOCALE_ES] = INI_LOCALE_ES;
    m_localeToIniLocale[RUFUS_LOCALE_PT] = INI_LOCALE_PT;
    m_localeToIniLocale[RUFUS_LOCALE_SA] = INI_LOCALE_SA;
    m_localeToIniLocale[RUFUS_LOCALE_FR] = INI_LOCALE_FR;
    m_localeToIniLocale[RUFUS_LOCALE_ZH_CN] = INI_LOCALE_ZH;
    m_localeToIniLocale[RUFUS_LOCALE_BN] = INI_LOCALE_BN;
    m_localeToIniLocale[RUFUS_LOCALE_ID] = INI_LOCALE_ID;
    m_localeToIniLocale[RUFUS_LOCALE_TH] = INI_LOCALE_TH;
    m_localeToIniLocale[RUFUS_LOCALE_VI] = INI_LOCALE_VI;
}

CEndlessUsbToolDlg::~CEndlessUsbToolDlg() {
    FUNCTION_ENTER;
}

void CEndlessUsbToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CEndlessUsbToolDlg, CDHtmlDialog)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

const IID my_IID_ITaskbarList3 =
{ 0xea1afb91, 0x9e28, 0x4b86,{ 0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf } };
const IID my_CLSID_TaskbarList =
{ 0x56fdf344, 0xfd6d, 0x11d0,{ 0x95, 0x8a ,0x0, 0x60, 0x97, 0xc9, 0xa0 ,0x90 } };

LRESULT CEndlessUsbToolDlg::OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam)
{
	if (nWindowsVersion >= WINDOWS_7) {
		m_taskbarProgress.Release();
		//CoCreateInstance(my_CLSID_TaskbarList, NULL, CLSCTX_ALL, my_IID_ITaskbarList3, (void**)&m_taskbarProgress);
		HRESULT hr = m_taskbarProgress.CoCreateInstance(my_CLSID_TaskbarList);
		IFFALSE_GOTO(SUCCEEDED(hr), "Error creating progressbar.", done);
	}

done:
	return 0;
}

// Browse navigation handling methods
void CEndlessUsbToolDlg::OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl)
{
    FUNCTION_ENTER;

	CDHtmlDialog::OnDocumentComplete(pDisp, szUrl);

	uprintf("OnDocumentComplete '%ls'", szUrl);

	IFTRUE_RETURN(m_selectedInstallMethod == InstallMethod_t::UninstallDualBoot, "Returning as we are in uninstall mode");

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
        CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_REFORMATTER_USB_BUTTON))), CComVariant(FALSE));
    }

	CallJavascript(_T(JS_SET_CODING_MODE), CComVariant(IsCoding()));
    SetElementText(_T(ELEMENT_VERSION_LINK), CComBSTR(RELEASE_VER_STR));

    StartCheckInternetConnectionThread();
    FindMaxUSBSpeed();

	UpdateDualBootTexts();

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

	m_initialized = true;

	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };

	PF_INIT(ChangeWindowMessageFilterEx, user32);
	if (nWindowsVersion >= WINDOWS_7 && pfChangeWindowMessageFilterEx != NULL) {
		pfChangeWindowMessageFilterEx(m_hWnd, m_uTaskbarBtnCreatedMsg, MSGFLT_ALLOW, &cfs);
	}

	if(CEndlessUsbToolApp::m_enableLogDebugging) hLog = m_hWnd;

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
    IFFALSE_PRINTERROR(result, "DownloadManager.Init failed");

    result = m_iso.Init();
    IFFALSE_PRINTERROR(result, "EndlessISO.Init failed");

    SetWindowTextW(L"");

    // set up manufacturer/model and firmware dimensions before further interacting with Analytics
    CString manufacturer, model;
    IFFALSE_PRINTERROR(WMI::GetMachineInfo(manufacturer, model), "Couldn't query manufacturer & model");
    uprintf("System manufacturer: %ls; Model: %ls", manufacturer, model);
    Analytics::instance()->setManufacturerModel(manufacturer, model);

    auto isBIOS = IsLegacyBIOSBoot();
    Analytics::instance()->setFirmware(isBIOS ? L"BIOS" : L"EFI");

    Analytics::instance()->startSession();
    TrackEvent(_T("IEVersion"), m_ieVersion);

    if (m_ieVersion < MIN_SUPPORTED_IE_VERSION) {
        ShowIETooOldError();
        EndDialog(IDCANCEL);
    }

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEndlessUsbToolDlg::ShowIETooOldError()
{
    CString message = UTF8ToCString(lmprintf(MSG_368, m_ieVersion, MIN_SUPPORTED_IE_VERSION));
    int result = AfxMessageBox(message, MB_OKCANCEL | MB_ICONERROR);
    if (result == IDOK) {
        const char *command;
        if (nWindowsVersion >= WINDOWS_VISTA) {
            // https://msdn.microsoft.com/en-us/library/cc144191(VS.85).aspx
            command = "c:\\windows\\system32\\control.exe /name Microsoft.WindowsUpdate";
        } else {
            // This is what the Windows Update item in the Start menu points to
            command = "c:\\windows\\system32\\wupdmgr.exe";
        }

        UINT ret = WinExec(command, SW_NORMAL);
        if (ret <= 31) {
            uprintf("WinExec('%s', SW_NORMAL) failed: %d", command, ret);
        }
    }
}

#define THREADS_WAIT_TIMEOUT 10000 // 10 seconds
void CEndlessUsbToolDlg::Uninit()
{
    FUNCTION_ENTER;

    IFFALSE_RETURN(m_initialized, "not yet initialized; or already uninitialized");
    m_initialized = false;

    int handlesCount = 0;
    HANDLE handlesToWaitFor[5];
    
    HANDLE analyticsHandle = Analytics::instance()->stopSession(InstallMethodToStr(m_selectedInstallMethod));

#define await_handle(h) do { \
	if (h != INVALID_HANDLE_VALUE) handlesToWaitFor[handlesCount++] = h; \
} while (0)
    await_handle(analyticsHandle);
    await_handle(m_fileScanThread);
    await_handle(m_operationThread);
    await_handle(m_downloadUpdateThread);
    await_handle(m_checkConnectionThread);
#undef await_handle

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

    m_iso.Uninit();
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
    if (message == m_uTaskbarBtnCreatedMsg) {
        OnTaskbarBtnCreated(wParam, lParam);
    } else if (message >= CB_GETEDITSEL && message < CB_MSGMAX) {
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
    } else if (message >= EM_GETSEL && message < EM_GETIMESTATUS) {
        // Handle messages sent to the log window
        // Do not use any method that calls _uprintf as it will generate an infinite recursive loop
        // Use OutputDebugString instead.
        switch (message) {
            case EM_SETSEL:
            {
                char *logMessage = (char*)lParam;
				CEndlessUsbToolApp::Log(logMessage);
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
            if (!img_report.is_bootable_img) {
                uprintf("FAILURE: selected image is not bootable");
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
            if ((m_currentStep == OP_FILE_COPY || m_currentStep == OP_NEW_LIVE_CREATION) && (op == OP_FORMAT || op == OP_CREATE_FS)) break;
            
            // Radu: maybe divide the progress bar also based on the size of the image to be copied to disk after format is complete
            if (op == OP_FORMAT && m_selectedInstallMethod == InstallMethod_t::ReformatterUsb) {
                percent = percent / 2;
            } else if (op == OP_FILE_COPY) {
                percent = 50 + percent / 2;
            }

            if (percent >= 0 && percent <= 100) {
                hr = CallJavascript(_T(JS_SET_PROGRESS), CComVariant(percent));
                IFFALSE_BREAK(SUCCEEDED(hr), "Error when calling set progress.");

				if (m_taskbarProgress != NULL) {
					m_taskbarProgress->SetProgressState(m_hWnd, TBPF_NORMAL);
					m_taskbarProgress->SetProgressValue(m_hWnd, percent, 100);
				}
            }

            if (op == OP_VERIFYING_SIGNATURE || op == OP_FORMAT || op == OP_FILE_COPY || op == OP_SETUP_DUALBOOT || op == OP_NEW_LIVE_CREATION) {
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
            if (!IS_ERROR(FormatStatus) && m_selectedInstallMethod == InstallMethod_t::ReformatterUsb) {
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
				if (isReleaseJsonDownload) {
					uprintf("JSON download failed.");
					SetJSONDownloadState(JSONDownloadState::Failed);
				} else if (m_lastErrorCause == ErrorCause_t::ErrorCauseCancelled) {
					uprintf("Download cancelled by user request.");
				} else {
					uprintf("Error on download.");
					bool diskFullError = (downloadStatus->errorContext == BG_ERROR_CONTEXT_LOCAL_FILE);
					diskFullError = diskFullError && (downloadStatus->errorCode == HRESULT_FROM_WIN32(ERROR_DISK_FULL));
					m_lastErrorCause = diskFullError ? ErrorCause_t::ErrorCauseDownloadFailedDiskFull : ErrorCause_t::ErrorCauseDownloadFailed;
					m_downloadManager.ClearExtraDownloadJobs();
					ErrorOccured(m_lastErrorCause);
				}
            } else if (downloadStatus->done) {
                uprintf("Download done for %ls", downloadStatus->jobName);

                if (isReleaseJsonDownload) {
                    if (m_jsonDownloadState != JSONDownloadState::Succeeded) {
                        UpdateDownloadOptions();
                    }
                } else if (m_currentStep == OP_DOWNLOADING_FILES) {
                    // ->done can be signaled more than once; only proceed to verification the first time
                    StartFileVerificationThread();
                }
            } else {
				CStringA part, total;
				part = SizeToHumanReadable(downloadStatus->progress.BytesTransferred, FALSE, use_fake_units);
				total = SizeToHumanReadable(downloadStatus->progress.BytesTotal, FALSE, use_fake_units);
                uprintf("Download [%ls] progress %s of %s (%d of %d files) %s", downloadStatus->jobName,
                    part, total,
                    downloadStatus->progress.FilesTransferred, downloadStatus->progress.FilesTotal,
                    downloadStatus->transientError ? "(transient error)" : "");

                if (isReleaseJsonDownload) {
                    if (downloadStatus->transientError) {
                        SetJSONDownloadState(JSONDownloadState::Retrying);
                    }
                    // Don't clear this on the downwards edge. If the link is up but the connection times out, the
                    // download will enter state TRANSIENT_ERROR and then immediately re-enter state CONNECTING.
                    // We want to give some indication that some error has occured, even as we retry.
                } else {
                    static ULONGLONG startedTickCount = 0;
                    static ULONGLONG startedBytes = 0;

                    RemoteImageEntry_t remote = m_remoteImages.GetAt(m_remoteImages.FindIndex(m_selectedRemoteIndex));
                    // we don't take the signature files into account but we are taking about ~2KB compared to >2GB
                    ULONGLONG totalSize = downloadStatus->progress.BytesTotal; //GetActualDownloadSize(remote);
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
                    CStringA strTotal = totalSize == BG_SIZE_UNKNOWN ? "---" : SizeToHumanReadable(totalSize, FALSE, use_fake_units);
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
                if (IsDualBootOrCombinedUsb()) {
                    m_cancelImageUnpack = 0;
                    if (m_selectedInstallMethod == InstallMethod_t::CombinedUsb) {
                        StartOperationThread(OP_NEW_LIVE_CREATION, CEndlessUsbToolDlg::CreateUSBStick);
                    } else {
                        StartOperationThread(OP_SETUP_DUALBOOT, CEndlessUsbToolDlg::SetupDualBoot);
                    }
                } else {
                    // RufusISOScanThread and FormatThread use a global variable for the image path
                    CString diskImage = m_localFile;
                    if (m_selectedInstallMethod == InstallMethod_t::ReformatterUsb) {
                        diskImage = m_localInstallerImage.filePath;
                    }

                    safe_free(image_path);
                    image_path = wchar_to_utf8(diskImage);
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

			if (m_selectedInstallMethod != InstallMethod_t::InstallDualBoot) ChangeDriveAutoRunAndMount(false);

            switch (m_lastErrorCause) {
            case ErrorCause_t::ErrorCauseNone:
                TrackEvent(_T("Completed"));
                PrintInfo(0, MSG_210);
                m_operationThread = INVALID_HANDLE_VALUE;
				if (m_taskbarProgress != NULL) {
					m_taskbarProgress->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
					m_taskbarProgress->SetProgressValue(m_hWnd, 0, 100);
				}
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
            }

            UpdateDownloadableState();
            break;
        }

        case WM_POWERBROADCAST:
        {
            uprintf("Received WM_POWERBROADCAST with WPARAM 0x%X LPARAM 0x%X", wParam, lParam);
            if (m_currentStep == OP_NO_OPERATION_IN_PROGRESS) {
                uprintf("no operation in progress, ignoring");
                return TRUE;
            }

            switch (wParam) {
            case PBT_APMQUERYSUSPEND:
                // Windows XP only. Vista and later do not send PBT_APMQUERYSUSPEND;
                // we attempt to inhibit suspend in EnableHibernate with the newer API.
                uprintf("Received WM_POWERBROADCAST with PBT_APMQUERYSUSPEND and trying to cancel it.");
                return BROADCAST_QUERY_DENY;

            case PBT_APMSUSPEND:
                uprintf("Received PBT_APMSUSPEND so canceling the operation.");
                m_lastErrorCause = ErrorCause_t::ErrorCauseSuspended;
                CancelRunningOperation();
                break;
            }

            return TRUE;
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

	Analytics::instance()->setLanguage(CString(m_selectedLocale->txt[0]));

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

	hr = GetSelectElement(_T(ELEMENT_LANGUAGE_DUALBOOT), selectElementDualBoot);
	IFFALSE_RETURN(SUCCEEDED(hr) && selectElementDualBoot != NULL, "Error returned from GetSelectElement.");

	hr = selectElementDualBoot->put_length(0);

	int index = 0;
	// Add languages to dropdown and apply localization
	list_for_each_entry(lcmd, &locale_list, loc_cmd, list) {
		luprintf("Language available : %s", lcmd->txt[1]);

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

#define MSG_RECOVER_RESUME          MSG_326
#define MSG_RECOVER_DOWNLOAD_AGAIN  MSG_327
#define MSG_RECOVER_TRY_AGAIN       MSG_328

void CEndlessUsbToolDlg::ErrorOccured(ErrorCause_t errorCause)
{
    uint32_t recoverButtonMsgId = 0, suggestionMsgId = 0, headlineMsgId = IsCoding() ? MSG_381 : MSG_370;
    bool driveLetterInHeading = false;

    switch (errorCause) {
    case ErrorCause_t::ErrorCauseDownloadFailed:
        recoverButtonMsgId = MSG_RECOVER_RESUME;
        suggestionMsgId = IsCoding() ? MSG_382 : MSG_323;
        break;
    case ErrorCause_t::ErrorCauseDownloadFailedDiskFull:
        recoverButtonMsgId = MSG_RECOVER_RESUME;
        headlineMsgId = MSG_350;
        suggestionMsgId = IsCoding() ? MSG_303 : MSG_334;
	break;
    case ErrorCause_t::ErrorCauseInstallFailedDiskFull:
        recoverButtonMsgId = MSG_RECOVER_RESUME;
        headlineMsgId = MSG_350;
        suggestionMsgId = IsCoding() ? MSG_301 : MSG_351;
        break;
    case ErrorCause_t::ErrorCauseVerificationFailed:
        recoverButtonMsgId = MSG_RECOVER_DOWNLOAD_AGAIN;
        suggestionMsgId = IsCoding() ? MSG_383 : MSG_324;
        break;
    case ErrorCause_t::ErrorCauseCancelled:
    case ErrorCause_t::ErrorCauseGeneric:
    case ErrorCause_t::ErrorCauseWriteFailed:
    case ErrorCause_t::ErrorCauseSuspended: // TODO: new string here
        recoverButtonMsgId = MSG_RECOVER_TRY_AGAIN;
        suggestionMsgId = m_selectedInstallMethod == InstallMethod_t::InstallDualBoot
	    ? (IsCoding() ? MSG_385 : MSG_358)
	    : (IsCoding() ? MSG_384 : MSG_325);
        break;
    case ErrorCause_t::ErrorCauseNot64Bit:
    case ErrorCause_t::ErrorCause32BitEFI:
        headlineMsgId = IsCoding() ? MSG_313 : MSG_354;
        suggestionMsgId = IsCoding() ? MSG_375 : MSG_355;
        break;
    case ErrorCause_t::ErrorCauseBitLocker:
        headlineMsgId = MSG_356;
        driveLetterInHeading = true;
        suggestionMsgId = IsCoding() ? MSG_378 : MSG_357;
        break;
    case ErrorCause_t::ErrorCauseNotNTFS:
        headlineMsgId = MSG_352;
        driveLetterInHeading = true;
        suggestionMsgId = IsCoding() ? MSG_308 : MSG_353;
        break;
    case ErrorCause_t::ErrorCauseNonWindowsMBR:
        headlineMsgId = MSG_359;
        suggestionMsgId = IsCoding() ? MSG_379 : MSG_360;
        break;
    case ErrorCause_t::ErrorCauseCantCheckMBR:
        // TODO: new string here; or, better, eliminate this failure mode
        suggestionMsgId = MSG_358;
        break;
    default:
        uprintf("Unhandled error cause %ls(%d)", ErrorCauseToStr(errorCause), errorCause);
        break;
    }

    // Update the error button text if it's a "recoverable" error case or hide it otherwise
    if (recoverButtonMsgId != 0) {
        SetElementText(_T(ELEMENT_ERROR_BUTTON), UTF8ToBSTR(lmprintf(recoverButtonMsgId)));
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_ERROR_BUTTON)), CComVariant(TRUE));
    } else {
        CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_ERROR_BUTTON)), CComVariant(FALSE));
    }

    if (headlineMsgId != 0) {
        CComBSTR headlineMsg;
        if (driveLetterInHeading) headlineMsg = UTF8ToBSTR(lmprintf(headlineMsgId, ConvertUnicodeToUTF8(GetSystemDrive())));
        else headlineMsg = UTF8ToBSTR(lmprintf(headlineMsgId));
        SetElementText(_T(ELEMENT_ERROR_MESSAGE), headlineMsg);
    }

    // Ask user to delete the file that didn't pass signature verification
    // Trying again with the same file will result in the same verification error
    bool fileVerificationFailed = (errorCause == ErrorCause_t::ErrorCauseVerificationFailed);
    CComBSTR deleteFilesText("");
    if (fileVerificationFailed) {
        CSingleLock lock(&m_verifyFilesMutex);
        lock.Lock();
        ULONGLONG invalidFileSize = 0;

        if (m_verifyFiles.empty()) {
            uprintf("Verification failed, but no file is invalid(?)");
        } else {
            auto path = m_verifyFiles.front().filePath;
            try {
                CFile file(path, CFile::modeRead | CFile::shareDenyNone);
                invalidFileSize = file.GetLength();
            }
            catch (CFileException *ex) {
                TCHAR szError[1024];
                ex->GetErrorMessage(szError, ARRAYSIZE(szError));
                uprintf("CFileException on file [%ls] with cause [%d] and OS error [%d]: %ls", path, ex->m_cause, ex->m_lOsError, szError);
                ex->Delete();
            }
        }
        deleteFilesText = UTF8ToBSTR(lmprintf(MSG_336, SizeToHumanReadable(invalidFileSize, FALSE, use_fake_units)));
        CallJavascript(_T(JS_RESET_CHECK), CComVariant(_T(ELEMENT_ERROR_DELETE_CHECKBOX)));
    }
    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_ERROR_BUTTON))), CComVariant(!fileVerificationFailed));
    CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_ERROR_DELETE_CHECKBOX), CComVariant(fileVerificationFailed));
    SetElementText(_T(ELEMENT_ERROR_DELETE_TEXT), deleteFilesText);

    // Update the error description and "recovery" suggestion
    if (suggestionMsgId != 0) {
        CComBSTR message;
        if (suggestionMsgId == MSG_334 || suggestionMsgId == MSG_303) {
            POSITION p = m_remoteImages.FindIndex(m_selectedRemoteIndex);
            ULONGLONG size = 0;
            if (p != NULL) {
                RemoteImageEntry_t remote = m_remoteImages.GetAt(p);
                size = remote.compressedSize;
            }
            // we don't take the signature files into account but we are taking about ~2KB compared to >2GB
            ULONGLONG totalSize = size + (m_selectedInstallMethod == InstallMethod_t::ReformatterUsb ? m_installerImage.compressedSize : 0);
            message = UTF8ToBSTR(lmprintf(suggestionMsgId, SizeToHumanReadable(totalSize, FALSE, use_fake_units)));
        } else if(suggestionMsgId == MSG_351 || suggestionMsgId == MSG_301) {
            int nrGigsNeeded;
            ULONGLONG neededSize = GetNeededSpaceForDualBoot(nrGigsNeeded);
            message = UTF8ToBSTR(lmprintf(suggestionMsgId, SizeToHumanReadable(neededSize, FALSE, use_fake_units), ConvertUnicodeToUTF8(GetSystemDrive())));
        } else {
            message = UTF8ToBSTR(lmprintf(suggestionMsgId));
        }
        SetElementText(_T(ELEMENT_ERROR_SUGGESTION), message);
    }

	if (m_taskbarProgress != NULL) {
		m_taskbarProgress->SetProgressState(m_hWnd, TBPF_ERROR);
		m_taskbarProgress->SetProgressValue(m_hWnd, 100, 100);
	}

    ChangePage(_T(ELEMENT_ERROR_PAGE));

	if (errorCause == ErrorCause_t::ErrorCauseCancelled)
		TrackEvent(_T("Cancelled"));
	else
		TrackEvent(_T("Failed"), ErrorCauseToStr(errorCause));

	bool fatal = FALSE;
	switch (errorCause) {
	case ErrorCause_t::ErrorCauseDownloadFailed:
	case ErrorCause_t::ErrorCauseVerificationFailed:
	case ErrorCause_t::ErrorCauseWriteFailed:
		fatal = FALSE;
		break;
	default:
		fatal = TRUE;
		break;
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

HRESULT CEndlessUsbToolDlg::UpdateSelectOptionText(CComPtr<IHTMLSelectElement> selectElement, const CString &text, LONG index)
{
	CComPtr<IDispatch> pDispatch;
	CComPtr<IHTMLOptionElement> pOption;
	CComBSTR valueBSTR;

	HRESULT hr = selectElement->item(CComVariant(index), CComVariant(index), &pDispatch);
	IFFALSE_GOTOERROR(SUCCEEDED(hr) && pDispatch != NULL, "Error when querying for element at requested index.");

	hr = pDispatch.QueryInterface<IHTMLOptionElement>(&pOption);
	IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error when querying for IHTMLOptionElement interface");

	hr = pOption->put_text(CComBSTR(text));
	IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error on IHTMLOptionElement::put_text");

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

void CEndlessUsbToolDlg::TrackEvent(const CString &action, const CString &label, LONGLONG value)
{
	Analytics::instance()->eventTracking(InstallMethodToStr(m_selectedInstallMethod), action, label, value);
}

void CEndlessUsbToolDlg::TrackEvent(const CString &action, LONGLONG value)
{
	CString label;
	label.Format(_T("%I64i"), value);
	Analytics::instance()->eventTracking(InstallMethodToStr(m_selectedInstallMethod), action, label, value);
}

void CEndlessUsbToolDlg::SetSelectedInstallMethod(InstallMethod_t method)
{
	if (m_selectedInstallMethod == method) return;
	m_selectedInstallMethod = method;
	TrackEvent(_T("Selected"));
}

#define KEY_PRESSED 0x8000
// Dual Boot Page Handlers
HRESULT CEndlessUsbToolDlg::OnAdvancedOptionsClicked(IHTMLElement* pElement)
{
	FUNCTION_ENTER;

	SHORT keyState = GetKeyState(VK_CONTROL);
	bool oldStyleUSB = ((keyState & KEY_PRESSED) != 0) || (nWindowsVersion == WINDOWS_XP);

	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_ADVOPT_SUBTITLE), CComVariant(!oldStyleUSB));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_COMBINED_USB_BUTTON)), CComVariant(!oldStyleUSB));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_USB_LEARN_MORE), CComVariant(!oldStyleUSB));

	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_REFORMATTER_USB_BUTTON)), CComVariant(oldStyleUSB));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(HTML_BUTTON_ID(ELEMENT_LIVE_USB_BUTTON)), CComVariant(oldStyleUSB));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_COMPARE_OPTIONS), CComVariant(oldStyleUSB));

	ChangePage(_T(ELEMENT_ADVANCED_PAGE));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnInstallDualBootClicked(IHTMLElement* pElement)
{
	IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnInstallDualBootClicked: Button is disabled. ", S_OK);

	FUNCTION_ENTER;

	CString systemDriveLetter = GetSystemDrive();

	BOOL x64BitSupported = Has64BitSupport() ? TRUE : FALSE;
	uprintf("HW processor has 64 bit support: %s", x64BitSupported ? "YES" : "NO");

	BOOL isBitLockerEnabled = WMI::IsBitlockedDrive(systemDriveLetter.Left(2));
	uprintf("Is BitLocker/device encryption enabled on '%ls': %s", systemDriveLetter, isBitLockerEnabled ? "YES" : "NO");

	bool isBIOS = IsLegacyBIOSBoot();
	ErrorCause cause = ErrorCauseNone;
	// TODO: move bitlocker check in here too?
	bool canInstall = CanInstallToDrive(systemDriveLetter, isBIOS, cause);

	if (ShouldUninstall()) {
		QueryAndDoUninstall();
		return S_OK;
	}

	SetSelectedInstallMethod(InstallMethod_t::InstallDualBoot);

	if (!x64BitSupported) {
		ErrorOccured(ErrorCauseNot64Bit);
	} else if (isBitLockerEnabled) {
		ErrorOccured(ErrorCauseBitLocker);
	} else if (!canInstall) {
		ErrorOccured(cause);
	} else {
		GoToSelectFilePage(true);
	}

	return S_OK;
}

// Advanced Page Handlers
HRESULT CEndlessUsbToolDlg::OnLiveUsbClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    SetSelectedInstallMethod(InstallMethod_t::LiveUsb);

    GoToSelectFilePage(true);

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnReformatterUsbClicked(IHTMLElement* pElement)
{
    IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnReformatterUsbClicked: Button is disabled. ", S_OK);

    FUNCTION_ENTER;

    SetSelectedInstallMethod(InstallMethod_t::ReformatterUsb);

    GoToSelectFilePage(true);

    return S_OK;
}

void CEndlessUsbToolDlg::GoToSelectFilePage(bool forwards)
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
        SetElementText(_T(ELEMENT_SET_FILE_TITLE), UTF8ToBSTR(lmprintf(IsCoding() ? MSG_373 : MSG_321)));
        SetElementText(_T(ELEMENT_SET_FILE_SUBTITLE), UTF8ToBSTR(lmprintf(MSG_322)));
    } else if(CanUseRemoteFile()) {
        ApplyRufusLocalization();
    } else {
        uprintf("No remote images available and no local images available.");
    }

    // Update page with no local files found
    POSITION p = m_remoteImages.FindIndex(m_baseImageRemoteIndex);
    if (p != NULL) {
        r = m_remoteImages.GetAt(p);

        // Update light download size
        SetElementText(_T(ELEMENT_DOWNLOAD_LIGHT_SIZE), GetDownloadString(r));
    }

    // Update full download size
    HRESULT hr = GetElement(_T(ELEMENT_REMOTE_SELECT), &selectElem);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "GoToSelectFilePage: querying for local select element.");
    bool useLocalFile = m_useLocalFile;
    OnSelectedRemoteFileChanged(selectElem);
    m_useLocalFile = useLocalFile;

    if (m_remoteImages.GetSize() == 1 && !canUseLocal) {
        uprintf("No local images and one remote image, skipping selection screen");
        m_useLocalFile = false;
        if (forwards) {
            OnSelectFileNextClicked(NULL);
        } else {
            OnSelectFilePreviousClicked(NULL);
        }
    } else {
        ChangePage(_T(ELEMENT_FILE_PAGE));
    }

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
	} else if (id == _T(ELEMENT_USB_LEARN_MORE)) {
		msg_id = MSG_371;
    } else if (id == _T(ELEMENT_VERSION_LINK)) {
        url = RELEASE_VER_TAG_URL;
    } else {
        uprintf("Unknown link clicked %ls", id);
    }

    if (msg_id != 0) {
        url = lmprintf(msg_id);
    }

    if (url != NULL) {
        // Radu: do we care about the errors?
        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
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

	UpdateDualBootTexts();

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
	CString searchPath = GET_IMAGE_PATH(ALL_FILES);
    const CString liveFilePath = GET_IMAGE_PATH(EXFAT_ENDLESS_LIVE_FILE_NAME);
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
        uprintf("UpdateFileEntries: No files found in current directory [%ls]", CEndlessUsbToolApp::m_appDir);
        goto checkEntries;
    }

    do {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
        // The real filename on disk
        const CString currentFile = findFileData.cFileName;
        // If running from a live USB, the image file is named endless.img;
        // trueName is the filename it *should* have had (eos-...base.img, for example)
        CString trueName = currentFile;
        if ((
                currentFile == ENDLESS_IMG_FILE_NAME ||
                currentFile == ENDLESS_SQUASH_FILE_NAME
            ) && PathFileExists(liveFilePath)) {
			uprintf("Found %ls; checking %ls to find image name", currentFile, liveFilePath);
			bool foundTrueName = false;
			FILE *liveFile = NULL;
			char originalFileName[MAX_PATH];
			memset(originalFileName, 0, MAX_PATH);
			errno_t result = _wfopen_s(&liveFile, liveFilePath, L"r");
			if (result == 0) {
				fread(originalFileName, 1, MAX_PATH - 1, liveFile);
				if (feof(liveFile)) {
					trueName = UTF8ToCString(originalFileName);
					trueName.TrimRight();
					uprintf("image name is %ls", trueName);
					foundTrueName = true;
				}
				fclose(liveFile);
			}

			if (!foundTrueName) {
				uprintf("couldn't determine the unpacked image's true name");
			}
		}

        const CString currentFilePath =  GET_IMAGE_PATH(currentFile);
        const CString trueNamePath = GET_IMAGE_PATH(trueName);

        if (!PathFileExists(currentFilePath)) continue; // file is present

        const CString imgSigPath = trueNamePath + SIGNATURE_FILE_EXT;
        if (!PathFileExists(imgSigPath)) continue; // signature file is present

        try {
            CString displayName, personality, version, date;
            bool isInstallerImage = false;
            if (!ParseImgFileName(trueName, personality, version, date, isInstallerImage)) continue;
            CompressionType compressionType;
            ULONGLONG extractedSize = GetExtractedSize(currentFilePath, isInstallerImage, compressionType);
            if (0 == extractedSize) continue;
            CFile file(currentFilePath, CFile::modeRead | CFile::shareDenyNone);
            GetImgDisplayName(displayName, version, personality, file.GetLength());

            if (isInstallerImage) {
                // TODO: version comparison again!
                if (version > currentInstallerVersion) {
                    currentInstallerVersion = version;
                    m_localInstallerImage.stillPresent = TRUE;
                    m_localInstallerImage.filePath = file.GetFilePath();
                    m_localInstallerImage.fileSize = file.GetLength();
                    m_localInstallerImage.extractedSize = extractedSize;
                    m_localInstallerImage.imgSigPath = imgSigPath;
                }
            } else {
                if (IsDualBootOrCombinedUsb() && !HasImageBootSupport(version, date)) {
                    uprintf("Skipping '%ls' because it doesn't have image boot support.", file.GetFileName());
                    continue;
                }
                // add entry to list or update it
                pFileImageEntry_t currentEntry = NULL;
                if (!m_imageFiles.Lookup(file.GetFilePath(), currentEntry)) {
                    currentEntry = new FileImageEntry_t;
                    currentEntry->autoAdded = TRUE;
                    currentEntry->filePath = file.GetFilePath();
                    currentEntry->fileSize = file.GetLength();
                    currentEntry->imgSigPath = imgSigPath;
                    currentEntry->extractedSize = extractedSize;
                    currentEntry->personality = personality;
                    currentEntry->version = version;
                    currentEntry->date = date;

                    hr = AddEntryToSelect(selectElement, CComBSTR(currentEntry->filePath), CComBSTR(displayName), &currentEntry->htmlIndex, 0);
                    IFFALSE_RETURN(SUCCEEDED(hr), "Error adding item in image file list.");

                    m_imageFiles.SetAt(currentEntry->filePath, currentEntry);
                    m_imageIndexToPath.AddTail(currentEntry->filePath);
                } else {
                    UpdateSelectOptionText(selectElement, displayName, currentEntry->htmlIndex);
                }
                currentEntry->stillPresent = TRUE;
                CString basePath = (
                    compressionType == CompressionTypeNone ||
                    compressionType == CompressionTypeSquash)
                    ? CSTRING_GET_PATH(trueNamePath, '.')
                    : CSTRING_GET_PATH(CSTRING_GET_PATH(trueNamePath, '.'), '.');

                currentEntry->bootArchivePath = basePath + BOOT_ARCHIVE_SUFFIX;
                currentEntry->bootArchiveSigPath = basePath + BOOT_ARCHIVE_SUFFIX + SIGNATURE_FILE_EXT;
                try {
                    CFile bootArchive(currentEntry->bootArchivePath, CFile::modeRead | CFile::shareDenyNone);
                    currentEntry->bootArchiveSize = bootArchive.GetLength();
                } catch (CFileException *ex) {
                    currentEntry->bootArchiveSize = 0;
                    ex->Delete();
                }
                currentEntry->unpackedImgSigPath = basePath + IMAGE_FILE_EXT + SIGNATURE_FILE_EXT;
                currentEntry->hasBootArchive = PathFileExists(currentEntry->bootArchivePath);
                currentEntry->hasBootArchiveSig = PathFileExists(currentEntry->bootArchiveSigPath);
                currentEntry->hasUnpackedImgSig = PathFileExists(currentEntry->unpackedImgSigPath);
            }

			uprintf("Found local image '%ls'", file.GetFilePath());
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

    CComBSTR selectedValue;
    hr = selectElement->get_value(&selectedValue);
    IFFALSE_RETURN(SUCCEEDED(hr) && selectedValue != NULL, "Error getting selected file value");

    if (m_useLocalFile) {
        m_localFile = selectedValue;
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
    handlesToWaitFor[1] = FindFirstChangeNotification(CEndlessUsbToolApp::m_appDir, FALSE, changeNotifyFilter);
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

    if (m_remoteImages.GetCount() != 0) {
        uprintf("List of remote images already downloaded");
        return;
    }

    // Add both JSONs to download, maybe user goes back and switches between Try and Install
	CString liveJson(JsonLiveFile());
	CString installerJson(JsonInstallerFile());

    liveJson = CEndlessUsbToolApp::TempFilePath(liveJson);
    installerJson = CEndlessUsbToolApp::TempFilePath(installerJson);

    ListOfStrings urls = { JsonLiveFileURL(), JsonInstallerFileURL() };
    ListOfStrings files = { liveJson, installerJson };

    success = m_downloadManager.AddDownload(DownloadType_t::DownloadTypeReleseJson, urls, files, true);
    if (!success) {
        // If resuming a possibly-existing download failed, try harder to start a new one.
        // In theory this shouldn't be necessary but this is done for the image download case so who knows!
        success = m_downloadManager.AddDownload(DownloadType_t::DownloadTypeReleseJson, urls, files, false);
    }

    if (!success) {
        uprintf("AddDownload failed");
        SetJSONDownloadState(JSONDownloadState::Failed);
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
#define JSON_IMG_BOOT           "boot"

#define JSON_IMG_COMPRESSED_SIZE    "compressed_size"
#define JSON_IMG_EXTRACTED_SIZE     "extracted_size"
#define JSON_IMG_URL_FILE           "file"
#define JSON_IMG_URL_SIG            "signature"
#define JSON_IMG_UNPACKED_URL_SIG   "extracted_signature"

#define CHECK_ENTRY(parentValue, tag) \
do { \
    IFFALSE_CONTINUE(!parentValue[tag].isNull(), CString("\t\t Elem is NULL - ") + tag); \
    uprintf("\t\t %s=%s", tag, parentValue[tag].toStyledString().c_str()); \
} while(false);

bool CEndlessUsbToolDlg::UnpackFile(const CString &archive, const CString &destination, int compressionType, void* progress_function, unsigned long* cancel_request)
{
    FUNCTION_ENTER;

    int64_t result = -1;

    // RADU: provide a progress function and move this from UI thread
    // For initial release this is ok as the operation should be very fast for the JSON
    // Unpack the file
    result = bled_init(_uprintf, (progress_t)progress_function, cancel_request);
    result = bled_uncompress(ConvertUnicodeToUTF8(archive), ConvertUnicodeToUTF8(destination), compressionType);
    bled_exit();

    return result >= 0;
}

bool CEndlessUsbToolDlg::UnpackImage(const CString &image, const CString &destination)
{
    FUNCTION_ENTER_FMT("%ls -> %ls", image, destination);
    CompressionType type = GetCompressionType(image);

    switch (type) {
    case CompressionTypeNone:
    {
        BOOL result = CopyFileEx(image, destination, CEndlessUsbToolDlg::CopyProgressRoutine, this, NULL, 0);
        IFFALSE_RETURN_VALUE(result, "Copying uncompressed image failed/cancelled", false);
        return true;
    }
    case CompressionTypeSquash:
    {
        auto result = m_iso.UnpackSquashFS(image, destination,
            CEndlessUsbToolDlg::UpdateUnpackProgress, m_cancelOperationEvent);
        IFFALSE_RETURN_VALUE(result, "Unpacking SquashFS image failed/cancelled", false);
        return true;
    }
    case CompressionTypeUnknown:
        PRINT_ERROR_MSG("Unknown compression type");
        return false;
    default:
    {
        int bled_type = GetBledCompressionType(type);
        bool unpackResult = UnpackFile(image, destination, bled_type, ImageUnpackCallback, &this->m_cancelImageUnpack);
        IFFALSE_RETURN_VALUE(unpackResult, "Error unpacking image", false);
        return true;
    }
    }
}

static bool FindLatestVersion(const Json::Value &rootValue, Json::Value &latestEntry)
{
    FUNCTION_ENTER;

    Json::Value jsonElem, imagesElem, personalities, persImages;
    ImageVersion latestParsedVersion;

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

        const char *versionString = (*it)[JSON_IMG_VERSION].asCString();
        ImageVersion currentParsedVersion;
        if (!ImageVersion::Parse(versionString, currentParsedVersion)) {
            uprintf("Can't parse version '%s'", versionString);
            continue;
        }
        if (currentParsedVersion > latestParsedVersion) {
            latestParsedVersion = currentParsedVersion;
            latestEntry = *it;
        }
    }

    IFFALSE_GOTOERROR(!latestEntry.isNull(), "No images found in the JSON.");
    return true;

error:
    return false;
}

bool CEndlessUsbToolDlg::ParseJsonFile(LPCTSTR filename, bool isInstallerJson)
{
    FUNCTION_ENTER;

    Json::Reader reader;
    Json::Value rootValue, personalities, persImages, persImage, fullImage, latestEntry, bootImage;
    CString latestVersion("");

    std::ifstream jsonStream;

    jsonStream.open(filename);
    IFFALSE_GOTOERROR(!jsonStream.fail(), "Opening JSON file failed.");
    IFFALSE_GOTOERROR(reader.parse(jsonStream, rootValue, false), "Parsing of JSON failed.");

    IFFALSE_GOTOERROR(FindLatestVersion(rootValue, latestEntry), "No images found in the JSON.");

    if (!latestEntry.isNull()) {
        latestVersion = latestEntry[JSON_IMG_VERSION].asCString();
        uprintf("Selected version '%ls'", latestVersion);
        m_downloadManager.SetLatestEosVersion(latestVersion);
        personalities = latestEntry[JSON_IMG_PERSONALITIES];
        persImages = latestEntry[JSON_IMG_PERS_IMAGES];

        // Json::ValueIterator doesn't support random access so we can't std::sort personalties.
        // CList's POSITIONs aren't STL-style iterators so we can't std::sort m_remoteImages;
        // and it's a linked list so (again) random access is not cheap. It would be neater to
        // store images as a sorted map keyed by personalities, but for now let's just make a
        // new array and sort that.
        std::vector<std::string> ps;
        for (Json::ValueIterator persIt = personalities.begin(); persIt != personalities.end(); persIt++) {
            IFFALSE_CONTINUE(persIt->isString(), "Entry is not string, continuing");
            ps.push_back(persIt->asString());
        }
        std::sort(ps.begin(), ps.end(), [](const std::string &a, const std::string &b) {
            // Sort "base" before all other images, for the benefit of the "local images found" page
            // which lists all remote images together.
            if (a == "base") {
                return true;
            }
            if (b == "base") {
                return false;
            }
            return a < b;
        });

        for (auto persIt = ps.begin(); persIt != ps.end(); persIt++) {
            std::string &p = *persIt;
            CString pCStr = CString(p.c_str());
            IFFALSE_CONTINUE(!isInstallerJson || pCStr == PERSONALITY_BASE, "Installer JSON parsing: not base personality");

            persImage = persImages[p];
            IFFALSE_CONTINUE(!persImage.isNull(), CString("Personality image entry not found - ") + pCStr);

            fullImage = persImage[JSON_IMG_FULL];
            IFFALSE_CONTINUE(!fullImage.isNull(), CString("'full' entry not found for personality - ") + pCStr);

            CHECK_ENTRY(fullImage, JSON_IMG_COMPRESSED_SIZE);
            CHECK_ENTRY(fullImage, JSON_IMG_EXTRACTED_SIZE);
            CHECK_ENTRY(fullImage, JSON_IMG_URL_FILE);
            CHECK_ENTRY(fullImage, JSON_IMG_URL_SIG);
            CHECK_ENTRY(fullImage, JSON_IMG_UNPACKED_URL_SIG);

            RemoteImageEntry_t remoteImageEntry;
            RemoteImageEntry_t &remoteImage = isInstallerJson ? m_installerImage : remoteImageEntry;

            remoteImage.compressedSize = fullImage[JSON_IMG_COMPRESSED_SIZE].asUInt64();
            remoteImage.extractedSize = fullImage[JSON_IMG_EXTRACTED_SIZE].asUInt64();
            // before releases extractedSize is not populated; add the compressed size so we don't show 0
            if(remoteImage.extractedSize == 0) remoteImage.extractedSize = remoteImage.compressedSize;            
            remoteImage.urlFile = fullImage[JSON_IMG_URL_FILE].asCString();
            remoteImage.urlSignature = fullImage[JSON_IMG_URL_SIG].asCString();
            remoteImage.personality = pCStr;
            remoteImage.version = latestVersion;
            remoteImage.urlUnpackedSignature = fullImage[JSON_IMG_UNPACKED_URL_SIG].asCString();

            if(!isInstallerJson) {
                bootImage = persImage[JSON_IMG_BOOT];
                IFFALSE_CONTINUE(!bootImage.isNull(), CString("'boot' entry not found for personality - ") + pCStr);
                CHECK_ENTRY(bootImage, JSON_IMG_COMPRESSED_SIZE);
                CHECK_ENTRY(bootImage, JSON_IMG_URL_FILE);
                CHECK_ENTRY(bootImage, JSON_IMG_URL_SIG);

                remoteImage.urlBootArchive = bootImage[JSON_IMG_URL_FILE].asCString();
                remoteImage.urlBootArchiveSignature = bootImage[JSON_IMG_URL_SIG].asCString();
                remoteImage.bootArchiveSize = bootImage[JSON_IMG_COMPRESSED_SIZE].asUInt64();

                // Create dowloadJobName
                remoteImage.downloadJobName = latestVersion;
                remoteImage.downloadJobName += pCStr;

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
    filePath = CEndlessUsbToolApp::TempFilePath(CString(JsonLiveFile(false)));
#ifdef ENABLE_JSON_COMPRESSION
    filePathGz = CEndlessUsbToolApp::TempFilePath(CString(JsonLiveFile()));
    IFFALSE_GOTOERROR(UnpackFile(filePathGz, filePath, BLED_COMPRESSION_GZIP), "Error uncompressing eos JSON file.");
#endif // ENABLE_JSON_COMPRESSION
    IFFALSE_GOTOERROR(ParseJsonFile(filePath, false), "Error parsing eos JSON file.");

    // Parse JSON with installer images
    filePath = CEndlessUsbToolApp::TempFilePath(CString(JsonInstallerFile(false)));
#ifdef ENABLE_JSON_COMPRESSION
    filePathGz = CEndlessUsbToolApp::TempFilePath(CString(JsonInstallerFile()));
    IFFALSE_GOTOERROR(UnpackFile(filePathGz, filePath, BLED_COMPRESSION_GZIP), "Error uncompressing eosinstaller JSON file.");
#endif // ENABLE_JSON_COMPRESSION
    IFFALSE_GOTOERROR(ParseJsonFile(filePath, true), "Error parsing eosinstaller JSON file.");

    AddDownloadOptionsToUI();
    SetJSONDownloadState(JSONDownloadState::Succeeded);
    return;

error:
    // TODO: on error, retry download?
    SetJSONDownloadState(JSONDownloadState::Failed);
}

void CEndlessUsbToolDlg::GetPreferredPersonality(CString & personality)
{
    if (!m_localeToPersonality.Lookup(m_selectedLocale->txt[0], personality)) {
        uprintf("ERROR: Selected language personality not found. Defaulting to English");
        personality = PERSONALITY_ENGLISH;
    }
}

void CEndlessUsbToolDlg::AddDownloadOptionsToUI()
{
    CString languagePersonalty;
    GetPreferredPersonality(languagePersonalty);

    // remove all options from UI
    HRESULT hr = ClearSelectElement(_T(ELEMENT_REMOTE_SELECT));
    IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error clearing remote images select.");
    hr = ClearSelectElement(_T(ELEMENT_SELFILE_DOWN_LANG));
    IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error clearing remote images select.");

    // add options to UI
    long selectIndex = -1;
    for (POSITION pos = m_remoteImages.GetHeadPosition(); pos != NULL; ) {
        RemoteImageEntry_t imageEntry = m_remoteImages.GetNext(pos);
        bool matchesLanguage = languagePersonalty == imageEntry.personality;

        if (!matchesLanguage && imageEntry.personality == PERSONALITY_SOUTHEAST_ASIA) {
            /* Assume that if there's a sea image, there aren't language-specific images. */
            matchesLanguage = m_seaPersonalities.Find(languagePersonalty) != NULL;
        }

        // Create display name for advanced-mode dropdown list
        CString displayName;
        GetImgDisplayName(displayName, imageEntry.version, imageEntry.personality, imageEntry.compressedSize);

        hr = AddEntryToSelect(_T(ELEMENT_REMOTE_SELECT), CComBSTR(""), CComBSTR(displayName), &selectIndex, matchesLanguage);
        IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error adding remote image to list.");

        // option size
		CComBSTR sizeText = GetDownloadString(imageEntry);
        const wchar_t *htmlElemId = NULL;

        if (imageEntry.personality == PERSONALITY_BASE) {
            m_baseImageRemoteIndex = selectIndex;
            htmlElemId = _T(ELEMENT_DOWNLOAD_LIGHT_SIZE);
        }
        else {
            CString imageLanguage = LocalizePersonalityName(imageEntry.personality);
            CString indexStr;
            indexStr.Format(L"%d", selectIndex);
            hr = AddEntryToSelect(_T(ELEMENT_SELFILE_DOWN_LANG), CComBSTR(indexStr), CComBSTR(imageLanguage), NULL, matchesLanguage);
            IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error adding remote image to full images list.");
            if (matchesLanguage) {
                htmlElemId = _T(ELEMENT_DOWNLOAD_FULL_SIZE);
            }
        }
        if (htmlElemId != 0) {
            SetElementText(htmlElemId, sizeText);
        }
    }

    if (m_imageFiles.GetCount() == 0) {
        m_selectedRemoteIndex = m_baseImageRemoteIndex;
    }
}

void CEndlessUsbToolDlg::UpdateDownloadableState()
{
    FUNCTION_ENTER;

    HRESULT hr;

    bool foundRemoteImages = m_remoteImages.GetCount() != 0;
    // Show JSON download errors as connection errors. Maybe a proxy is mangling the JSON?
    bool connected = m_isConnected
        && m_jsonDownloadState != JSONDownloadState::Failed
        && m_jsonDownloadState != JSONDownloadState::Retrying;
    hr = CallJavascript(_T(JS_ENABLE_DOWNLOAD), CComVariant(foundRemoteImages), CComVariant(connected));
    IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error calling " JS_ENABLE_DOWNLOAD "()");

    bool foundBaseImage = m_baseImageRemoteIndex != -1;
    hr = CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_DOWNLOAD_LIGHT_BUTTON))), CComVariant(m_isConnected && foundBaseImage));
    IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error enabling 'Light' button");
}

HRESULT CEndlessUsbToolDlg::OnAdvancedPagePreviousClicked(IHTMLElement* pElement)
{
	ChangePage(_T(ELEMENT_DUALBOOT_PAGE));

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnCombinedUsbButtonClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    SetSelectedInstallMethod(InstallMethod_t::CombinedUsb);

    GoToSelectFilePage(true);

    return S_OK;
}

// Select File Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectFilePreviousClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    ChangePage(m_selectedInstallMethod == InstallMethod_t::InstallDualBoot ? _T(ELEMENT_DUALBOOT_PAGE) : _T(ELEMENT_ADVANCED_PAGE));
	SetSelectedInstallMethod(None);

	return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectFileNextClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    LPITEMIDLIST pidlDesktop = NULL;
    SHChangeNotifyEntry NotifyEntry;
    
    IFFALSE_RETURN_VALUE(pElement == NULL || !IsButtonDisabled(pElement), "OnSelectFileNextClicked: Button is disabled. ", S_OK);

    // Get display name with actual image size, not compressed
    CString personality, version;
    ULONGLONG size = 0;
    bool willUnpackImage = m_selectedInstallMethod == LiveUsb || m_selectedInstallMethod == CombinedUsb;

    if (m_useLocalFile) {
        pFileImageEntry_t localEntry = NULL;

		m_imageFiles.Lookup(m_localFile, localEntry);
		if (localEntry == NULL) {
			uprintf("ERROR: Selected local file not found.");
			ErrorOccured(ErrorCause_t::ErrorCauseGeneric);
			return S_OK;
		}

		personality = localEntry->personality;
		version = localEntry->version;
		size = localEntry->extractedSize;
		m_selectedFileSize = localEntry->fileSize;
    } else {
		POSITION p = m_remoteImages.FindIndex(m_selectedRemoteIndex);
		if (p == NULL) {
			uprintf("Remote index value not valid.");
			ErrorOccured(ErrorCause_t::ErrorCauseGeneric);
			return S_OK;
		}

		RemoteImageEntry_t remote = m_remoteImages.GetAt(p);

		personality = remote.personality;
		version = remote.version;

        CString selectedImage = CSTRING_GET_LAST(remote.urlFile, '/');
		CString selectedImagePath = GET_IMAGE_PATH(selectedImage);
        size = (willUnpackImage || RemoteMatchesUnpackedImg(selectedImagePath)) ? remote.extractedSize : remote.compressedSize;

		m_selectedFileSize = remote.compressedSize;
    }

    // add the installer size if this is not a live image
    if (m_selectedInstallMethod == InstallMethod_t::ReformatterUsb) {
        size += INSTALLER_DELTA_SIZE + m_installerImage.extractedSize;
    }

    m_finalSize = size;

	// update Thank You page fields with the selected image data
	uint32_t headlineMsg;
	switch (m_selectedInstallMethod) {
	case InstallDualBoot:
		headlineMsg = IsCoding() ? MSG_347 : MSG_320;
		break;
	case ReformatterUsb:
		headlineMsg = IsCoding() ? MSG_349 : MSG_344;
		break;
	case LiveUsb:
	case CombinedUsb:
		headlineMsg = IsCoding() ? MSG_348 : MSG_343;
		break;
	default:
		uprintf("Unexpected install method %ls", InstallMethodToStr(m_selectedInstallMethod));
		headlineMsg = IsCoding() ? MSG_348 : MSG_343;
	}

	CString finalMessageStr = UTF8ToCString(lmprintf(headlineMsg));
	CString imageLanguage = LocalizePersonalityName(personality);
	CStringA imageTypeA = lmprintf(personality == PERSONALITY_BASE ? MSG_400 : MSG_316); // Basic or Full
	CString imageType = UTF8ToCString(imageTypeA);

	SetElementText(_T(ELEMENT_THANKYOU_MESSAGE), CComBSTR(finalMessageStr));

	SetElementText(_T(ELEMENT_INSTALLER_VERSION), CComBSTR(version));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_INSTALLER_LANGUAGE_ROW), CComVariant(personality != PERSONALITY_BASE));
	SetElementText(_T(ELEMENT_INSTALLER_LANGUAGE), CComBSTR(imageLanguage));
	CString contentStr  = UTF8ToCString(lmprintf(MSG_319, imageTypeA, SizeToHumanReadable(size, FALSE, use_fake_units)));
	SetElementText(_T(ELEMENT_INSTALLER_CONTENT), CComBSTR(contentStr));

	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_DUALBOOT_REMINDER), CComVariant(m_selectedInstallMethod == InstallMethod_t::InstallDualBoot));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_LIVE_REMINDER), CComVariant(m_selectedInstallMethod == InstallMethod_t::LiveUsb || m_selectedInstallMethod == InstallMethod_t::CombinedUsb));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_REFLASHER_REMINDER), CComVariant(m_selectedInstallMethod == InstallMethod_t::ReformatterUsb));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_USBBOOT_HOWTO), CComVariant(m_selectedInstallMethod != InstallMethod_t::InstallDualBoot));

	if (m_selectedInstallMethod != InstallMethod_t::InstallDualBoot) {
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

	if (m_selectedInstallMethod != InstallMethod_t::InstallDualBoot) {
		CString displayName;
        GetImgDisplayName(displayName, version, personality, 0);
		SetElementText(_T(ELEMENT_SELUSB_NEW_DISK_NAME), CComBSTR(displayName));

        CString selectedSize = UTF8ToCString(SizeToHumanReadable(size, FALSE, use_fake_units));
		SetElementText(_T(ELEMENT_SELUSB_NEW_DISK_SIZE), CComBSTR(selectedSize));

		ChangePage(_T(ELEMENT_USB_PAGE));
	} else {
		GoToSelectStoragePage();
	}

	CString imageSource = _T("Remote");
	if (m_useLocalFile) imageSource = _T("Local");
	TrackEvent(_T("ImageSource"), imageSource);
	TrackEvent(_T("Version"), version);
	TrackEvent(_T("Personality"), personality);

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
    IFFALSE_RETURN_VALUE(SUCCEEDED(hr) && selectedValue != NULL, "Error getting selected file value", S_OK);

    m_localFile = selectedValue;
    uprintf("OnSelectedImageFileChanged to LOCAL [%ls]", m_localFile);

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
    uprintf("OnSelectedRemoteFileChanged to REMOTE [%ls]", r.urlFile);

    if (r.personality != PERSONALITY_BASE) {
        SetElementText(_T(ELEMENT_DOWNLOAD_FULL_SIZE), GetDownloadString(r));
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
	m_useLocalFile = false;
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
 
	m_useLocalFile = false;
    OnSelectFileNextClicked(pElement);

    return S_OK;
}

// Select USB Page Handlers
HRESULT CEndlessUsbToolDlg::OnSelectUSBPreviousClicked(IHTMLElement* pElement)
{
    FUNCTION_ENTER;

    LeavingDevicesPage();
    GoToSelectFilePage(false);

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectUSBNextClicked(IHTMLElement* pElement)
{
    IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnSelectUSBNextClicked: Button is disabled. ", S_OK);

    FUNCTION_ENTER;

	TrackEvent(_T("USBSizeMB"), SelectedDrive.DiskSize / BYTES_IN_MEGABYTE);

	LeavingDevicesPage();
	StartInstallationProcess();

	return S_OK;
}

void CEndlessUsbToolDlg::StartInstallationProcess()
{
	SetElementText(_T(ELEMENT_INSTALL_STATUS), CComBSTR(""));

	TrackEvent(_T("Started"));

	EnableHibernate(false);

	if(m_selectedInstallMethod != InstallMethod_t::InstallDualBoot) ChangeDriveAutoRunAndMount(true);

	if (m_useLocalFile) {
		pFileImageEntry_t localEntry = NULL;
		if (!m_imageFiles.Lookup(m_localFile, localEntry)) {
			ChangePage(_T(ELEMENT_INSTALL_PAGE));
			uprintf("Error: couldn't find local entry for selected local file '%ls'", m_localFile);
			FormatStatus = FORMAT_STATUS_CANCEL;
			m_lastErrorCause = ErrorCause_t::ErrorCauseGeneric;
			PostMessage(WM_FINISHED_ALL_OPERATIONS, 0, 0);
			return;
		}

		m_localFileSig = localEntry->imgSigPath;
		if (IsDualBootOrCombinedUsb()) {
			if (localEntry->hasBootArchive && localEntry->hasBootArchiveSig && localEntry->hasUnpackedImgSig) {
				m_bootArchive = localEntry->bootArchivePath;
				m_bootArchiveSig = localEntry->bootArchiveSigPath;
				m_bootArchiveSize = localEntry->bootArchiveSize;
				m_unpackedImageSig = localEntry->unpackedImgSigPath;
			} else {
				m_useLocalFile = false;
			}
		}
	}

	// Radu: we need to download an installer if only a live image is found and full install was selected
	if (m_useLocalFile) {
		StartFileVerificationThread();
	} else {
		UpdateCurrentStep(OP_DOWNLOADING_FILES);

		RemoteImageEntry_t remote = m_remoteImages.GetAt(m_remoteImages.FindIndex(m_selectedRemoteIndex));
		DownloadType_t downloadType = GetSelectedDownloadType();

		// live image file
		CString url = CString(RELEASE_JSON_URLPATH) + remote.urlFile;
		m_localFile = GET_IMAGE_PATH(CSTRING_GET_LAST(remote.urlFile, '/'));
		bool localFileExists = PackedImageAlreadyExists(m_localFile, remote.compressedSize, remote.extractedSize, false);

		// live image signature file
		CString urlAsc = CString(RELEASE_JSON_URLPATH) + remote.urlSignature;
		if(RemoteMatchesUnpackedImg(m_localFile, &m_localFileSig)) {
			m_localFile = GET_IMAGE_PATH(ENDLESS_IMG_FILE_NAME);
			urlAsc = CString(RELEASE_JSON_URLPATH) + remote.urlUnpackedSignature;
		} else {
			m_localFileSig = GET_IMAGE_PATH(CSTRING_GET_LAST(remote.urlSignature, '/'));
		}

		// List of files to download
		ListOfStrings urls, files;
		CString urlInstaller, urlInstallerAsc, installerFile, installerAscFile;
		CString urlBootFiles, urlBootFilesAsc, urlImageSig;
		if (IsDualBootOrCombinedUsb()) {
			urlBootFiles = CString(RELEASE_JSON_URLPATH) +  remote.urlBootArchive;
			m_bootArchive = GET_IMAGE_PATH(CSTRING_GET_LAST(urlBootFiles, '/'));

			urlBootFilesAsc = CString(RELEASE_JSON_URLPATH) + remote.urlBootArchiveSignature;
			m_bootArchiveSig = GET_IMAGE_PATH(CSTRING_GET_LAST(urlBootFilesAsc, '/'));

			m_bootArchiveSize = remote.bootArchiveSize;

			urlImageSig = CString(RELEASE_JSON_URLPATH) + remote.urlUnpackedSignature;
			m_unpackedImageSig = GET_IMAGE_PATH(CSTRING_GET_LAST(urlImageSig, '/'));

			if (localFileExists) {
				urls = { urlAsc, urlBootFiles, urlBootFilesAsc, urlImageSig };
				files = { m_localFileSig, m_bootArchive, m_bootArchiveSig, m_unpackedImageSig };
			} else {
				urls = { url, urlAsc, urlBootFiles, urlBootFilesAsc, urlImageSig };
				files = { m_localFile, m_localFileSig, m_bootArchive, m_bootArchiveSig, m_unpackedImageSig };
			}
		} else if (m_selectedInstallMethod == InstallMethod_t::LiveUsb) {
			if (localFileExists) {
				urls = { urlAsc };
				files = { m_localFileSig };
			} else {
				urls = { url, urlAsc };
				files = { m_localFile, m_localFileSig };
			}
		} else {
			// installer image file + signature
			urlInstaller = CString(RELEASE_JSON_URLPATH) + m_installerImage.urlFile;
			installerFile = GET_IMAGE_PATH(CSTRING_GET_LAST(m_installerImage.urlFile, '/'));
			bool installerFileExists = PackedImageAlreadyExists(installerFile, m_installerImage.compressedSize, m_installerImage.extractedSize, true);

			urlInstallerAsc = CString(RELEASE_JSON_URLPATH) + m_installerImage.urlSignature;
			installerAscFile = GET_IMAGE_PATH(CSTRING_GET_LAST(m_installerImage.urlSignature, '/'));

			if (installerFileExists) {
				if (localFileExists) {
					urls = { urlAsc, urlInstallerAsc };
					files = { m_localFileSig, installerAscFile };
				} else {
					urls = { url, urlAsc, urlInstallerAsc };
					files = { m_localFile, m_localFileSig, installerAscFile };
				}
			} else if(localFileExists) {
				urls = { urlAsc, urlInstaller, urlInstallerAsc };
				files = { m_localFileSig, installerFile, installerAscFile };
			} else {
				urls = { url, urlAsc, urlInstaller, urlInstallerAsc };
				files = { m_localFile, m_localFileSig, installerFile, installerAscFile };
			}
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
		m_localInstallerImage.filePath = GET_IMAGE_PATH(CSTRING_GET_LAST(m_installerImage.urlFile, '/'));
		m_localInstallerImage.imgSigPath = GET_IMAGE_PATH(CSTRING_GET_LAST(m_installerImage.urlSignature, '/'));
		m_localInstallerImage.fileSize = m_installerImage.compressedSize;
		m_localInstallerImage.extractedSize = m_installerImage.extractedSize;
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

	UpdateUSBSpeedMessage(deviceIndex);

    IFFALSE_GOTOERROR(deviceIndex >= 0, "Selected drive index is invalid.");

    memset(&SelectedDrive, 0, sizeof(SelectedDrive));
    SelectedDrive.DeviceNumber = (DWORD)ComboBox_GetItemData(hDeviceList, deviceIndex);
    GetDrivePartitionData(SelectedDrive.DeviceNumber, fs_type, sizeof(fs_type), FALSE);
    
    if (pElement != NULL) {
        CallJavascript(_T(JS_RESET_CHECK), CComVariant(_T(ELEMENT_SELUSB_AGREEMENT)));
        m_usbDeleteAgreement = false;
    }

    // RADU: should it be >= or should we take some more stuff into account like the partition size 
    BOOL isDiskBigEnough = (ULONGLONG)SelectedDrive.DiskSize > m_finalSize;

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

    GoToSelectFilePage(false);

    return S_OK;
}

HRESULT CEndlessUsbToolDlg::OnSelectStorageNextClicked(IHTMLElement *pElement)
{
	IFFALSE_RETURN_VALUE(!IsButtonDisabled(pElement), "OnSelectStorageNextClicked: Button is disabled. ", S_OK);

	FUNCTION_ENTER;

	TrackEvent(_T("StorageSizeGB"), (LONGLONG) m_nrGigsSelected);

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

ULONGLONG CEndlessUsbToolDlg::GetNeededSpaceForDualBoot(int &neededGigs, bool *isBaseImage)
{
	if(isBaseImage != NULL)  *isBaseImage = true;
	// figure out how much space we need
	ULONGLONG neededSize = 0;
	ULONGLONG bytesInGig = BYTES_IN_GIGABYTE;
	if (m_useLocalFile) {
		pFileImageEntry_t localEntry = NULL;

		if (!m_imageFiles.Lookup(m_localFile, localEntry)) {
			uprintf("ERROR: Selected local file '%ls' not found.", m_localFile);
		} else {
			if (isBaseImage != NULL)  *isBaseImage = (localEntry->personality == PERSONALITY_BASE);
			neededSize = localEntry->extractedSize;
		}
	} else {
		POSITION p = m_remoteImages.FindIndex(m_selectedRemoteIndex);
		if (p != NULL) {
			RemoteImageEntry_t remote = m_remoteImages.GetAt(p);
			neededSize = remote.extractedSize;
			if (GetExePath().Left(3) == GetSystemDrive()) {
				neededSize += remote.compressedSize + remote.bootArchiveSize;
			}
			if (isBaseImage != NULL)  *isBaseImage = (remote.personality == PERSONALITY_BASE);
		}
	}
	neededSize += bytesInGig;
	neededGigs = (int)(neededSize / bytesInGig);
	return neededSize;
}

void CEndlessUsbToolDlg::GoToSelectStoragePage()
{
	ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	CComPtr<IHTMLSelectElement> selectElement;
	HRESULT hr;
	int maxAvailableGigs = 0;
	CStringA freeSize, totalSize;
	bool isBaseImage = true;

	ULONGLONG bytesInGig = BYTES_IN_GIGABYTE;
	int neededGigs = 0;
	ULONGLONG neededSize = GetNeededSpaceForDualBoot(neededGigs, &isBaseImage);

	CStringW systemDrive = GetSystemDrive();
	CStringA systemDriveA = ConvertUnicodeToUTF8(systemDrive);

	// get available space on system drive
	IFFALSE_RETURN(GetDiskFreeSpaceEx(systemDrive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes) != 0, "Error on GetDiskFreeSpace");
	totalSize = SizeToHumanReadable(totalNumberOfBytes.QuadPart, FALSE, use_fake_units);
	freeSize = SizeToHumanReadable(freeBytesAvailable.QuadPart, FALSE, use_fake_units);
	uprintf("Available space on drive %s is %s out of %s; we need %s", systemDrive, freeSize, totalSize, SizeToHumanReadable(neededSize, FALSE, use_fake_units));
	maxAvailableGigs = (int) ((freeBytesAvailable.QuadPart - bytesInGig) / bytesInGig);

	// update messages with needed space based on selected version
	CStringA osVersion = lmprintf(isBaseImage ? MSG_400 : MSG_316);
	CStringA osSizeText = SizeToHumanReadable((isBaseImage ? RECOMMENDED_GIGS_BASE : RECOMMENDED_GIGS_FULL) * bytesInGig, FALSE, use_fake_units);

	CString message = UTF8ToCString(lmprintf(IsCoding() ? MSG_346 : MSG_337, osVersion, osSizeText));
	SetElementText(_T(ELEMENT_STORAGE_DESCRIPTION), CComBSTR(message));

	message = UTF8ToCString(lmprintf(MSG_341, freeSize, totalSize, systemDriveA));
	SetElementText(_T(ELEMENT_STORAGE_AVAILABLE), CComBSTR(message));

	message = UTF8ToCString(lmprintf(IsCoding() ? MSG_345 : MSG_342, systemDriveA));
	SetElementText(_T(ELEMENT_STORAGE_MESSAGE), CComBSTR(message));

	// Clear existing elements from the drop down
	hr = GetSelectElement(_T(ELEMENT_STORAGE_SELECT), selectElement);
	IFFALSE_RETURN(SUCCEEDED(hr) && selectElement != NULL, "Error returned from GetSelectElement.");
	hr = selectElement->put_length(0);

	bool enoughBytesAvailable = (freeBytesAvailable.QuadPart - bytesInGig) > neededSize;

	// Enable/disable ui elements based on space availability
	CallJavascript(_T(JS_ENABLE_ELEMENT), CComVariant(_T(ELEMENT_STORAGE_SELECT)), CComVariant(enoughBytesAvailable));
    CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_SELSTORAGE_NEXT_BUTTON))), CComVariant(enoughBytesAvailable));

	// Show/hide space warning vs "back up your files!" notice
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(_T(ELEMENT_STORAGE_AGREEMENT_TEXT)), CComVariant(enoughBytesAvailable));
	CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(_T(ELEMENT_STORAGE_SPACE_WARNING)), CComVariant(!enoughBytesAvailable));

	if (!enoughBytesAvailable) {
		uprintf("Not enough bytes available.");

		message = UTF8ToCString(lmprintf(IsCoding() ? MSG_374 : MSG_335, SizeToHumanReadable(neededSize, FALSE, use_fake_units), freeSize, systemDriveA));
		SetElementText(_T(ELEMENT_STORAGE_SPACE_WARNING), CComBSTR(message));

		ChangePage(_T(ELEMENT_STORAGE_PAGE));

		TrackEvent(_T("StorageInsufficient"));

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

    m_lastErrorCause = ErrorCause_t::ErrorCauseCancelled;
    CancelRunningOperation(true);

    return S_OK;
}

// Error/Thank You Page Handlers
HRESULT CEndlessUsbToolDlg::OnCloseAppClicked(IHTMLElement* pElement)
{
	EndDialog(IDCLOSE);
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
    SetJSONDownloadState(JSONDownloadState::Pending);
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
        OnSelectFileNextClicked(NULL);
        break;
    case ErrorCause_t::ErrorCauseVerificationFailed:
    {
        CSingleLock lock(&m_verifyFilesMutex);
        lock.Lock();

        if (m_verifyFiles.empty()) {
            uprintf("Verification failed, but no file is invalid(?)");
        } else {
            auto &p = m_verifyFiles.front();
            BOOL result = DeleteFile(p.filePath);
            uprintf("%s on deleting file '%ls' - %s", result ? "Success" : "Error", p.filePath, WindowsErrorString());
            SetLastError(ERROR_SUCCESS);
            result = DeleteFile(p.sigPath);
            uprintf("%s on deleting sig '%ls' - %s", result ? "Success" : "Error", p.sigPath, WindowsErrorString());
        }
        m_verifyFiles.clear();
        ChangePage(_T(ELEMENT_DUALBOOT_PAGE));
        break;
    }
    case ErrorCause_t::ErrorCauseCancelled:
    default:
        ChangePage(_T(ELEMENT_DUALBOOT_PAGE));
        break;
    }

	if (m_taskbarProgress != NULL) {
		m_taskbarProgress->SetProgressState(m_hWnd, TBPF_NORMAL);
		m_taskbarProgress->SetProgressValue(m_hWnd, 0, 100);
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
                m_lastErrorCause = ErrorCause_t::ErrorCauseCancelled;
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
	if (IsDualBootOrCombinedUsb()) {
		return DownloadType_t::DownloadTypeDualBootFiles;
	} else {
		return m_selectedInstallMethod == InstallMethod_t::LiveUsb ? DownloadType_t::DownloadTypeLiveImage : DownloadType_t::DownloadTypeInstallerImage;
	}
}

void CEndlessUsbToolDlg::OnClose()
{
    FUNCTION_ENTER;

    if (!CancelInstall()) {
        m_closeRequested = true;
        return;
    }

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
    CString action;
    int nrCurrentStep;
    int locMsgIdTitle;
    m_currentStep = currentStep;
    
    switch (m_currentStep)
    {
    case OP_DOWNLOADING_FILES:
        action = _T("DownloadStarted");
        locMsgIdTitle = MSG_304;
        nrCurrentStep = 1;
        break;
    case OP_VERIFYING_SIGNATURE:
        action = _T("VerifyStarted");
        locMsgIdTitle = MSG_310;
        nrCurrentStep = m_useLocalFile ? 1 : 2;
        break;
    case OP_FLASHING_DEVICE:
    case OP_FILE_COPY:
    case OP_NEW_LIVE_CREATION:
        action = _T("WritingStarted");
        locMsgIdTitle = MSG_311;
        nrCurrentStep = m_useLocalFile ? 2 : 3;
        break;
    case OP_SETUP_DUALBOOT:
        action = _T("InstallStarted");
        locMsgIdTitle = MSG_309;
        nrCurrentStep = m_useLocalFile ? 2 : 3;
        break;
    default:
        action.Format(_T("%ls(%d)"), OperationToStr(currentStep), currentStep);
        uprintf("Unknown operation %ls", action);
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

    TrackEvent(action);
}

void CEndlessUsbToolDlg::StartFileVerificationThread()
{
    FUNCTION_ENTER;

    {
        CSingleLock lock(&m_verifyFilesMutex);
        lock.Lock();

        m_verifyFiles.clear();
        if (IsDualBootOrCombinedUsb()) {
            m_verifyFiles.push_back(SignedFile_t(m_bootArchive, m_bootArchiveSize, m_bootArchiveSig));
        }
        else if (m_selectedInstallMethod == ReformatterUsb) {
            m_verifyFiles.push_back(SignedFile_t(
                m_localInstallerImage.filePath, m_localInstallerImage.fileSize, m_localInstallerImage.imgSigPath));
        }

        m_verifyFiles.push_back(SignedFile_t(m_localFile, m_selectedFileSize, m_localFileSig));
    }

    StartOperationThread(OP_VERIFYING_SIGNATURE, CEndlessUsbToolDlg::FileVerificationThread, this);
}

struct FileHashingContext {
    HANDLE cancelOperationEvent;

    // The sum of sizes of files that have already been verified, divided by
    // the total size of all files to be verified.
    float previousFiles;

    // The size of the file being verified, divided by the total size of all
    // files being verified.
    float currentScale;
};

bool CEndlessUsbToolDlg::FileHashingCallback(__int64 currentSize, __int64 totalSize, LPVOID context)
{
    FileHashingContext *ctx = (FileHashingContext *) context;
    DWORD dwWaitStatus = WaitForSingleObject(ctx->cancelOperationEvent, 0);
    if(dwWaitStatus == WAIT_OBJECT_0) {
        uprintf("FileHashingCallback: Cancel requested.");
        return false;
    }
    
    // When verifying a SquashFS image, we actually decompress on the fly and
    // verify the uncompressed image within. In this case, currentSize and
    // totalSize are measured in *uncompressed* bytes. We allocate proportions
    // of the progress bar based on the compressed size of the files, and
    // assume that the proportion of decompressed bytes processed increases at
    // approximately the same rate as the proportion of compressed bytes.
    // (In the common case, the two files to be verified are a tiny boot.zip
    // bundle and an enormous OS image, so the files will get none and all of
    // the progress bar, respectively!)
    float current = ctx->currentScale * currentSize / totalSize;
    UpdateProgress(OP_VERIFYING_SIGNATURE, 100 * (ctx->previousFiles + current));

    return true;
}

DWORD WINAPI CEndlessUsbToolDlg::FileVerificationThread(void* param)
{
    FUNCTION_ENTER;

    CEndlessUsbToolDlg *dlg = (CEndlessUsbToolDlg *)param;
    FileHashingContext ctx = { dlg->m_cancelOperationEvent, 0, 0 };
    ULONGLONG totalSize = 0;
    CSingleLock lock(&dlg->m_verifyFilesMutex);
    lock.Lock();

    auto &verifyFiles = dlg->m_verifyFiles;
    bool verificationResult = false;
    IFFALSE_GOTOERROR(!verifyFiles.empty(), "nothing to verify");

    for (auto &p : verifyFiles) {
        totalSize += p.fileSize;
    }

    verificationResult = true;
    while (!verifyFiles.empty()) {
        auto &p = verifyFiles.front();
        const CString &filePath = p.filePath;
        const CString &sigPath = p.sigPath;
        uprintf("Verifying %ls against %ls", filePath, sigPath);
        bool v;

        ctx.currentScale = (float) p.fileSize / totalSize;

        switch (GetCompressionType(filePath)) {
        case CompressionTypeSquash:
            v = dlg->m_iso.VerifySquashFS(filePath, sigPath, FileHashingCallback, &ctx);
            break;
        default:
            v = VerifyFile(filePath, sigPath, FileHashingCallback, &ctx);
        }

        if (!v) {
            uprintf("Verification failed for %ls against %ls", filePath, sigPath);
            verificationResult = false;
            break;
        }

        ctx.previousFiles += ctx.currentScale;
        verifyFiles.pop_front();
    }

error:
    ::PostMessage(hMainDialog, WM_FINISHED_FILE_VERIFICATION, (WPARAM)verificationResult, 0);

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

// Used in ReformatterUsb mode, after the eosinstaller image has been written
// to the target device. Appends an exFAT partition (renumbered to be logically
// first) and copies the OS image, signature, and language preset across.
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
    CString driveDestination, fileDestination, liveFileName;
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

    uprintf("Partition type: GPT, NB Partitions: %d\n", DriveLayout->PartitionCount);
    uprintf("Disk GUID: %s\n", GuidToString(&DriveLayout->Gpt.DiskId));
    uprintf("Max parts: %d, Start Offset: %I64i, Usable = %I64i bytes\n",
        DriveLayout->Gpt.MaxPartitionCount, DriveLayout->Gpt.StartingUsableOffset.QuadPart, DriveLayout->Gpt.UsableLength.QuadPart);

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
    newPartition->PartitionLength.QuadPart = DriveLayout->Gpt.UsableLength.QuadPart - newPartition->StartingOffset.QuadPart;

    newPartition->Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;
    IGNORE_RETVAL(CoCreateGuid(&newPartition->Gpt.PartitionId));
    wcscpy(newPartition->Gpt.Name, EXFAT_PARTITION_NAME_IMAGES);

    DriveLayout->PartitionCount += 1;

    size = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + DriveLayout->PartitionCount * sizeof(PARTITION_INFORMATION_EX);
    IFFALSE_GOTOERROR(DeviceIoControl(hPhysical, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, layout, size, NULL, 0, &size, NULL), "Could not set drive layout.");
    result = RefreshDriveLayout(hPhysical);
    safe_closehandle(hPhysical);

    // Check if user cancelled
    IFFALSE_GOTOERROR(WaitForSingleObject(dlg->m_cancelOperationEvent, 0) == WAIT_TIMEOUT, "User cancel.");
	// Format the partition
	IFFALSE_GOTOERROR(FormatFirstPartitionOnDrive(DriveIndex, FS_EXFAT, dlg->m_cancelOperationEvent, EXFAT_PARTITION_NAME_IMAGES), "Error on FormatFirstPartitionOnDrive");
    // Mount it
	IFFALSE_GOTOERROR(MountFirstPartitionOnDrive(DriveIndex, driveDestination), "Error on MountFirstPartitionOnDrive");

    // Copy Files
	liveFileName = CSTRING_GET_LAST(dlg->m_localFile, L'\\');
	if (liveFileName == ENDLESS_IMG_FILE_NAME) {
		fileDestination = driveDestination + CSTRING_GET_PATH(CSTRING_GET_LAST(dlg->m_localFileSig, L'\\'), L'.');
	} else {
		fileDestination = driveDestination + liveFileName;
	}
    result = CopyFileEx(dlg->m_localFile, fileDestination, CEndlessUsbToolDlg::CopyProgressRoutine, dlg, NULL, 0);
    IFFALSE_GOTOERROR(result, "Copying live image failed/cancelled.");

    fileDestination = driveDestination + CSTRING_GET_LAST(dlg->m_localFileSig, L'\\');
    result = CopyFileEx(dlg->m_localFileSig, fileDestination, NULL, NULL, NULL, 0);
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

	if (dlg->m_currentStep == OP_SETUP_DUALBOOT || dlg->m_currentStep ==OP_NEW_LIVE_CREATION) {
		CEndlessUsbToolDlg::ImageUnpackCallback(TotalBytesTransferred.QuadPart);
	} else {
		float current = (float)(TotalBytesTransferred.QuadPart * 100 / TotalFileSize.QuadPart);
		UpdateProgress(OP_FILE_COPY, current);
	}

    return PROGRESS_CONTINUE;
}

const CString CEndlessUsbToolDlg::LocalizePersonalityName(const CString &personality)
{
    uint32_t msgId;

    if (personality == PERSONALITY_SOUTHEAST_ASIA) {
        CString localePersonality;
        CString seaName;
        POSITION pos;

        GetPreferredPersonality(localePersonality);

        pos = m_seaPersonalities.GetHeadPosition();
        while (pos) {
            auto p = m_seaPersonalities.GetNext(pos);
            if (m_personalityToLocaleMsg.Lookup(p, msgId)) {
                auto m = UTF8ToCString(lmprintf(msgId));

                // If running in a sea language, label the personality as
                // that language alone.
                if (p == localePersonality) {
                    return m;
                }

                if (!seaName.IsEmpty()) {
                    seaName += L", ";
                }

                seaName += m;
            }
        }

        if (!seaName.IsEmpty()) {
            return seaName;
        }
    }

    if (m_personalityToLocaleMsg.Lookup(personality, msgId)) {
        return UTF8ToCString(lmprintf(msgId));
    }

    uprintf("unknown personality ID '%ls'", personality);
    Analytics::instance()->eventTracking(L"FIXME", L"UnknownPersonality", personality);
    return personality;
}

void CEndlessUsbToolDlg::GetImgDisplayName(CString &displayName, const CString &version, const CString &personality, ULONGLONG size)
{
    FUNCTION_ENTER;

    ULONGLONG actualsize = m_selectedInstallMethod == InstallMethod_t::ReformatterUsb ? (size + m_installerImage.compressedSize) : size;
    // Create display name
    displayName = _T(ENDLESS_OS);
    displayName += " ";
    displayName += version;
    displayName += " ";
    bool isBase = personality == PERSONALITY_BASE;
    displayName += UTF8ToCString(lmprintf(isBase ? MSG_400 : MSG_316));
    if (personality != PERSONALITY_BASE) {
        displayName += " - ";
        displayName += LocalizePersonalityName(personality);
    }
    if (size != 0) {
        displayName += " - ";
        displayName += SizeToHumanReadable(actualsize, FALSE, use_fake_units);
    }
}

CompressionType CEndlessUsbToolDlg::GetCompressionType(const CString& filename)
{
    FUNCTION_ENTER_FMT("%ls", filename);

    CString ext = CSTRING_GET_LAST(filename, '.').MakeLower();
    if (ext == "gz")  return CompressionTypeGz;
    if (ext == "xz")  return CompressionTypeXz;
    if (ext == "img") return CompressionTypeNone;
    if (ext == "squash") return CompressionTypeSquash;

    uprintf("%ls has unknown compression type %ls", filename, ext);
    return CompressionTypeUnknown;
}

int CEndlessUsbToolDlg::GetBledCompressionType(const CompressionType type)
{
    switch (type) {
    case CompressionTypeGz:
        return BLED_COMPRESSION_GZIP;
    case CompressionTypeXz:
        return BLED_COMPRESSION_XZ;
    case CompressionTypeNone:
        return BLED_COMPRESSION_NONE;
    case CompressionTypeUnknown:
    default:
        return BLED_COMPRESSION_MAX;
    }
}

ULONGLONG CEndlessUsbToolDlg::GetExtractedSize(const CString& filename, BOOL isInstallerImage, CompressionType &compressionType)
{
    FUNCTION_ENTER;

    compressionType = GetCompressionType(filename);
    switch (compressionType) {
    case CompressionTypeUnknown:
        return 0;
    case CompressionTypeSquash:
        return m_iso.GetExtractedSize(filename, isInstallerImage);
    default:
        int bled_type = GetBledCompressionType(compressionType);
        IFFALSE_RETURN_VALUE(bled_type >= BLED_COMPRESSION_NONE && bled_type < BLED_COMPRESSION_MAX, "unknown compression", 0);

        CStringA asciiFileName = ConvertUnicodeToUTF8(filename);
        return get_eos_archive_disk_image_size(asciiFileName, bled_type, isInstallerImage);
    }
}

void CEndlessUsbToolDlg::GetIEVersion()
{
    FUNCTION_ENTER;

    CRegKey registryKey;
    LSTATUS result;
    wchar_t versionValue[256];
    ULONG size = _countof(versionValue);

    result = registryKey.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Internet Explorer", KEY_QUERY_VALUE);
    IFFALSE_RETURN(result == ERROR_SUCCESS, "Error opening IE registry key.");

    // Quoth <https://support.microsoft.com/en-us/kb/969393>:
    // "The version string value for Internet Explorer 10 is 9.10.9200.16384,
    // and the svcVersion string value is 10.0.9200.16384."
    result = registryKey.QueryStringValue(L"svcVersion", versionValue, &size);
    if (result != ERROR_SUCCESS) {
        size = _countof(versionValue);
        result = registryKey.QueryStringValue(L"Version", versionValue, &size);
    }
    IFFALSE_RETURN(result == ERROR_SUCCESS, "Error Querying for version value.");
    uprintf("Internet Explorer %ls", versionValue);

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
                DownloadStatus_t downloadStatus;
                bool result = dlg->m_downloadManager.GetDownloadProgress(currentJob, downloadStatus, jobName);
                if (!result || downloadStatus.done || downloadStatus.error) {
                    uprintf("CEndlessUsbToolDlg::UpdateDownloadProgressThread - Exiting");
                    goto done;
                }
                ::PostMessage(dlg->m_hWnd, WM_FILE_DOWNLOAD_STATUS, (WPARAM) new DownloadStatus_t(downloadStatus), 0);
                break;
            }
        }
    }

done:
    dlg->m_downloadUpdateThread = INVALID_HANDLE_VALUE;
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

                break;
            }
        }

        if (firstTime || result != connected) {
            uprintf("InternetGetConnectedState changed: %s (flags %d)", result ? "online" : "offline", flags);
            firstTime = FALSE;
            connected = result;
            ::PostMessage(dlg->m_hWnd, WM_INTERNET_CONNECTION_STATE, (WPARAM)connected, 0);
        }
    }

done:
    dlg->m_checkConnectionThread = INVALID_HANDLE_VALUE;
    return 0;
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

void CEndlessUsbToolDlg::CancelRunningOperation(bool userCancel)
{
    FUNCTION_ENTER;

    SetEvent(m_cancelOperationEvent);
	m_cancelImageUnpack = 1;

    FormatStatus = FORMAT_STATUS_CANCEL;
    if (m_currentStep != OP_FLASHING_DEVICE) {
        m_downloadManager.ClearExtraDownloadJobs(userCancel);
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
    bool hasLocalInstaller = m_selectedInstallMethod == InstallMethod_t::LiveUsb || (m_selectedInstallMethod == InstallMethod_t::ReformatterUsb && m_localInstallerImage.stillPresent == TRUE);
    bool hasLocalImages = (m_imageFiles.GetCount() != 0);

	// If we have a local entry with all needed files
	bool hasFilesForDualBoot = false;
	if (IsDualBootOrCombinedUsb() && hasLocalImages) {
		pFileImageEntry_t currentEntry = NULL;
		CString path;
		for (POSITION position = m_imageFiles.GetStartPosition(); position != NULL; ) {
			m_imageFiles.GetNextAssoc(position, path, currentEntry);
			bool imageBootSupport = HasImageBootSupport(currentEntry->version, currentEntry->date);

			if (imageBootSupport && currentEntry->hasBootArchive && currentEntry->hasBootArchiveSig &&currentEntry->hasUnpackedImgSig) {
				hasFilesForDualBoot = true;
				break;
			}
		}
	}

    return !m_localFilesScanned || (hasLocalInstaller && hasLocalImages) || hasFilesForDualBoot;
}

// Returns true if we might, in principle, be able to use a remote file.
bool CEndlessUsbToolDlg::CanUseRemoteFile()
{
    return m_jsonDownloadState != JSONDownloadState::Failed || m_remoteImages.GetCount() != 0;
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

void CEndlessUsbToolDlg::UpdateUSBSpeedMessage(int deviceIndex)
{
	if (deviceIndex < 0) {
		SetElementText(_T(ELEMENT_SELUSB_SPEEDWARNING), UTF8ToBSTR(lmprintf(MSG_372)));
		return;
	}

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

void CEndlessUsbToolDlg::SetJSONDownloadState(JSONDownloadState state)
{
    FUNCTION_ENTER_FMT("%d", state);

    m_jsonDownloadState = state;
    if (state == JSONDownloadState::Failed) {
        TrackEvent(_T("Failed"), _T("ErrorCauseJSONDownloadFailed"));
    }
    if (state == JSONDownloadState::Succeeded) {
        // TODO: dissociate this event from the install method
        TrackEvent(_T("JSONDownloadSucceeded"));
    }
    UpdateDownloadableState();
}

#define GPT_MAX_PARTITION_COUNT		128
#define ESP_PART_STARTING_SECTOR	2048
#define ESP_PART_LENGTH_BYTES		65011712
#define MBR_PART_STARTING_SECTOR	129024
#define MBR_PART_LENGTH_BYTES		1048576
#define EXFAT_PART_STARTING_SECTOR	131072

#define EFI_BOOT_SUBDIRECTORY			L"EFI\\BOOT"
#define ENDLESS_BOOT_SUBDIRECTORY		L"EFI\\Endless"
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
#define BACKUP_MBR_IMG					L"mbr.img"
#define MAX_BOOTTRACK_SIZE				(4 * 1024 * 1024) // 4Mb

/* The offset of BOOT_DRIVE_CHECK.  */
#define GRUB_BOOT_MACHINE_DRIVE_CHECK	0x66

// Below defines need to be in this order
#define USB_PROGRESS_UNPACK_BOOT_ZIP		2
#define USB_PROGRESS_MBR_SBR_DONE			4
#define USB_PROGRESS_ESP_CREATION_DONE		6
#define USB_PROGRESS_EXFAT_PREPARED			10
#define USB_PROGRESS_IMG_COPY_DONE			98
#define USB_PROGRESS_ALL_DONE				100

#define CHECK_IF_CANCELLED IFFALSE_GOTOERROR(dlg->m_cancelImageUnpack == 0 && WaitForSingleObject((HANDLE)dlg->m_cancelOperationEvent, 0) != WAIT_OBJECT_0, "Operation has been cancelled")

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
	CString bootFilesPathGz = dlg->m_bootArchive;
	CString bootFilesPath = CEndlessUsbToolApp::TempFilePath(CString(BOOT_COMPONENTS_FOLDER)) + L"\\";
	CString usbFilesPath;
	PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;

	UpdateProgress(OP_NEW_LIVE_CREATION, 0);
	// Unpack boot components
	IFFALSE_GOTOERROR(UnpackBootComponents(bootFilesPathGz, bootFilesPath), "Error unpacking boot components.");

	UpdateProgress(OP_NEW_LIVE_CREATION, USB_PROGRESS_UNPACK_BOOT_ZIP);
	CHECK_IF_CANCELLED;

	// initialize create disk data
	memset(&createDiskData, 0, sizeof(createDiskData));
	createDiskData.PartitionStyle = PARTITION_STYLE_GPT;
	createDiskData.Gpt.MaxPartitionCount = GPT_MAX_PARTITION_COUNT;

	// get disk handle
	hPhysical = GetPhysicalHandle(DriveIndex, TRUE, TRUE);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

	// initialize the disk
	result = DeviceIoControl(hPhysical, IOCTL_DISK_CREATE_DISK, &createDiskData, sizeof(createDiskData), NULL, 0, &size, NULL);
	IFFALSE_GOTOERROR(result != 0, "Error when calling IOCTL_DISK_CREATE_DISK");

	// get disk geometry information
	result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, geometry, sizeof(geometry), &size, NULL);
	IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk geometry.");

	// set initial drive layout data
	memset(layout, 0, sizeof(layout));
	IFFALSE_GOTOERROR(CreateFakePartitionLayout(hPhysical, layout, geometry), "Error on CreateFakePartitionLayout");

	CHECK_IF_CANCELLED;

	// Write MBR and SBR to disk
	IFFALSE_GOTOERROR(WriteMBRAndSBRToUSB(hPhysical, bootFilesPath, DiskGeometry->Geometry.BytesPerSector), "Error on WriteMBRAndSBRToUSB");

	safe_closehandle(hPhysical);

	UpdateProgress(OP_NEW_LIVE_CREATION, USB_PROGRESS_MBR_SBR_DONE);
	CHECK_IF_CANCELLED;

	// Format and mount ESP
	IFFALSE_GOTOERROR(FormatFirstPartitionOnDrive(DriveIndex, FS_FAT32, dlg->m_cancelOperationEvent, L""), "Error on FormatFirstPartitionOnDrive");

	CHECK_IF_CANCELLED;

	IFFALSE_GOTOERROR(MountFirstPartitionOnDrive(DriveIndex, driveLetter), "Error on MountFirstPartitionOnDrive");

	// Copy files to the ESP partition
	IFFALSE_GOTOERROR(CopyFilesToESP(bootFilesPath, driveLetter), "Error when trying to copy files to ESP partition.");

	// Unmount ESP
	if (!DeleteVolumeMountPoint(driveLetter)) uprintf("Failed to unmount volume: %s", WindowsErrorString());

	UpdateProgress(OP_NEW_LIVE_CREATION, USB_PROGRESS_ESP_CREATION_DONE);
	CHECK_IF_CANCELLED;

	// get disk handle again
	hPhysical = GetPhysicalHandle(DriveIndex, TRUE, TRUE);
	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

	// fix partition types and layout
	IFFALSE_GOTOERROR(CreateCorrectPartitionLayout(hPhysical, layout, geometry), "Error on CreateCorrectPartitionLayout");
	safe_closehandle(hPhysical);

	CHECK_IF_CANCELLED;

	// Format and mount exFAT
	IFFALSE_GOTOERROR(FormatFirstPartitionOnDrive(DriveIndex, FS_EXFAT, dlg->m_cancelOperationEvent, EXFAT_PARTITION_NAME_LIVE), "Error on FormatFirstPartitionOnDrive");

	CHECK_IF_CANCELLED;

	IFFALSE_GOTOERROR(MountFirstPartitionOnDrive(DriveIndex, driveLetter), "Error on MountFirstPartitionOnDrive");

	UpdateProgress(OP_NEW_LIVE_CREATION, USB_PROGRESS_EXFAT_PREPARED);
	// Copy files to the exFAT partition
	IFFALSE_GOTOERROR(CopyFilesToexFAT(dlg, bootFilesPath, driveLetter), "Error on CopyFilesToexFAT");

	// Unmount exFAT
	if (!DeleteVolumeMountPoint(driveLetter)) uprintf("Failed to unmount volume: %s", WindowsErrorString());

	UpdateProgress(OP_NEW_LIVE_CREATION, USB_PROGRESS_ALL_DONE);
	CHECK_IF_CANCELLED;

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
	// the EFI spec says the GPT will take the first and last two sectors of
	// the disk, plus however much is allocated for partition entries. there
	// must be at least 128 entries of at least 128 in size - in practice this
	// is what everyone uses. 128 * 128 is 16k which will always be aligned to
	// sector size which is at least 512, but a hypothetical GIANT SECTOR disk
	// would need three sectors reserved for the whole partition table, so we
	// use max()
	LONGLONG gptLength = (2 * DiskGeometry->Geometry.BytesPerSector) +
		max((128 * 128), DiskGeometry->Geometry.BytesPerSector);
	bool returnValue = false;

	DriveLayout->PartitionStyle = PARTITION_STYLE_GPT;
	DriveLayout->PartitionCount = EXPECTED_NUMBER_OF_PARTITIONS;
	IGNORE_RETVAL(CoCreateGuid(&DriveLayout->Gpt.DiskId));

	LONGLONG partitionStart[EXPECTED_NUMBER_OF_PARTITIONS] = {
		ESP_PART_STARTING_SECTOR * DiskGeometry->Geometry.BytesPerSector,
		MBR_PART_STARTING_SECTOR * DiskGeometry->Geometry.BytesPerSector,
		EXFAT_PART_STARTING_SECTOR * DiskGeometry->Geometry.BytesPerSector
	};
	LONGLONG partitionSize[EXPECTED_NUMBER_OF_PARTITIONS] = {
		ESP_PART_LENGTH_BYTES,
		MBR_PART_LENGTH_BYTES,
		// there is a 2nd copy of the GPT at the end of the disk so we
		// subtract the length here to avoid the operation failing
		DiskGeometry->DiskSize.QuadPart - gptLength - partitionStart[2]
	};
	GUID partitionType[EXPECTED_NUMBER_OF_PARTITIONS] = {
		PARTITION_BASIC_DATA_GUID, // will become PARTITION_SYSTEM_GUID later
		PARTITION_BIOS_BOOT_GUID,
		PARTITION_BASIC_DATA_GUID
	};

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
		// Check if user cancelled
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
	// On Windows XP, SHFileOperation fails with code 0x402 ("an unknown error occurred")
	// if there is a trailing slash on the path. In practice we always pass a string with
	// a trailing slash to this function, but let's not rely on that and explicitly check:
	const CString withoutTrailingSlash = directoryPath.Right(1) == _T("\\")
		? directoryPath.Left(directoryPath.GetLength() - 1)
		: directoryPath;
	wsprintf(dir, L"%ls", withoutTrailingSlash);

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
    UpdateUnpackProgress(read_bytes, CEndlessUsbToolDlg::ImageUnpackFileSize);
}

void CEndlessUsbToolDlg::UpdateUnpackProgress(const uint64_t current_bytes, const uint64_t total_bytes)
{
	static int oldPercent = 0;
	static int oldOp = -1;

	float perecentageUnpacked = 100.0f * current_bytes / total_bytes;
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
		// SizeToHumanReadable returns a static buffer
		CStringA current_bytes_str(SizeToHumanReadable(current_bytes, FALSE, use_fake_units));
		CStringA total_bytes_str(SizeToHumanReadable(total_bytes, FALSE, use_fake_units));
		uprintf("Operation %ls(%d) - unpacked %s/%s",
			OperationToStr(CEndlessUsbToolDlg::ImageUnpackOperation),
			CEndlessUsbToolDlg::ImageUnpackOperation,
			current_bytes_str,
			total_bytes_str);
	}
	UpdateProgress(CEndlessUsbToolDlg::ImageUnpackOperation, percentage);
}

bool CEndlessUsbToolDlg::CopyFilesToexFAT(CEndlessUsbToolDlg *dlg, const CString &fromFolder, const CString &driveLetter)
{
	FUNCTION_ENTER;

	bool retResult = false;

	CString usbFilesPath = driveLetter + PATH_ENDLESS_SUBDIRECTORY;
	CString exePath = GetExePath();
	CStringA originalImgFilename = ConvertUnicodeToUTF8(CSTRING_GET_PATH(CSTRING_GET_LAST(dlg->m_unpackedImageSig, '\\'), '.'));

	int createDirResult = SHCreateDirectoryExW(NULL, usbFilesPath, NULL);
	IFFALSE_GOTOERROR(createDirResult == ERROR_SUCCESS || createDirResult == ERROR_FILE_EXISTS, "Error creating directory on USB drive.");

	CEndlessUsbToolDlg::ImageUnpackOperation = OP_NEW_LIVE_CREATION;
	CEndlessUsbToolDlg::ImageUnpackPercentStart = USB_PROGRESS_EXFAT_PREPARED;
	CEndlessUsbToolDlg::ImageUnpackPercentEnd = USB_PROGRESS_IMG_COPY_DONE;
	CEndlessUsbToolDlg::ImageUnpackFileSize = dlg->m_selectedFileSize;

	IFFALSE_GOTOERROR(dlg->UnpackImage(dlg->m_localFile, usbFilesPath + ENDLESS_IMG_FILE_NAME), "Error unpacking image to USB drive");
	IFFALSE_GOTOERROR(0 != CopyFile(dlg->m_unpackedImageSig, usbFilesPath + CSTRING_GET_LAST(dlg->m_unpackedImageSig, '\\'), FALSE), "Error copying image signature file to drive.");

	FILE *liveFile;
	IFFALSE_GOTOERROR(0 == _wfopen_s(&liveFile, usbFilesPath + EXFAT_ENDLESS_LIVE_FILE_NAME, L"w"), "Error creating empty live file.");
	fwrite((const char*)originalImgFilename, 1, originalImgFilename.GetLength(), liveFile);
	fclose(liveFile);

	// copy grub to USB drive
	IFFALSE_GOTOERROR(CopyMultipleItems(fromFolder + GRUB_BOOT_SUBDIRECTORY, usbFilesPath), "Error copying grub folder to USB drive.");

	IFFALSE_GOTOERROR(0 != CopyFile(dlg->m_bootArchive, usbFilesPath + CSTRING_GET_LAST(dlg->m_bootArchive, '\\'), FALSE), "Error copying boot.zip file to drive.");
	IFFALSE_GOTOERROR(0 != CopyFile(dlg->m_bootArchiveSig, usbFilesPath + CSTRING_GET_LAST(dlg->m_bootArchiveSig, '\\'), FALSE), "Error copying boot.zip signature file to drive.");

	DWORD attributes = FILE_ATTRIBUTE_READONLY;
	IFFALSE_PRINTERROR(SetAttributesForFilesInFolder(usbFilesPath, attributes), "Error on SetFileAttributes");

	IFFALSE_GOTOERROR(0 != CopyFile(exePath, driveLetter + CSTRING_GET_LAST(exePath, '\\'), FALSE), "Error copying installer binary to drive.");

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
#define DB_PROGRESS_CHECK_PARTITION		1
#define DB_PROGRESS_UNPACK_BOOT_ZIP		3
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
	CString bootFilesPath = CEndlessUsbToolApp::TempFilePath(CString(BOOT_COMPONENTS_FOLDER)) + L"\\";
	CStringW exeFilePath = GetExePath();
	const bool isBIOS = IsLegacyBIOSBoot();

	systemDriveLetter = GetSystemDrive();
	endlessFilesPath = systemDriveLetter + PATH_ENDLESS_SUBDIRECTORY;
	endlessImgPath = endlessFilesPath + ENDLESS_IMG_FILE_NAME;

	// Double-check this is an NTFS drive and (on BIOS systems) that there's no
	// non-Windows bootloader. Note that we deliberately jump to 'done' in this
	// case rather than 'error' -- we have not yet written anything to
	// endlessFilesPath so we shouldn't try to delete it, and we set
	// m_lastErrorCause directly.
	IFFALSE_GOTO(CanInstallToDrive(systemDriveLetter, isBIOS, dlg->m_lastErrorCause), "Can't install to this drive", done);

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_CHECK_PARTITION);

	IFFALSE_PRINTERROR(ChangeAccessPermissions(endlessFilesPath, false), "Error on granting Endless files permissions.");

	// Unpack boot components
	IFFALSE_GOTOERROR(UnpackBootComponents(dlg->m_bootArchive, bootFilesPath), "Error unpacking boot components.");

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_UNPACK_BOOT_ZIP);
	CHECK_IF_CANCELLED;

	// Create endless folder
	int createDirResult = SHCreateDirectoryExW(NULL, endlessFilesPath, NULL);
	IFFALSE_GOTOERROR(createDirResult == ERROR_SUCCESS || createDirResult == ERROR_ALREADY_EXISTS, "Error creating directory on system drive.");

	// Check folder is not set to compressed
	{
		CString folderName = CSTRING_GET_PATH(endlessFilesPath, L'\\');
		IFFALSE_GOTOERROR(EnsureUncompressed(folderName), "Error setting folder to uncompressed.");
	}

	// Copy ourselves to the <SYSDRIVE>:\endless directory
	IFFALSE_GOTOERROR(0 != CopyFile(exeFilePath, endlessFilesPath + UninstallerFileName(), FALSE), "Error copying exe uninstaller file.");

	// Add uninstall entry for Control Panel
	IFFALSE_GOTOERROR(AddUninstallRegistryKeys(endlessFilesPath + UninstallerFileName(), endlessFilesPath), "Error on AddUninstallRegistryKeys.");

	CEndlessUsbToolDlg::ImageUnpackOperation = OP_SETUP_DUALBOOT;
	CEndlessUsbToolDlg::ImageUnpackPercentStart = DB_PROGRESS_UNPACK_BOOT_ZIP;
	CEndlessUsbToolDlg::ImageUnpackPercentEnd = DB_PROGRESS_FINISHED_UNPACK;
	CEndlessUsbToolDlg::ImageUnpackFileSize = dlg->m_selectedFileSize;
	IFFALSE_GOTOERROR(dlg->UnpackImage(dlg->m_localFile, endlessImgPath), "Error unpacking dual-boot image");

	// Clear READONLY flag inherited from endless.img on live USB.
	IFFALSE_PRINTERROR(SetFileAttributes(endlessImgPath, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN),
		"Failed to clear FILE_ATTRIBUTE_READONLY; ExtendImageFile will probably now fail");
	// extend this file so it reaches the required size
	IFFALSE_GOTOERROR(ExtendImageFile(endlessImgPath, dlg->m_nrGigsSelected), "Error extending Endless image file");

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_FINISHED_UNPACK);
	CHECK_IF_CANCELLED;

	// Copy grub
	IFFALSE_GOTOERROR(CopyMultipleItems(bootFilesPath + GRUB_BOOT_SUBDIRECTORY, endlessFilesPath), "Error copying grub folder to system drive.");
	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_COPY_GRUB_FOLDER);
	CHECK_IF_CANCELLED;

	if (isBIOS) {
		IFFALSE_GOTOERROR(WriteMBRAndSBRToWinDrive(dlg, systemDriveLetter, bootFilesPath, endlessFilesPath), "Error on WriteMBRAndSBRToWinDrive");
	} else {
		IFFALSE_GOTOERROR(SetupEndlessEFI(systemDriveLetter, bootFilesPath), "Error on SetupEndlessEFI");
	}

	// set Endless file permissions
	IFFALSE_PRINTERROR(ChangeAccessPermissions(endlessFilesPath, true), "Error on setting Endless files permissions.");

	// set the registry key for Windows clock in UTC
	IFFALSE_PRINTERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UTC_TIME_PATH, REGKEY_UTC_TIME, CComVariant(1)), "Error on setting Windows clock to UTC.");

	// disable Fast Start (aka Hiberboot) so that the NTFS partition is always cleanly unmounted at shutdown:
	IFFALSE_PRINTERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FASTBOOT_PATH, REGKEY_FASTBOOT, CComVariant(0)), "Error on disabling fastboot.");

	UpdateProgress(OP_SETUP_DUALBOOT, DB_PROGRESS_MBR_OR_EFI_SETUP);

	goto done;

error:
	if (GetLastError() == ERROR_DISK_FULL) {
		dlg->m_lastErrorCause = ErrorCause_t::ErrorCauseInstallFailedDiskFull;
	}

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

bool CEndlessUsbToolDlg::EnsureUncompressed(const CString &filePath)
{
	FUNCTION_ENTER;
	bool success = false;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	USHORT compression;
	DWORD size = 0;
	BOOL result;

	hFile = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	IFFALSE_GOTOERROR(hFile != INVALID_HANDLE_VALUE, "CreateFile returned INVALID_HANDLE_VALUE.");

	result = DeviceIoControl(hFile, FSCTL_GET_COMPRESSION, NULL, 0, &compression, sizeof(compression), &size, NULL);
	IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying compression status.");

	uprintf("Path %ls has compression status %hu.", filePath, compression);

	if (compression != COMPRESSION_FORMAT_NONE) {
		compression = COMPRESSION_FORMAT_NONE;
		result = DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &compression, sizeof(compression), NULL, 0, &size, NULL);
		IFFALSE_GOTOERROR(result != 0, "Error on setting compression status.");

		uprintf ("Set %ls to uncompressed.", filePath);
	}

	success = true;

error:
	safe_closehandle(hFile);

	return success;
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
	
    if (lCurrentSize.QuadPart < lsize.QuadPart) {
        uprintf("Trying to extend Endless image from %I64i bytes to %I64i bytes which is about %s", lCurrentSize.QuadPart, lsize.QuadPart, SizeToHumanReadable(lsize.QuadPart, FALSE, use_fake_units));
        IFFALSE_GOTOERROR(SetFilePointerEx(endlessImage, lsize, NULL, FILE_BEGIN) != 0, "Error on SetFilePointerEx");
        IFFALSE_GOTOERROR(SetEndOfFile(endlessImage), "Error on SetEndOfFile");
        IFFALSE_GOTOERROR(SetFileValidData(endlessImage, lsize.QuadPart), "Error on SetFileValidData");
        uprintf("Extended Endless image");
    } else {
        uprintf("Image size == %I64iB >= %I64iB == selected size", lCurrentSize.QuadPart, lsize.QuadPart);
        uprintf("This is a bug -- continuing without truncating the image");
    }

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

// Checks preconditions for installing Endless OS to a given drive. Guaranteed to only modify 'cause' on error.
bool CEndlessUsbToolDlg::CanInstallToDrive(const CString & systemDriveLetter, const bool isBIOS, ErrorCause &cause)
{
	FUNCTION_ENTER;
	
	wchar_t fileSystemType[MAX_PATH + 1];
	IFFALSE_GOTOERROR(GetVolumeInformation(systemDriveLetter, NULL, 0, NULL, NULL, NULL, fileSystemType, MAX_PATH + 1), "Error on GetVolumeInformation");
	uprintf("File system type '%ls'", fileSystemType);
	if (0 != wcscmp(fileSystemType, L"NTFS")) {
		cause = ErrorCauseNotNTFS;
		return false;
	}

	if (!isBIOS) {
		if (!is_x64()) {
			uprintf("EFI system with 32-bit Windows; assuming IA32 EFI (which is unsupported)");
			cause = ErrorCause32BitEFI;
			return false;
		} else {
			uprintf("EFI system with 64-bit Windows; assuming x64 EFI (which is ok)");
			// assume there will be no problem installing GRUB
			return true;
		}
	}

	if (CEndlessUsbToolApp::m_enableOverwriteMbr) {
		uprintf("Not checking for Windows MBR as /forcembr is enabled.");
		return true;
	} else {
		bool ret = false;

		HANDLE hPhysical = GetPhysicalFromDriveLetter(systemDriveLetter);
		if (hPhysical != INVALID_HANDLE_VALUE) {
			FAKE_FD fake_fd = { 0 };
			FILE* fp = (FILE*)&fake_fd;
			fake_fd._handle = (char*)hPhysical;

			if (IsWindowsMBR(fp, systemDriveLetter)) {
				ret = true;
			} else {
				cause = ErrorCauseNonWindowsMBR;
			}
		} else {
			PRINT_ERROR_MSG("can't check MBR; installation would fail so giving up now");
			cause = ErrorCauseCantCheckMBR;
		}
		safe_closehandle(hPhysical);
		return ret;
	}

error:
	// Something unexpected happened happened
	cause = ErrorCauseWriteFailed;
	return false;
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

// Unconditionally write GRUB from 'bootFilesPath' to 'systemDriveLetter', backing up the current boot track to 'endlessFilesPath'.
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
	LONGLONG minStartingOffset = MAX_BOOTTRACK_SIZE;
	PARTITION_INFORMATION_EX *partition = NULL;
	for (DWORD index = 0; index < DriveLayout->PartitionCount; index++) {
		partition = &(DriveLayout->PartitionEntry[index]);
		IFFALSE_GOTOERROR(partition->PartitionStyle == PARTITION_STYLE_MBR, "non-MBR partition in supposedly-MBR table(?)");
		if (partition->Mbr.PartitionType == PARTITION_ENTRY_UNUSED) {
			uprintf("Partition %d is unused", index);
		} else {
			uprintf("Partition %d starting offset = %I64i", index, partition->StartingOffset.QuadPart);
			minStartingOffset = min(minStartingOffset, partition->StartingOffset.QuadPart);
		}
	}

	dlg->TrackEvent(_T("BootTrackSize"), minStartingOffset);

	IFFALSE_GOTOERROR(0 == _wfopen_s(&boottrackImgFile, endlessFilesPath + BACKUP_BOOTTRACK_IMG, L"wb"), "Error opening boottrack.img file");
	boottrackData = (unsigned char*)malloc(DiskGeometry->Geometry.BytesPerSector);
	LONGLONG boottrackNrSectors = minStartingOffset / DiskGeometry->Geometry.BytesPerSector;
	uprintf("Trying to save %I64i sectors in file %ls", boottrackNrSectors, endlessFilesPath + BACKUP_BOOTTRACK_IMG);
	for (LONGLONG i = 0; i < boottrackNrSectors; i++) {
		int64_t result = read_sectors(hPhysical, DiskGeometry->Geometry.BytesPerSector, i, 1, boottrackData);
		IFFALSE_GOTOERROR(result == DiskGeometry->Geometry.BytesPerSector, "Error reading from disk.");
		IFFALSE_GOTOERROR(fwrite(boottrackData, 1, DiskGeometry->Geometry.BytesPerSector, boottrackImgFile) == DiskGeometry->Geometry.BytesPerSector, "Error writing data to boottrack.img.");
	}
	safe_closefile(boottrackImgFile);

	// Write core.img
	IFFALSE_GOTOERROR(grub_util_bios_setup(coreImgFilePath, hPhysical, boottrackNrSectors), "Error writing SBR to disk");

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

	// reusing boottrackImgFile, saving mbr.img for uninstall purposes
	IFFALSE_GOTOERROR(0 == _wfopen_s(&boottrackImgFile, endlessFilesPath + BACKUP_MBR_IMG, L"wb"), "Error opening mbr.img file");
	fwrite(endlessMBRData, 1, MBR_WINDOWS_NT_MAGIC, boottrackImgFile);
	safe_closefile(boottrackImgFile);

	// write boot.img to disk
	fake_fd._handle = (char*)hPhysical;
	set_bytes_per_sector(SelectedDrive.Geometry.BytesPerSector);
	IFFALSE_GOTOERROR(write_data(fp, 0x0, endlessMBRData, MBR_WINDOWS_NT_MAGIC) != 0, "Error on write_data with boot.img contents.");
    IFFALSE_PRINTERROR(write_bootmark(fp, DiskGeometry->Geometry.BytesPerSector), "Error on write_bootmark");

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

/* Protect or unprotect files in 'path' (C:\endless or $eoslive:\endless) from modification,
   *non-*-recursively.
*/
BOOL CEndlessUsbToolDlg::SetAttributesForFilesInFolder(CString path, DWORD attributes)
{
	WIN32_FIND_DATA findFileData;
	HANDLE findFilesHandle = FindFirstFile(path + ALL_FILES, &findFileData);

	uprintf("SetAccessToFilesInFolder(L\"%ls\", %x)", path, attributes);
	DWORD folderAttributes = attributes & ~FILE_ATTRIBUTE_HIDDEN;

	uprintf("SetFileAttributes(L\"%ls\", %x)", path, folderAttributes);
	IFFALSE_PRINTERROR(SetFileAttributes(path, folderAttributes) != 0, "Error on SetFileAttributes");

	if (findFilesHandle == INVALID_HANDLE_VALUE) {
		uprintf("UpdateFileEntries: No files found in directory \"%ls\"", path);
	} else {
		do {
			if ((0 == wcscmp(findFileData.cFileName, L".") || 0 == wcscmp(findFileData.cFileName, L".."))) continue;

			if (0 != wcscmp(findFileData.cFileName, UninstallerFileName())) {
				IFFALSE_PRINTERROR(SetFileAttributes(path + findFileData.cFileName, attributes) != 0, "Error on SetFileAttributes for child");

				// We deliberately do not recurse into subdirectories.
				//
				// In Endless OS, files and directories marked +h/+s on NTFS/exFAT partitions (respectively) are
				// hidden from directory listings, but can be manipulated if you know the full path. If we mark
				// the *contents* of \endless\grub as hidden & system then it's impossible to remove them,
				// because we don't have a full list of its contents.
				//
				// If \endless\grub is hidden from listings but its contents are not, you can
				// rm -r $eoslive/endless/grub if you know its full path -- which we do.
			}
		} while (FindNextFile(findFilesHandle, &findFileData) != 0);

		FindClose(findFilesHandle);
	}

	return true;
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
	if (restrictAccess) {
		DWORD attributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;
		IFFALSE_PRINTERROR(SetAttributesForFilesInFolder(path, attributes), "Error on SetAttributesForFilesInFolder");
	}

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
	if (!restrictAccess) {
		DWORD attributes = FILE_ATTRIBUTE_NORMAL;
		IFFALSE_PRINTERROR(SetAttributesForFilesInFolder(path, attributes), "Error on SetFileAttributes");
	}

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
		if (nWindowsVersion > WINDOWS_XP) {
			IFFALSE_GOTOERROR(result != 0 && (GetLastError() == ERROR_SUCCESS || GetLastError() == ERROR_INSUFFICIENT_BUFFER), "Could not get needed size for module path");
			neededSize = result;
			IFFALSE_CONTINUE(GetLastError() != ERROR_INSUFFICIENT_BUFFER, "Not enough memory allocated for buffer. Trying again.");
		} else {
			IFFALSE_GOTOERROR(result != 0, "Windows XP: Could not get needed size for module path");
		}

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
#define REGKEY_UNINSTALL_STRING	L"UninstallString"
#define REGKEY_INSTALL_LOCATION	L"InstallLocation"
#define REGKEY_PUBLISHER		L"Publisher"
#define REGKEY_HELP_LINK		L"HelpLink"
#define REGKEY_NOCHANGE			L"NoChange"
#define REGKEY_NOMODIFY			L"NoModify"

#define REGKEY_DISPLAYNAME_TEXT	ENDLESS_OS_NAME
#define REGKEY_PUBLISHER_TEXT	L"Endless Mobile, Inc."

BOOL CEndlessUsbToolDlg::AddUninstallRegistryKeys(const CStringW &uninstallExePath, const CStringW &installPath)
{
	CRegKey registryKey;
	LSTATUS result;
	BOOL retResult = FALSE;
	CStringW displayVersion;

	result = registryKey.Create(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS);
	uprintf("Result %d on creating key '%ls'", result, REGKEY_UNINSTALLENDLESS);
	IFFALSE_GOTOERROR(result == ERROR_SUCCESS, "Error on CRegKey::Create.");

	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_DISPLAYNAME, REGKEY_DISPLAYNAME_TEXT, false), "Error on REGKEY_DISPLAYNAME");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_HELP_LINK, UTF8ToBSTR(lmprintf(MSG_314)), false), "Error on REGKEY_HELP_LINK");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_UNINSTALL_STRING, CComBSTR(uninstallExePath), false), "Error on REGKEY_UNINSTALL_STRING");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_INSTALL_LOCATION, CComBSTR(installPath), false), "Error on REGKEY_INSTALL_LOCATION");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_PUBLISHER, REGKEY_PUBLISHER_TEXT, false), "Error on REGKEY_PUBLISHER");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_NOCHANGE, 1, false), "Error on REGKEY_NOCHANGE");
	IFFALSE_GOTOERROR(SetEndlessRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_UNINSTALLENDLESS, REGKEY_NOMODIFY, 1, false), "Error on REGKEY_NOMODIFY");

	retResult = TRUE;
error:

	return retResult;
}

bool CEndlessUsbToolDlg::UninstallEndlessMBR(const CString & systemDriveLetter, const CString & endlessFilesPath, HANDLE hPhysical, ErrorCause & cause)
{
    FAKE_FD fake_fd = { 0 };
    FILE* fp = (FILE*)&fake_fd;
    fake_fd._handle = (char*)hPhysical;

    IFTRUE_RETURN_VALUE(IsWindowsMBR(fp, systemDriveLetter), "Windows MBR detected so skipping MBR writing", true);
    if (!CEndlessUsbToolApp::m_enableOverwriteMbr && !IsEndlessMBR(fp, endlessFilesPath)) {
        cause = ErrorCause_t::ErrorCauseNonEndlessMBR;
        return false;
    }

    // restore boottrack.img if present and use embedded grub only as fallback
    IFTRUE_RETURN_VALUE(RestoreOriginalBoottrack(endlessFilesPath, hPhysical, fp), "Success on restoring original boottrack", true);
    uprintf("Failure on restoring original boottrack, falling back to embedded MBRs.");

    if (nWindowsVersion >= WINDOWS_7) {
        IFFALSE_RETURN_VALUE(write_win7_mbr(fp), "Error on write_win7_mbr", false);
    } else if(nWindowsVersion == WINDOWS_VISTA) {
        IFFALSE_RETURN_VALUE(write_vista_mbr(fp), "Error on write_vista_mbr", false);
    } else if (nWindowsVersion == WINDOWS_2003 || nWindowsVersion == WINDOWS_XP) {
        IFFALSE_RETURN_VALUE(write_2000_mbr(fp), "Error on write_2000_mbr", false);
    } else {
        uprintf("Restore MBR not supported for this windows version '%s', ver %d", WindowsVersionStr, nWindowsVersion);
        return false;
    }

    return true;
}

bool CEndlessUsbToolDlg::UninstallEndlessEFI(const CString & systemDriveLetter, HANDLE hPhysical, bool & found_boot_entry)
{
    IFFALSE_PRINTERROR(EFIRequireNeededPrivileges(), "Error on EFIRequireNeededPrivileges.");

    IFFALSE_PRINTERROR(EFIRemoveEntry(ENDLESS_OS_NAME, found_boot_entry), "Error on EFIRemoveEntry, continuing with uninstall anyway.");

    const char *espMountLetter = NULL;
    IFFALSE_RETURN_VALUE(MountESPFromDrive(hPhysical, &espMountLetter, systemDriveLetter), "Error on MountESPFromDrive", false);
    CString windowsEspDriveLetter = UTF8ToCString(espMountLetter);

    RemoveNonEmptyDirectory(windowsEspDriveLetter + L"\\" + ENDLESS_BOOT_SUBDIRECTORY);

    if (espMountLetter != NULL) {
        AltUnmountVolume(espMountLetter);
    }

    return true;
}

BOOL CEndlessUsbToolDlg::UninstallDualBoot(CEndlessUsbToolDlg *dlg)
{
	FUNCTION_ENTER;

	BOOL retResult = FALSE;
	uint32_t popupMsgId = IsCoding() ? MSG_380 : MSG_363;
	UINT popupStyle = MB_OK | MB_ICONERROR;

	CString systemDriveLetter = GetSystemDrive();
	CString endlessFilesPath = systemDriveLetter + PATH_ENDLESS_SUBDIRECTORY;
	CStringA endlessImgPath = ConvertUnicodeToUTF8(endlessFilesPath + ENDLESS_IMG_FILE_NAME);
	HANDLE hPhysical = GetPhysicalFromDriveLetter(systemDriveLetter);
	CString exePath = GetExePath();
	CString image_state;

	IFFALSE_GOTOERROR(hPhysical != INVALID_HANDLE_VALUE, "Error on acquiring disk handle.");

	if (IsLegacyBIOSBoot()) { // remove MBR entry
		IFFALSE_GOTOERROR(
			UninstallEndlessMBR(systemDriveLetter, endlessFilesPath, hPhysical, dlg->m_lastErrorCause),
			"Couldn't uninstall Endless MBR");
	} else { // remove EFI entry
		bool found_boot_entry;
		bool ret = UninstallEndlessEFI(systemDriveLetter, hPhysical, found_boot_entry);
		dlg->TrackEvent(_T("FoundBootEntry"), found_boot_entry ? _T("True") : _T("False"));
		IFFALSE_GOTOERROR(ret, "UninstallEndlessEFI failed");
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


	// remove <SYSDRIVE>:\Endless after checking whether image is present and has been booted
	IFFALSE_PRINTERROR(ChangeAccessPermissions(endlessFilesPath, false), "Error on granting Endless files permissions.");

	image_state = L"Invalid";
	BOOL booted;
	if (is_eos_image_valid (endlessImgPath, &booted)) {
		if (booted)
			image_state = L"Booted";
		else
			image_state = L"Unbooted";
	}
	dlg->TrackEvent(L"ImageState", image_state);

	uprintf("exePath=%ls, endless=%ls", exePath, endlessFilesPath);
	DelayDeleteFolder(endlessFilesPath);

	// set success message and icon
	popupMsgId = IsCoding() ? MSG_377 : MSG_362;
	popupStyle = MB_OK | MB_ICONINFORMATION;
	retResult = TRUE;

error:
	safe_closehandle(hPhysical);

	AfxMessageBox(UTF8ToCString(lmprintf(popupMsgId)), popupStyle);

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
	INT execResult = (INT)ShellExecute(0, 0, cmd, params, GetSystemDrive(), SW_HIDE);
	uprintf("ShellExecute returned %d", execResult);
	IFFALSE_GOTOERROR(execResult > 32, "Error returned by ShellExecute");

error:
	if (shortPath != NULL) {
		delete [] shortPath;
		shortPath = NULL;
	}
}

static int _lowerCaseExeNameFindSubstring(CStringW exePath, const wchar_t* substring, bool* isSuffix)
{
	CStringW exeName = CSTRING_GET_LAST(exePath, '\\');
	exeName.MakeLower();
	int pos = exeName.Find(substring);
	if (pos >= 0 && isSuffix != NULL) {
		size_t substringLen = wcslen(substring);
		*isSuffix = (pos + substringLen == exeName.GetLength());
	}
	return pos;
}

static bool _matchesUninstallerSuffix(CStringW exePath)
{
	bool isSuffix = false;
	int pos = _lowerCaseExeNameFindSubstring(exePath, L"-uninstaller.exe", &isSuffix);
	return (pos > 0) && isSuffix;
}

bool CEndlessUsbToolDlg::IsUninstaller()
{
	static const bool is_uninstaller = _matchesUninstallerSuffix(GetExePath());
	return is_uninstaller;
}

bool CEndlessUsbToolDlg::IsCoding()
{
	static const bool is_coding =
	    (_lowerCaseExeNameFindSubstring(GetExePath(), L"-code-", NULL) > 0);
	return is_coding;
}

const wchar_t* CEndlessUsbToolDlg::UninstallerFileName()
{
	return IsCoding()
		? L"endless-code-uninstaller.exe"
		: L"endless-uninstaller.exe";
}

const char* CEndlessUsbToolDlg::JsonLiveFile(bool withCompressedSuffix)
{
	if (withCompressedSuffix) {
		return IsCoding()
			? JSON_CODING_LIVE_FILE JSON_SUFFIX
			: JSON_LIVE_FILE        JSON_SUFFIX;
	}
	return IsCoding()
		? JSON_CODING_LIVE_FILE
		: JSON_LIVE_FILE;
}

const wchar_t* CEndlessUsbToolDlg::JsonLiveFileURL()
{
    return IsCoding()
        ? (PRIVATE_JSON_URLPATH _T(JSON_CODING_LIVE_FILE JSON_SUFFIX))
        : (RELEASE_JSON_URLPATH _T(JSON_LIVE_FILE        JSON_SUFFIX));
}

const char* CEndlessUsbToolDlg::JsonInstallerFile(bool withCompressedSuffix)
{
	if (withCompressedSuffix) {
        return JSON_INSTALLER_FILE JSON_SUFFIX;
	}
	return JSON_INSTALLER_FILE;
}

const wchar_t* CEndlessUsbToolDlg::JsonInstallerFileURL()
{
    return RELEASE_JSON_URLPATH _T(JSON_INSTALLER_FILE JSON_SUFFIX);
}

bool CEndlessUsbToolDlg::ShouldUninstall()
{
	return (PathFileExists(GetSystemDrive() + PATH_ENDLESS_SUBDIRECTORY + ENDLESS_IMG_FILE_NAME)
		|| IsUninstaller());
}

#define MIN_DATE_IMAGE_BOOT	L"161020"

// Image boot means support for booting from endless/endless.img
// on dual boot (ntfs) and combined USB (exfat)
bool CEndlessUsbToolDlg::HasImageBootSupport(const CString &version, const CString &date)
{
	return version[0] >= '3' && date >= MIN_DATE_IMAGE_BOOT;
}

bool CEndlessUsbToolDlg::PackedImageAlreadyExists(const CString &filePath, ULONGLONG expectedSize, ULONGLONG expectedUnpackedSize, bool isInstaller)
{
	bool exists = false;
	CFile file;
	bool isUnpackedImage = false;

	uprintf("Called with '%ls', expected size = [%I64u] and expected unpacked size = [%I64u]", filePath, expectedSize, expectedUnpackedSize);
	if(!isInstaller) {
		isUnpackedImage = RemoteMatchesUnpackedImg(filePath);
	}
	CString actualFile = isUnpackedImage ? GET_IMAGE_PATH(ENDLESS_IMG_FILE_NAME) : filePath;
	IFFALSE_GOTOERROR(PathFileExists(actualFile), "File doesn't exists");

	CompressionType unused;
	ULONGLONG extractedSize = GetExtractedSize(actualFile, isInstaller, unused);
	file.Open(actualFile, CFile::modeRead);
	uprintf("size=%I64u, extracted size=%I64u", file.GetLength(), extractedSize);
	if (isUnpackedImage) {
		IFFALSE_GOTOERROR(expectedUnpackedSize == file.GetLength(), "Extracted size doesn't match");
	} else {
		IFFALSE_GOTOERROR(expectedUnpackedSize == extractedSize, "Extracted size doesn't match");
		IFFALSE_GOTOERROR(expectedSize == file.GetLength(), "Extracted size doesn't match");
	}
	exists = true;
error:
	return exists;
}

#define SIGNATURE_FILE_SIZE	819

ULONGLONG CEndlessUsbToolDlg::GetActualDownloadSize(const RemoteImageEntry &r, bool fullSize)
{
	ULONGLONG size = SIGNATURE_FILE_SIZE; // img.gz.asc
	CString localFile = GET_IMAGE_PATH(CSTRING_GET_LAST(r.urlFile, '/'));
	bool localFileExists = PackedImageAlreadyExists(localFile, r.compressedSize, r.extractedSize, false);
	size += !fullSize && localFileExists ? 0 : r.compressedSize;

	if (IsDualBootOrCombinedUsb()) {
		size += r.bootArchiveSize + SIGNATURE_FILE_SIZE * 2; // img.asc and boot.zip.asc
	} else if (m_selectedInstallMethod == InstallMethod_t::ReformatterUsb) {
		CString installerFile = GET_IMAGE_PATH(CSTRING_GET_LAST(m_installerImage.urlFile, '/'));
		bool installerExists = PackedImageAlreadyExists(installerFile, m_installerImage.compressedSize, m_installerImage.extractedSize, true);
		size += SIGNATURE_FILE_SIZE; // installer img.gz.asc
		size += !fullSize && installerExists ? 0 : m_installerImage.compressedSize;
	}

	return size;
}

bool CEndlessUsbToolDlg::RemoteMatchesUnpackedImg(const CString &remoteFilePath, CString *unpackedImgSig)
{
	pFileImageEntry_t localEntry = NULL;
	if (m_imageFiles.Lookup(GET_IMAGE_PATH(ENDLESS_IMG_FILE_NAME), localEntry)) {
		if (CSTRING_GET_PATH(localEntry->unpackedImgSigPath, '.') == CSTRING_GET_PATH(remoteFilePath, '.')) {
			if (unpackedImgSig != NULL) *unpackedImgSig = localEntry->unpackedImgSigPath;
			return true;
		}
	}
	return false;
}

bool CEndlessUsbToolDlg::IsDualBootOrCombinedUsb()
{
	return m_selectedInstallMethod == InstallMethod_t::InstallDualBoot || m_selectedInstallMethod == InstallMethod_t::CombinedUsb;
}

void CEndlessUsbToolDlg::UpdateDualBootTexts()
{
	if (ShouldUninstall()) {
		SetElementText(_T(ELEMENT_DUALBOOT_TITLE), UTF8ToBSTR(lmprintf(MSG_364)));
		SetElementText(_T(ELEMENT_DUALBOOT_DESCRIPTION), UTF8ToBSTR(lmprintf(MSG_365)));
		SetElementText(_T(ELEMENT_DUALBOOT_INSTALL_BUTTON), UTF8ToBSTR(lmprintf(MSG_366)));
		SetElementText(_T(ELEMENT_DUALBOOT_RECOMMENDATION), UTF8ToBSTR(lmprintf(MSG_367)));
		CallJavascript(_T(JS_ENABLE_BUTTON), CComVariant(HTML_BUTTON_ID(_T(ELEMENT_DUALBOOT_INSTALL_BUTTON))), CComVariant(TRUE));
	}

	if (IsUninstaller()) {
		CallJavascript(_T(JS_SHOW_ELEMENT), CComVariant(ELEMENT_DUALBOOT_ADVANCED_TEXT), CComVariant(FALSE));
	}
}

void CEndlessUsbToolDlg::QueryAndDoUninstall()
{
	int selected = AfxMessageBox(UTF8ToCString(lmprintf(IsCoding() ? MSG_376 : MSG_361)), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

	if (selected != IDYES)
		return;

	SetSelectedInstallMethod(InstallMethod_t::UninstallDualBoot);

	Analytics::instance()->screenTracking(_T("UninstallPage"));

	ShowWindow(SW_HIDE);

	TrackEvent(_T("Started"));

	if (UninstallDualBoot(this)) {
		TrackEvent(_T("Completed"));
	} else {
		TrackEvent(_T("Failed"), ErrorCauseToStr(m_lastErrorCause));
		Analytics::instance()->exceptionTracking(_T("UninstallError"), TRUE);
	}

	EndDialog(IDCLOSE);
}

bool CEndlessUsbToolDlg::IsEndlessMBR(FILE* fp, const CString &endlessPath)
{
	CString mbrImgPath = endlessPath + BACKUP_MBR_IMG;
	bool retResult = false;
	unsigned char endlessMbrData[MBR_WINDOWS_NT_MAGIC], existingMbrData[MBR_WINDOWS_NT_MAGIC];
	FILE *mbrFile = NULL;

	memset(endlessMbrData, 0, MBR_WINDOWS_NT_MAGIC);
	memset(existingMbrData, 0, MBR_WINDOWS_NT_MAGIC);
	// load backup mbr data
	errno_t openRet = _wfopen_s(&mbrFile, mbrImgPath, L"rb");
	if (openRet == ENOENT) {
		uprintf("%ls missing; assuming the Endless OS GRUB MBR is installed", mbrImgPath);
		return TRUE;
	} else {
		IFFALSE_GOTOERROR(0 == openRet, "Error opening mbr.img file");
	}
	fread(endlessMbrData, 1, MBR_WINDOWS_NT_MAGIC, mbrFile);
	safe_closefile(mbrFile);
	// load current mbr data
	IFFALSE_GOTOERROR(read_data(fp, 0x0, existingMbrData, MBR_WINDOWS_NT_MAGIC), "Error reading current MBR");

	retResult = (0 == memcmp(existingMbrData, endlessMbrData, MBR_WINDOWS_NT_MAGIC));
error:
	return retResult;
}

bool CEndlessUsbToolDlg::RestoreOriginalBoottrack(const CString &endlessPath, HANDLE hPhysical, FILE *fp)
{
	FUNCTION_ENTER;

	bool retResult = false;
	DWORD size;
	BYTE geometry[256] = { 0 };
	PDISK_GEOMETRY_EX DiskGeometry = (PDISK_GEOMETRY_EX)(void*)geometry;
	FILE *boottrackImgFile = NULL;
	unsigned char *boottrackData[MBR_WINDOWS_NT_MAGIC];

	memset(boottrackData, 0, MBR_WINDOWS_NT_MAGIC);

	BOOL result = DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, geometry, sizeof(geometry), &size, NULL);
	IFFALSE_GOTOERROR(result != 0 && size > 0, "Error on querying disk geometry.");

	set_bytes_per_sector(SelectedDrive.Geometry.BytesPerSector);

	IFFALSE_GOTOERROR(0 == _wfopen_s(&boottrackImgFile, endlessPath + BACKUP_BOOTTRACK_IMG, L"rb"), "Error opening boottrack.img file");
	IFFALSE_GOTOERROR(fread(boottrackData, 1, MBR_WINDOWS_NT_MAGIC, boottrackImgFile), "Error reading from boottrack.img");
	IFFALSE_GOTOERROR(write_data(fp, 0x0, boottrackData, MBR_WINDOWS_NT_MAGIC), "Error writing boottrack.img data to MBR");

	retResult = true;
error:
	safe_closefile(boottrackImgFile);
	return retResult;
}

CComBSTR CEndlessUsbToolDlg::GetDownloadString(const RemoteImageEntry &imageEntry)
{
	ULONGLONG size = GetActualDownloadSize(imageEntry);
	CStringA sizeT = SizeToHumanReadable(size, FALSE, use_fake_units);
	ULONGLONG fullSize = GetActualDownloadSize(imageEntry, true);
	CStringA fullSizeT = SizeToHumanReadable(fullSize, FALSE, use_fake_units);
	return UTF8ToBSTR(size == fullSize ? lmprintf(MSG_369, fullSizeT) : lmprintf(MSG_315, sizeT, fullSizeT));
}
