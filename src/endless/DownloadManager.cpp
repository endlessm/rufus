#include "stdafx.h"
#include "DownloadManager.h"
#include <algorithm>

#define DOWNLOAD_JOB_PREFIX _T("Endless")

#define MINIMUM_RETRY_DELAY_SEC_JSON    5
#define MINIMUM_RETRY_DELAY_SEC         20

static LPCTSTR JobStateToStr(BG_JOB_STATE state)
{
    switch (state)
    {
        TOSTR(BG_JOB_STATE_QUEUED);
        TOSTR(BG_JOB_STATE_CONNECTING);
        TOSTR(BG_JOB_STATE_TRANSFERRING);
        TOSTR(BG_JOB_STATE_SUSPENDED);
        TOSTR(BG_JOB_STATE_ERROR);
        TOSTR(BG_JOB_STATE_TRANSIENT_ERROR);
        TOSTR(BG_JOB_STATE_TRANSFERRED);
        TOSTR(BG_JOB_STATE_ACKNOWLEDGED);
        TOSTR(BG_JOB_STATE_CANCELLED);
    default: return _T("Unknown BG_JOB_STATE");
    }
}

volatile ULONG DownloadManager::m_refCount = 0;

DownloadManager::DownloadManager()
{
    FUNCTION_ENTER;

    m_bcManager = NULL;
    m_latestEosVersion = "";
}

DownloadManager::~DownloadManager()
{
    FUNCTION_ENTER;
}

bool DownloadManager::Init(HWND window, DWORD statusMessageId)
{
    FUNCTION_ENTER;

    CComPtr<IEnumBackgroundCopyJobs> enumJobs;
    CComPtr<IBackgroundCopyJob> job;
    ULONG jobCount = 0;
    HRESULT hr = S_OK;

    m_dispatchWindow = window;
    m_statusMsgId = statusMessageId;

    IFFALSE_GOTOERROR(m_bcManager == NULL, "Manager already initialized.");

    // Specify the appropriate COM threading model for your application.
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IFFAILED_GOTOERROR(hr, "Error on calling CoInitializeEx");

    hr = CoCreateInstance(__uuidof(BackgroundCopyManager), NULL, CLSCTX_LOCAL_SERVER, __uuidof(IBackgroundCopyManager), (void**)&m_bcManager);
    IFFAILED_GOTOERROR(hr, "Error creating instance of BackgroundCopyManager.");

    ClearExtraDownloadJobs();    

    return true;
error:
    uprintf("DownloadManager::Init failed with hr = 0x%x, last error = %s", hr, WindowsErrorString());
    return false;
}

bool DownloadManager::Uninit()
{
    IFFALSE_GOTOERROR(m_bcManager != NULL,  "Manager or job is NULL. Nothing to uninit.");
    
    m_dispatchWindow = NULL;
    ClearExtraDownloadJobs();
    m_bcManager = NULL;

    //RADU: what else ?
    return S_OK;

error:
    return false;
}

bool DownloadManager::AddDownload(DownloadType_t type, const CString &urlBase, ListOfStrings urlPaths, ListOfStrings files, bool resumeExisting, LPCTSTR jobSuffix)
{
    FUNCTION_ENTER;

    LPWSTR local = NULL, remote = NULL;
    CComPtr<IBackgroundCopyJob> currentJob;
    GUID jobId;
    HRESULT hr = S_OK;
    ULONG count = 0;
    CString jobName;
    bool jobExisted = false;
    CComPtr<IBackgroundCopyJobHttpOptions> httpOptions;
    ULONG securityFlags;
    
    IFFALSE_GOTOERROR(m_bcManager != NULL, "Manager is NULL. Was Init called?");
    IFFALSE_GOTOERROR(urlPaths.size() == files.size(), "Count of files and urlPaths differ.");

    jobName = DownloadManager::GetJobName(type);
    if (jobSuffix != NULL) jobName += jobSuffix;
    uprintf("DownloadManager::AddDownload job %ls", jobName);

    if(type != DownloadType_t::DownloadTypeReleaseJson && jobSuffix == NULL) {
        uprintf("Error: AddDownload NO SUFFIX ADDED");
    }
        
    IFFAILED_GOTO(GetExistingJob(m_bcManager, jobName, currentJob), "No previous download job found", usecurrentjob);
    IFFALSE_GOTO(resumeExisting, "Canceling exiting download on request.", canceljob);
    
    if (currentJob != NULL) {
        CComPtr<IEnumBackgroundCopyFiles> pFileList;
        ULONG cFileCount = 0, index = 0;

        IFFAILED_GOTO(currentJob->EnumFiles(&pFileList), "Error getting filelist.", canceljob);
        IFFALSE_GOTO(pFileList != NULL, "Error getting filelist.", canceljob);
        IFFAILED_GOTO(pFileList->GetCount(&cFileCount), "Error getting file count.", canceljob);
        IFFALSE_GOTO(cFileCount == urlPaths.size(), "Number of files doesn't match.", canceljob);

        //Enumerate the files in the job.
        for (index = 0; index < cFileCount; index++) {
            CComPtr<IBackgroundCopyFile> pFile;
            CComPtr<IBackgroundCopyFile2> pFile2;
            CComHeapPtr<WCHAR> pLocalFileName, pRemoteName;

            CString remoteName;

            IFFALSE_GOTO(S_OK == pFileList->Next(1, &pFile, NULL), "Error querying for file object.", canceljob);
            IFFAILED_GOTO(pFile->QueryInterface(&pFile2), "Error querying for IBackgroundCopyFile2 interface.", canceljob);
            IFFAILED_GOTO(pFile2->GetLocalName(&pLocalFileName), "Error querying for local file name.", canceljob);
            IFFAILED_GOTO(pFile2->GetRemoteName(&pRemoteName), "Error querying for remote file name.", canceljob);

            remoteName = pRemoteName;

            ListOfStrings::iterator foundUrlItem = std::find_if(urlPaths.begin(), urlPaths.end(),
                [&remoteName](const CString &url) { return remoteName.Right(url.GetLength()) == url; });
            IFFALSE_GOTO(foundUrlItem != urlPaths.end(), "URL not found in list.", canceljob);
            ListOfStrings::iterator file = files.begin() + (foundUrlItem - urlPaths.begin());
            IFFALSE_GOTO(0 == wcscmp(*file, pLocalFileName), "Local file doesn't match at url index.", canceljob);

            CString fullUrl = urlBase + *foundUrlItem;
            if (fullUrl != remoteName) {
                uprintf("Adjusting remote URL from %ls to %ls", remoteName, fullUrl);
                /* If a later file in the job doesn't match, the job will be deleted anyway, so we can just fix up each URL while we're checking. */
                IFFAILED_GOTO(pFile2->SetRemoteName(fullUrl), "Couldn't adjust remote URL", canceljob);
            }
        }

    }
    goto usecurrentjob;

canceljob:
    // cancel the job
    hr = currentJob->Cancel();
    currentJob = NULL;

usecurrentjob:
    if (currentJob == NULL) {
        hr = m_bcManager->CreateJob(jobName, BG_JOB_TYPE_DOWNLOAD, &jobId, &currentJob);
        IFFAILED_GOTOERROR(hr, "Error creating instance of IBackgroundCopyJob.");

        ULONG secondsRetry = MINIMUM_RETRY_DELAY_SEC;
        if (type == DownloadType_t::DownloadTypeReleaseJson) {
            secondsRetry = MINIMUM_RETRY_DELAY_SEC_JSON;
        }
        currentJob->SetPriority(BG_JOB_PRIORITY_FOREGROUND);
        hr = currentJob->SetMinimumRetryDelay(secondsRetry);
        IFFAILED_PRINTERROR(hr, "Error on SetMinimumRetryDelay");

        for (auto url = urlPaths.begin(), file = files.begin(); url != urlPaths.end(); url++, file++) {
            CString fullUrl = urlBase + *url;
            uprintf("Adding download [%ls]->[%ls]", fullUrl, *file);
            hr = currentJob->AddFile(fullUrl, *file);
            IFFAILED_GOTOERROR(hr, "Error adding file to download job");
        }
    }

    /* Set ALLOW_REPORT to update remote URLs when we follow a redirect */
    IFFAILED_GOTOERROR(currentJob->QueryInterface(&httpOptions), "Error getting HttpOptions interface");
    IFFAILED_GOTOERROR(httpOptions->GetSecurityFlags(&securityFlags), "Error calling GetSecurityFlags");
    securityFlags |= BG_HTTP_REDIRECT_POLICY_ALLOW_REPORT;
    IFFAILED_GOTOERROR(httpOptions->SetSecurityFlags(securityFlags), "Error calling SetSecurityFlags");

    hr = currentJob->SetNotifyInterface(this);
    IFFAILED_GOTOERROR(hr, "Error calling SetNotifyInterface.");
    
    ULONG flags = BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_MODIFICATION
        | BG_NOTIFY_FILE_TRANSFERRED;
    hr = currentJob->SetNotifyFlags(flags);
    IFFAILED_GOTOERROR(hr, "Error calling SetNotifyFlags.");

    hr = currentJob->Resume();
    IFFAILED_GOTOERROR(hr, "Error resuming download.");

    return true;

error:
    uprintf("DownloadManager::AddDownload failed with hr = 0x%x, last error = %s", hr, WindowsErrorString());
    return false;
}

STDMETHODIMP DownloadManager::JobTransferred(IBackgroundCopyJob *pJob)
{
    HRESULT hr;

    //Add logic that will not block the callback thread. If you need to perform
    //extensive logic at this time, consider creating a separate thread to perform
    //the work.

    hr = pJob->Complete();
    if (FAILED(hr))
    {
        //Handle error. BITS probably was unable to rename one or more of the 
        //temporary files. See the Remarks section of the IBackgroundCopyJob::Complete 
        //method for more details.
    }

    //If you do not return S_OK, BITS continues to call this callback.
    return S_OK;
}

void DownloadManager::ReportStatus(const DownloadStatus_t & downloadStatus)
{
    if (m_dispatchWindow != NULL)
        ::PostMessage(m_dispatchWindow, m_statusMsgId, (WPARAM)new DownloadStatus_t(downloadStatus), 0);
}

void DownloadManager::PopulateErrorDetails(DownloadStatus_t & downloadStatus, IBackgroundCopyError * pError)
{
    FUNCTION_ENTER;

    HRESULT hr;
    CComHeapPtr<WCHAR> errorDescription;

    hr = pError->GetError(&downloadStatus.errorContext, &downloadStatus.errorCode);
    IFFAILED_PRINTERROR(hr, "GetError failed");

    // This fails, apparently because the message catalogue isn't loaded
    hr = pError->GetErrorDescription(LANGIDFROMLCID(GetThreadLocale()), &errorDescription);
    IFFAILED_PRINTERROR(hr, "GetErrorDescription failed");

    uprintf("Job %ls %s error 0x%x in context %d: %ls", downloadStatus.jobName,
        downloadStatus.error ? "fatal" : downloadStatus.transientError ? "transient" : "???",
        downloadStatus.errorCode, downloadStatus.errorContext,
        errorDescription ? errorDescription : L"");

    // Special-case one transient error that we want to treat as permanent
    if (downloadStatus.transientError &&
        downloadStatus.errorContext == BG_ERROR_CONTEXT_LOCAL_FILE &&
        downloadStatus.errorCode == HRESULT_FROM_WIN32(ERROR_DISK_FULL)) {
        uprintf("Escalating DISK_FULL error to fatal");
        downloadStatus.error = true;
        downloadStatus.transientError = false;
    }
}

void DownloadManager::PopulateErrorDetails(DownloadStatus_t & downloadStatus, IBackgroundCopyJob * pJob)
{
    FUNCTION_ENTER;

    CComPtr<IBackgroundCopyError> pError;
    HRESULT hr = pJob->GetError(&pError);
    if (SUCCEEDED(hr)) {
        PopulateErrorDetails(downloadStatus, pError);
    } else {
        PRINT_HRESULT(hr, "IBackgroundCopyJob::GetError failed");
    }
}

STDMETHODIMP DownloadManager::JobError(IBackgroundCopyJob *pJob, IBackgroundCopyError *pError)
{
    FUNCTION_ENTER;

    HRESULT hr;
    DownloadStatus_t downloadStatus = DownloadStatusNull;
    CComHeapPtr<WCHAR> jobName;
    CComHeapPtr<WCHAR> errorDescription;

    downloadStatus.error = true;

    hr = pJob->GetProgress(&downloadStatus.progress);
    IFFAILED_PRINTERROR(hr, "GetProgress failed");

    hr = pJob->GetDisplayName(&jobName);
    IFFAILED_PRINTERROR(hr, "GetDisplayName failed");
    downloadStatus.jobName = jobName;

    PopulateErrorDetails(downloadStatus, pError);
    ReportStatus(downloadStatus);

    return S_OK;
}

STDMETHODIMP DownloadManager::JobModification(IBackgroundCopyJob *JobModification, DWORD dwReserved)
{
    FUNCTION_ENTER;

    HRESULT hr;
    CComHeapPtr<WCHAR> pszJobName;
    BG_JOB_STATE State;
    DownloadStatus_t downloadStatus = DownloadStatusNull;

    hr = JobModification->GetDisplayName(&pszJobName);
    IFFAILED_GOTOERROR(hr, "GetDisplayName failed");
    downloadStatus.jobName = pszJobName;

    hr = JobModification->GetProgress(&downloadStatus.progress);
    IFFAILED_GOTOERROR(hr, "GetProgress failed");

    hr = JobModification->GetState(&State);
    IFFAILED_GOTOERROR(hr, "GetState failed");

    uprintf("Job %ls %ls (0x%X)", pszJobName, JobStateToStr(State), State);

    switch (State) {
    case BG_JOB_STATE_ACKNOWLEDGED:
        downloadStatus.done = true;
        break;
    case BG_JOB_STATE_TRANSFERRED:
        //Call pJob->Complete(); to acknowledge that the transfer is complete
        //and make the file available to the client.
        uprintf("Job %ls DONE\n", pszJobName);
        hr = JobModification->Complete();
        if (FAILED(hr)) JobModification->Cancel();
        downloadStatus.done = true;
        break;
    case BG_JOB_STATE_TRANSIENT_ERROR:
    {
        downloadStatus.transientError = true;

        CComPtr<IBackgroundCopyError> pError;
        hr = JobModification->GetError(&pError);
        if (SUCCEEDED(hr)) {
            PopulateErrorDetails(downloadStatus, pError);
        } else {
            PRINT_HRESULT(hr, "IBackgroundCopyJob::GetError failed");
        }
        break;
    }
    case BG_JOB_STATE_CANCELLED:
        downloadStatus.error = true;
        break;
    case BG_JOB_STATE_ERROR:
        // handled by the JobError callback
        return S_OK;
    default:
        return S_OK;
    }

    ReportStatus(downloadStatus);

error:
    return S_OK;
}

// IUnknown methods implementation
ULONG DownloadManager::AddRef(void) { return InterlockedIncrement(&m_refCount); }
ULONG DownloadManager::Release(void) { return InterlockedDecrement(&m_refCount); }
HRESULT DownloadManager::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (__uuidof(IBackgroundCopyCallback) == riid) {
        *ppvObj = (IBackgroundCopyCallback*)(this);        
    } else if (__uuidof(IUnknown) == riid) {
        *ppvObj = (IUnknown*)(this);
    }

    if (*ppvObj != NULL) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
} 

// Helper methods

HRESULT DownloadManager::GetExistingJob(CComPtr<IBackgroundCopyManager> &bcManager, LPCWSTR jobName, CComPtr<IBackgroundCopyJob> &existingJob)
{
    FUNCTION_ENTER;

    CComPtr<IEnumBackgroundCopyJobs> enumJobs;
    CComPtr<IBackgroundCopyJob> job;
    HRESULT hr;
    ULONG index = 0;
    ULONG jobCount = 0;
    LPWSTR displayName = NULL;
    BG_JOB_STATE state;

    IFFALSE_GOTOERROR(bcManager != NULL, "Manager or job is NULL. Nothing to uninit.");

    hr = bcManager->EnumJobs(0, &enumJobs);
    IFFAILED_GOTOERROR(hr, "Error calling EnumJobs.");
    
    hr = enumJobs->GetCount(&jobCount);
    IFFAILED_GOTOERROR(hr, "Error calling IEnumBackgroundCopyJobs::GetCount.");

    uprintf("Found a total of %d download jobs.", jobCount);

    for (index = 0; index < jobCount; index++) {
        hr = enumJobs->Next(1, &job, NULL);
        IFFAILED_GOTOERROR(hr, "Error querying for next job.");
        IFFALSE_GOTOERROR(job != NULL, "Error querying for next job.");
        
        hr = job->GetDisplayName(&displayName);
        IFFAILED_GOTOERROR(hr, "Error querying for display name.");
        uprintf("Found job %d name %ls", index, displayName);
        if (0 == _tcscmp(jobName, displayName)) {            
            hr = job->GetState(&state);
            IFFAILED_GOTOERROR(hr, "Error querying for job state.");
            if (state != BG_JOB_STATE_CANCELLED && state != BG_JOB_STATE_ERROR) {
                existingJob = job;
                return S_OK;
            } else {
                uprintf("Cancelling job %ls (in state %ls)", displayName, JobStateToStr(state));
                job->Cancel();
            }
        }

        CoTaskMemFree(displayName);
        job = NULL;
    }

error:
    return E_FAIL;
}

void DownloadManager::ClearExtraDownloadJobs(bool forceCancel)
{
    FUNCTION_ENTER;

    CComPtr<IEnumBackgroundCopyJobs> enumJobs;
    CComPtr<IBackgroundCopyJob> job;
    HRESULT hr;
    ULONG index = 0;
    ULONG jobCount = 0;
    LPWSTR displayName = NULL;
    CList<CString> foundJobs;

    IFFALSE_GOTOERROR(m_bcManager != NULL, "Manager or job is NULL. Nothing to uninit.");

    hr = m_bcManager->EnumJobs(0, &enumJobs);
    IFFAILED_GOTOERROR(hr, "Error calling EnumJobs.");

    hr = enumJobs->GetCount(&jobCount);
    IFFAILED_GOTOERROR(hr, "Error calling IEnumBackgroundCopyJobs::GetCount.");

    uprintf("Found a total of %d download jobs.", jobCount);

    for (index = 0; index < jobCount; index++) {
        hr = enumJobs->Next(1, &job, NULL);
        IFFAILED_GOTOERROR(hr, "Error querying for next job.");
        IFFALSE_GOTOERROR(job != NULL, "Next job is NULL.");

        hr = job->GetDisplayName(&displayName);
        IFFAILED_CONTINUE(hr, "Error querying for display name.");
        if (displayName == _tcsstr(displayName, DOWNLOAD_JOB_PREFIX)) {
            bool cancelJob = forceCancel || (m_latestEosVersion != _T("") && NULL == _tcsstr(displayName, m_latestEosVersion));
            if (!cancelJob && NULL == foundJobs.Find(displayName)) {
                foundJobs.AddTail(displayName);
                job->Suspend();
                uprintf("Suspending job %d name %ls", index, displayName);
            } else {
                job->Cancel();
                uprintf("Cancelling job %d name %ls", index, displayName);
            }
        }
        else {
            uprintf("Found job %d name %ls", index, displayName);
        }

        CoTaskMemFree(displayName);
        job = NULL;
    }

error:
    return;
}

CString DownloadManager::GetJobName(DownloadType_t type)
{
    return CString(DOWNLOAD_JOB_PREFIX) + DownloadTypeToString(type);
}

bool DownloadManager::GetDownloadProgress(CComPtr<IBackgroundCopyJob> &currentJob, DownloadStatus_t &downloadStatus, const CString &jobName)
{
    HRESULT hr;
    BG_JOB_STATE State;
    bool result = false;

    downloadStatus = DownloadStatusNull;
    downloadStatus.jobName = jobName;

    hr = currentJob->GetState(&State);
    IFFAILED_GOTOERROR(hr, "Error querying for job state");

    switch (State) {
    case BG_JOB_STATE_TRANSFERRED:
    case BG_JOB_STATE_ACKNOWLEDGED:
    case BG_JOB_STATE_SUSPENDED:
        downloadStatus.done = true;
        break;
    case BG_JOB_STATE_TRANSIENT_ERROR:
        downloadStatus.transientError = true;
        PopulateErrorDetails(downloadStatus, currentJob);
        break;
    case BG_JOB_STATE_ERROR:
    case BG_JOB_STATE_CANCELLED:
        downloadStatus.error = true;
        PopulateErrorDetails(downloadStatus, currentJob);
        break;
    }

    hr = currentJob->GetProgress(&downloadStatus.progress);
    IFFAILED_GOTOERROR(hr, "Error querying for job progress");

    result = true;
error:
    return result;
}
