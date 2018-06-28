#pragma once

#include <windows.h>
#include <bits.h>
#include <initializer_list>

#include "GeneralCode.h"

#include <vector>

/// IUnknown methods declaration macro
#define DECLARE_IUNKNOWN \
    STDMETHOD_(ULONG, AddRef)(void); \
    STDMETHOD_(ULONG, Release)(void); \
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);

typedef enum DownloadType {
    DownloadTypeReleaseJson,
    DownloadTypeLiveImage,
    DownloadTypeInstallerImage,
	DownloadTypeDualBootFiles,
    DownloadTypeMax
} DownloadType_t;

static LPCTSTR DownloadTypeToString(DownloadType_t type)
{
    switch (type)
    {
        TOSTR(DownloadTypeReleaseJson);
        TOSTR(DownloadTypeLiveImage);
        TOSTR(DownloadTypeInstallerImage);
		TOSTR(DownloadTypeDualBootFiles);
        default: return _T("UNKNOWN_DOWNLOAD_TYPE");
    }
}

typedef struct DownloadStatus {
    BG_JOB_PROGRESS progress;    
    bool done;
    bool transientError;
    bool error;
    CString jobName;
    BG_ERROR_CONTEXT errorContext;
    HRESULT errorCode;
} DownloadStatus_t;

typedef std::vector<CString> ListOfStrings;

static const DownloadStatus_t DownloadStatusNull = { {0, 0, 0, 0}, false, false, false, L"" };

class DownloadManager : public IBackgroundCopyCallback {
public:

    DownloadManager();
    ~DownloadManager();

    bool Init(HWND window, DWORD statusMessageId);
    bool Uninit();

    bool AddDownload(DownloadType_t type, const CString &urlBase, ListOfStrings urlPaths, ListOfStrings files, bool resumeExisting, LPCTSTR jobSuffix = NULL);

    static CString GetJobName(DownloadType_t type);

    static bool GetDownloadProgress(CComPtr<IBackgroundCopyJob> &currentJob, DownloadStatus_t &downloadStatus, const CString &jobName);
    static HRESULT GetExistingJob(CComPtr<IBackgroundCopyManager> &bcManager, LPCWSTR jobName, CComPtr<IBackgroundCopyJob> &existingJob);

    void ClearExtraDownloadJobs(bool forceCancel = false);

    // IUnknown implementation
    DECLARE_IUNKNOWN;

    // IBackgroundCopyCallback implementation
    STDMETHOD(JobTransferred)(IBackgroundCopyJob *pJob);
    STDMETHOD(JobError)(IBackgroundCopyJob *pJob, IBackgroundCopyError *pError);
    STDMETHOD(JobModification)(IBackgroundCopyJob *JobModification, DWORD dwReserved);

    void SetLatestEosVersion(CString latestEosVersion)
    {
        m_latestEosVersion = latestEosVersion;
    }

private:
    void ReportStatus(const DownloadStatus_t &downloadStatus);
    static void PopulateErrorDetails(DownloadStatus_t &downloadStatus, IBackgroundCopyError *pError);
    static void PopulateErrorDetails(DownloadStatus_t &downloadStatus, IBackgroundCopyJob *pJob);

    static volatile ULONG m_refCount;
    HWND m_dispatchWindow;
    DWORD m_statusMsgId;
    CComPtr<IBackgroundCopyManager> m_bcManager;
    CString m_latestEosVersion;
};