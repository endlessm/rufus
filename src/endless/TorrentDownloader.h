#pragma once

#define INTMAX_MAX   INT64_MAX
#undef malloc
#undef free
#include <libtorrent/session.hpp>

namespace lt = libtorrent;

class TorrentDownloader {
public:
	TorrentDownloader();
	~TorrentDownloader();

	bool Init(HWND window, DWORD statusMessageId);
	bool AddDownload(DownloadType_t type, ListOfStrings urls, ListOfStrings files, const CString& jobSuffix);
	void StopNotificationThread(bool canceled);
	bool GetDownloadProgress(DownloadStatus_t *downloadStatus);
	bool IsSeedingRatioMet();
	void Reset();

private:
	HANDLE			m_notificationThread;
	HANDLE			m_stopThreadEvent;
	lt::session		m_torrentSession;
	ListOfStrings	m_filesToDownload;
	DownloadType_t	m_downloadType;
	CStringW		m_jobDownloadName;
	std::mutex		m_torrentSync;
	CStringA		m_downloadPath;
	HWND			m_dispatchWindow;
	DWORD			m_statusMsgId;
	bool			m_downloadCanceled;

	static DWORD WINAPI NotificationThreadHandler(void* param);
};