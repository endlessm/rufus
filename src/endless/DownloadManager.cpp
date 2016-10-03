#include "stdafx.h"
#include "DownloadManager.h"
#include <algorithm>

extern "C" {
    #include "rufus.h"
}

#define DOWNLOAD_JOB_PREFIX _T("Endless")

#define MINIMUM_RETRY_DELAY_SEC_JSON    5
#define MINIMUM_RETRY_DELAY_SEC         20

#define NO_PROGRESS_TIMEOUT_SEC_JSON    30
#define NO_PROGRESS_TIMEOUT_SEC         600

volatile ULONG DownloadManager::m_refCount = 0;

DownloadManager::DownloadManager()
{
    FUNCTION_ENTER;

    m_bcManager = NULL;
    m_PendingJobModificationCount = 0;
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
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error on calling CoInitializeEx");

    hr = CoCreateInstance(__uuidof(BackgroundCopyManager), NULL, CLSCTX_LOCAL_SERVER, __uuidof(IBackgroundCopyManager), (void**)&m_bcManager);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error creating instance of BackgroundCopyManager.");

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

bool DownloadManager::AddDownload(DownloadType_t type, ListOfStrings urls, ListOfStrings files, bool resumeExisting, LPCTSTR jobSuffix)
{
    FUNCTION_ENTER;

    LPWSTR local = NULL, remote = NULL;
    CComPtr<IBackgroundCopyJob> currentJob;
    GUID jobId;
    HRESULT hr = S_OK;
    ULONG count = 0;
    CString jobName;
    bool jobExisted = false;
    WCHAR* pLocalFileName = NULL, *pRemoteName = NULL;
    
    IFFALSE_GOTOERROR(m_bcManager != NULL, "Manager is NULL. Was Init called?");
    IFFALSE_GOTOERROR(urls.size() == files.size(), "Count of files and urls differ.");

    jobName = DownloadManager::GetJobName(type);
    if (jobSuffix != NULL) jobName += jobSuffix;
    uprintf("DownloadManager::AddDownload job %ls", jobName);

    if(type != DownloadType_t::DownloadTypeReleseJson && jobSuffix == NULL) {
        uprintf("Error: AddDownload NO SUFFIX ADDED");
    }
        
    IFFALSE_GOTO(SUCCEEDED(GetExistingJob(m_bcManager, jobName, currentJob)), "No previous download job found", usecurrentjob);
    IFFALSE_GOTO(resumeExisting, "Canceling exiting download on request.", canceljob);
    
    if (currentJob != NULL) {
        CComPtr<IEnumBackgroundCopyFiles> pFileList;
        CComPtr<IBackgroundCopyFile> pFile;
        ULONG cFileCount = 0, index = 0;

        IFFALSE_GOTO(SUCCEEDED(currentJob->EnumFiles(&pFileList)) && pFileList != NULL, "Error getting filelist.", canceljob);
        IFFALSE_GOTO(SUCCEEDED(pFileList->GetCount(&cFileCount)) && cFileCount == urls.size(), "Number of files doesn't match.", canceljob);

        //Enumerate the files in the job.
        for (index = 0; index < cFileCount; index++) {
            IFFALSE_GOTO(S_OK == pFileList->Next(1, &pFile, NULL), "Error querying for file object.", canceljob);
            IFFALSE_GOTO(SUCCEEDED(pFile->GetLocalName(&pLocalFileName)), "Error querying for local file name.", canceljob);
            IFFALSE_GOTO(SUCCEEDED(pFile->GetRemoteName(&pRemoteName)), "Error querying for remote file name.", canceljob);
            
            ListOfStrings::iterator foundUrlItem = std::find_if(urls.begin(), urls.end(), [&pRemoteName](LPCTSTR url) { return 0 == wcscmp(url, pRemoteName); });
            IFFALSE_GOTO(foundUrlItem != urls.end(), "URL no found in list.", canceljob);
            ListOfStrings::iterator file = files.begin() + (foundUrlItem - urls.begin());
            IFFALSE_GOTO(0 == wcscmp(*file, pLocalFileName), "Local file doesn't match at url index.", canceljob);
            
            CoTaskMemFree(pLocalFileName); pLocalFileName = NULL;
            CoTaskMemFree(pRemoteName); pRemoteName = NULL;
            pFile = NULL;
        }

    }
    goto usecurrentjob;

canceljob:
    // extra cleaning in case the for loop exited because of error
    // No need to check for NULL as CoTaskMemFree handles it
    CoTaskMemFree(pLocalFileName);
    CoTaskMemFree(pRemoteName);
    // cancel the job
    hr = currentJob->Cancel();
    currentJob = NULL;

usecurrentjob:
    if (currentJob == NULL) {
        hr = m_bcManager->CreateJob(jobName, BG_JOB_TYPE_DOWNLOAD, &jobId, &currentJob);
        IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error creating instance of IBackgroundCopyJob.");

        ULONG secondsRetry = MINIMUM_RETRY_DELAY_SEC;
        ULONG secondsTimeout = MINIMUM_RETRY_DELAY_SEC;
        if (type == DownloadType_t::DownloadTypeReleseJson) {
            secondsRetry = MINIMUM_RETRY_DELAY_SEC_JSON;
            secondsTimeout = MINIMUM_RETRY_DELAY_SEC_JSON;
        }
        currentJob->SetPriority(BG_JOB_PRIORITY_FOREGROUND);
        hr = currentJob->SetMinimumRetryDelay(secondsRetry);
        IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error on SetMinimumRetryDelay");
        hr = currentJob->SetNoProgressTimeout(secondsTimeout);
        IFFALSE_PRINTERROR(SUCCEEDED(hr), "Error on SetNoProgressTimeout");

        for (auto url = urls.begin(), file = files.begin(); url != urls.end(); url++, file++) {
            uprintf("Adding download [%ls]->[%ls]", *url, *file);
            hr = currentJob->AddFile(*url, *file);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error adding file to download job");
        }
    }

    hr = currentJob->SetNotifyInterface(this);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error calling SetNotifyInterface.");
    
    ULONG flags = BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_MODIFICATION;
    if (nWindowsVersion > WINDOWS_XP) { // BG_NOTIFY_FILE_TRANSFERRED makes the call fail on XP
        flags = flags | BG_NOTIFY_FILE_TRANSFERRED;
    }
    hr = currentJob->SetNotifyFlags(flags);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error calling SetNotifyFlags.");

    hr = currentJob->Resume();
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error resuming download.");

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

#define TWO_GB 2147483648    // 2GB

STDMETHODIMP DownloadManager::JobError(IBackgroundCopyJob *pJob, IBackgroundCopyError *pError)
{
    FUNCTION_ENTER;

    HRESULT hr;
    BG_FILE_PROGRESS Progress;
    BG_ERROR_CONTEXT Context;
    IBackgroundCopyFile *pFile;
    HRESULT ErrorCode = S_OK;
    WCHAR* pszJobName = NULL;
    WCHAR* pszErrorDescription = NULL;
    BOOL IsError = TRUE;

    //Use pJob and pError to retrieve information of interest. For example,
    //if the job is an upload reply, call the IBackgroundCopyError::GetError method 
    //to determine the context in which the job failed. If the context is 
    //BG_JOB_CONTEXT_REMOTE_APPLICATION, the server application that received the 
    //upload file failed.

    hr = pError->GetError(&Context, &ErrorCode);

    //If the proxy or server does not support the Content-Range header or if
    //antivirus software removes the range requests, BITS returns BG_E_INSUFFICIENT_RANGE_SUPPORT.
    //This implementation tries to switch the job to foreground priority, so
    //the content has a better chance of being successfully downloaded.
    if (BG_E_INSUFFICIENT_RANGE_SUPPORT == ErrorCode)
    {
        hr = pError->GetFile(&pFile);
        hr = pFile->GetProgress(&Progress);
        if (BG_SIZE_UNKNOWN == Progress.BytesTotal)
        {
            //The content is dynamic, do not change priority. Handle as an error.
        }
        else if (Progress.BytesTotal > TWO_GB)
        {
            //BITS does not use range requests if the content is less than 2 GB. 
            //However, if the content is greater than 2 GB, BITS
            //uses 2 GB ranges to download the file, so switching to foreground 
            //priority will not help.
        }
        else
        {
            hr = pJob->SetPriority(BG_JOB_PRIORITY_FOREGROUND);
            hr = pJob->Resume();
            IsError = FALSE;
        }

        pFile->Release();
    }

    if (TRUE == IsError)
    {
        hr = pJob->GetDisplayName(&pszJobName);
        hr = pError->GetErrorDescription(LANGIDFROMLCID(GetThreadLocale()), &pszErrorDescription);

        if (pszJobName && pszErrorDescription)
        {
            //Do something with the job name and description. 
            uprintf("Error on download %ls: [%ls]", pszJobName, pszErrorDescription);
        }

        CoTaskMemFree(pszJobName);
        CoTaskMemFree(pszErrorDescription);
    }

    //If you do not return S_OK, BITS continues to call this callback.
    return S_OK;
}

STDMETHODIMP DownloadManager::JobModification(IBackgroundCopyJob *JobModification, DWORD dwReserved)
{
    HRESULT hr;
    WCHAR* pszJobName = NULL;
    BG_JOB_PROGRESS Progress;
    BG_JOB_STATE State;

    hr = JobModification->GetDisplayName(&pszJobName);
    if (SUCCEEDED(hr))
    {
        hr = JobModification->GetProgress(&Progress);
        if (SUCCEEDED(hr))
        {
            hr = JobModification->GetState(&State);
            if (SUCCEEDED(hr))
            {
                DownloadStatus_t *downloadStatus = NULL;
                switch (State) {
                case BG_JOB_STATE_ACKNOWLEDGED:
                case BG_JOB_STATE_TRANSFERRED:
                case BG_JOB_STATE_TRANSFERRING:
                case BG_JOB_STATE_ERROR:
                case BG_JOB_STATE_TRANSIENT_ERROR:
                case BG_JOB_STATE_CANCELLED:
                    downloadStatus = new DownloadStatus_t;
                    *downloadStatus = DownloadStatusNull;
                    if (SUCCEEDED(hr)) downloadStatus->jobName = pszJobName;
                    hr = JobModification->GetProgress(&downloadStatus->progress);
                    break;
                }

                //Do something with the progress and state information.
                //BITS generates a high volume of modification
                //callbacks. Use this callback with discretion. Consider creating a timer and 
                //polling for state and progress information.
                if (BG_JOB_STATE_TRANSFERRED == State) {
                    //Call pJob->Complete(); to acknowledge that the transfer is complete
                    //and make the file available to the client.                    
                    uprintf("Job %ls DONE\n", pszJobName);
                    hr = JobModification->Complete();
                    if (FAILED(hr)) JobModification->Cancel();
                    downloadStatus->done = true;
                }
                else if (BG_JOB_STATE_ERROR == State || BG_JOB_STATE_TRANSIENT_ERROR == State) {
                    //Call pJob->GetError(&pError); to retrieve an IBackgroundCopyError interface 
                    //pointer which you use to determine the cause of the error.
                    CComPtr<IBackgroundCopyError> error;
                    JobModification->GetError(&error);
                    LPWSTR str[1024];
                    memset(str, 0, 1024);
                    error->GetErrorDescription(GetUserDefaultLangID(), str);
                    error->GetError(&downloadStatus->errorContext, &downloadStatus->errorCode);
                    uprintf("Job %ls ERROR!%ls\n", pszJobName, str);
                    downloadStatus->error = true;
                }
                else if (BG_JOB_STATE_TRANSFERRING == State) {
                    //Call pJob->GetProgress(&Progress); to determine the number of bytes 
                    //and files transferred.                                        
                    downloadStatus->done = false;
                    
                }
                else if (BG_JOB_STATE_QUEUED == State) {
                    uprintf("Job %ls QUEUED\n", pszJobName);
                }
                else if (BG_JOB_STATE_CONNECTING == State) {
                    uprintf("%ls CONNECTING\n", pszJobName);
                }
                else if (BG_JOB_STATE_SUSPENDED == State) {
                    uprintf("Job %ls SUSPENDED\n", pszJobName);
                    //m_bcJob->Cancel();
                }
                else if (BG_JOB_STATE_ACKNOWLEDGED == State) {                    
                    uprintf("Job %ls ACKNOWLEDGED\n", pszJobName);
                    downloadStatus->done = true;
                }
                else if (BG_JOB_STATE_CANCELLED == State) {
                    uprintf("Job %ls CANCELLED\n", pszJobName);
                    downloadStatus->error = true;
                } else {
                    uprintf("Job %ls Unknown download state %d\n", pszJobName, State);
                }

                if(downloadStatus != NULL && m_dispatchWindow != NULL) ::PostMessage(m_dispatchWindow, m_statusMsgId, (WPARAM)downloadStatus, 0);
            }
        }
        CoTaskMemFree(pszJobName);
    }

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
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error calling EnumJobs.");
    
    hr = enumJobs->GetCount(&jobCount);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error calling IEnumBackgroundCopyJobs::GetCount.");

    uprintf("Found a total of %d download jobs.", jobCount);

    for (index = 0; index < jobCount; index++) {
        hr = enumJobs->Next(1, &job, NULL);
        IFFALSE_GOTOERROR(SUCCEEDED(hr) && job != NULL, "Error querying for next job.");
        
        hr = job->GetDisplayName(&displayName);
        IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for display name.");
        uprintf("Found job %d name %ls", index, displayName);
        if (0 == _tcscmp(jobName, displayName)) {            
            hr = job->GetState(&state);
            IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for job state.");
            if (state != BG_JOB_STATE_CANCELLED && state != BG_JOB_STATE_ERROR) {
                existingJob = job;
                return S_OK;
            } else {
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
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error calling EnumJobs.");

    hr = enumJobs->GetCount(&jobCount);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error calling IEnumBackgroundCopyJobs::GetCount.");

    uprintf("Found a total of %d download jobs.", jobCount);

    for (index = 0; index < jobCount; index++) {
        hr = enumJobs->Next(1, &job, NULL);
        IFFALSE_GOTOERROR(SUCCEEDED(hr) && job != NULL, "Error querying for next job.");

        hr = job->GetDisplayName(&displayName);
        IFFALSE_CONTINUE(SUCCEEDED(hr), "Error querying for display name.");
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

bool DownloadManager::GetDownloadProgress(CComPtr<IBackgroundCopyJob> &currentJob, DownloadStatus_t *downloadStatus, const CString &jobName)
{
    HRESULT hr;
    BG_JOB_STATE State;
    bool result = false;

    hr = currentJob->GetState(&State);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for job state");

    switch (State) {
    case BG_JOB_STATE_TRANSFERRED:
    case BG_JOB_STATE_ACKNOWLEDGED:
    case BG_JOB_STATE_SUSPENDED:
        downloadStatus->done = true;
        break;
    case BG_JOB_STATE_ERROR:
    case BG_JOB_STATE_CANCELLED:
    case BG_JOB_STATE_TRANSIENT_ERROR:
        downloadStatus->error = true;
        downloadStatus->errorContext = BG_ERROR_CONTEXT_NONE;
        break;
    }
    downloadStatus->jobName = jobName;

    hr = currentJob->GetProgress(&downloadStatus->progress);
    IFFALSE_GOTOERROR(SUCCEEDED(hr), "Error querying for job progress");

    result = true;
error:
    return result;
}