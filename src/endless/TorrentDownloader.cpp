#include "stdafx.h"

extern "C" {
#include "rufus.h"
}

#include "DownloadManager.h"
#include "TorrentDownloader.h"
#include "StringHelperMethods.h"

///////////////////////////////////////////
//#define BOOST_CB_DISABLE_DEBUG
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_info.hpp>

#include "boost/asio/impl/src.hpp"
#include "boost/asio/ssl/impl/src.hpp"


void DownloadTorrent(const CStringA appPath)
{
    lt::session ses;

    lt::add_torrent_params atp;
    atp.url = "https://torrents.endlessm.com/torrents/eos-amd64-amd64-base-3.0.4-full.torrent";
	//atp.url = "https://torrents.endlessm.com/torrents/eosinstaller-amd64-amd64-base-3.0.4-full.torrent";
    atp.save_path = appPath; // save in current dir

    lt::error_code error_code;
    lt::torrent_handle h = ses.add_torrent(atp, error_code);

	uprintf("File path %s", h.save_path().c_str());

    for (;;) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        for (lt::alert const* a : alerts) {
            //std::cout << a->message() << std::endl;
            uprintf("type=[%d] {%s}", a->type(), a->message().c_str());

			if (lt::alert_cast<lt::metadata_received_alert>(a)) {
				boost::shared_ptr<const lt::torrent_info> torrentInfo = h.torrent_file();
				int numberOfFiles = torrentInfo->num_files();
				uprintf("Number of file in torrent %d", numberOfFiles);
				const lt::file_storage &files = torrentInfo->files();

				for (int index = 0; index < numberOfFiles; index++) {
					uprintf("File at index %d: '%s'", index, files.file_name(index).c_str());
					std::string newName = appPath + "\\" + files.file_name(index).c_str();
					h.rename_file(index, newName);
				}

				//// RADU: add more validation on number of files, file names?
				std::vector<int> file_priorities = h.file_priorities();
				//// RADU: for now we use predefined indexes in the list of files, maybe detect files based on filename?
				file_priorities[0] = 0;		// don't download Windows binary

				////file_priorities[1] = 255;	// download full image
				////file_priorities[2] = 255;	// download full image asc file
				file_priorities[1] = 1;		// don't download full image
				file_priorities[2] = 1;		// don't download full image asc file

				file_priorities[3] = 0;	// don't download eos-image-keyring.gpg
				file_priorities[4] = 1;	// download installer image
				file_priorities[5] = 0;	// download installer image asc file

				h.prioritize_files(file_priorities);
			}

			// if we receive the finished alert or an error, we're done
            if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
                goto done;
            }
            if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                goto done;
            }

			if (lt::alert_cast<lt::file_renamed_alert>(a)) {
				continue;
			}
			if (lt::alert_cast<lt::file_rename_failed_alert>(a)) {
				goto done;
			}
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
done:

	//h.rename_file
    std::cout << "done, shutting down" << std::endl;
}


///////////////////////////////////////////

TorrentDownloader::TorrentDownloader() :
	m_stopDownloadEvent(INVALID_HANDLE_VALUE),
	m_notificationThread(INVALID_HANDLE_VALUE),
	m_downloadCanceled(false)
{

}

TorrentDownloader::~TorrentDownloader()
{
	Reset();
}

bool TorrentDownloader::Init(HWND window, DWORD statusMessageId)
{
	FUNCTION_ENTER;

	m_dispatchWindow = window;
	m_statusMsgId = statusMessageId;
	m_downloadCanceled = false;

	return true;
}

void TorrentDownloader::Reset()
{
	std::unique_lock<std::mutex> lock(m_torrentSync);

	safe_closehandle(m_stopDownloadEvent);
	m_notificationThread = INVALID_HANDLE_VALUE;
	m_downloadCanceled = false;

	std::vector<lt::torrent_handle> &torrents = m_torrentSession.get_torrents();
	for (auto torrent = torrents.begin(); torrent != torrents.end(); torrent++) {
		m_torrentSession.remove_torrent(*torrent);
	}
}

bool TorrentDownloader::AddDownload(DownloadType_t type, ListOfStrings urls, ListOfStrings files, const CString& jobSuffix)
{
	lt::add_torrent_params atp;
	std::unique_lock<std::mutex> lock(m_torrentSync);

	FUNCTION_ENTER;

	IFFALSE_GOTOERROR(m_notificationThread == INVALID_HANDLE_VALUE, "Torrent notification thread already started");
	IFFALSE_GOTOERROR(files.size() != 0 && urls.size() != 0, "No elements found in urls or files");
	m_downloadPath = ConvertUnicodeToUTF8(CSTRING_GET_PATH(files[0], '\\'));

	for (auto url = urls.begin(); url != urls.end(); url++) {
		uprintf("Adding torrent [%ls]", *url);
		atp.url = ConvertUnicodeToUTF8(*url);
		atp.save_path = m_downloadPath; // save in current dir
		m_torrentSession.async_add_torrent(atp);
	}

	m_filesToDownload = files;
	m_downloadType = type;
	m_jobDownloadName = DownloadManager::GetJobName(type) + jobSuffix;
	uprintf("TorrentDownloader::AddDownload job %ls", m_jobDownloadName);

	m_stopDownloadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_notificationThread = CreateThread(NULL, 0, NotificationThreadHandler, (LPVOID)this, 0, NULL);
	IFFALSE_GOTOERROR(m_notificationThread != NULL, "CreateThread returned NULL");

	return true;
error:

	safe_closehandle(m_stopDownloadEvent);
	m_notificationThread = INVALID_HANDLE_VALUE;
	return false;
}

void TorrentDownloader::StopDownload(bool canceled)
{
	FUNCTION_ENTER;
	std::unique_lock<std::mutex> lock(m_torrentSync);
	if (m_stopDownloadEvent != INVALID_HANDLE_VALUE && m_notificationThread ) {
		m_downloadCanceled = canceled;
		SetEvent(m_stopDownloadEvent);
	} else {
		// TODO: post download finished event with error
	}
}

DWORD WINAPI TorrentDownloader::NotificationThreadHandler(void* param)
{
	TorrentDownloader *downloader = (TorrentDownloader*)param;
	bool torrentFinished = false;

	for (;;) {
		std::vector<lt::alert*> alerts;

		// Check if user cancelled
		{
			std::unique_lock<std::mutex> lock(downloader->m_torrentSync);
			IFFALSE_GOTO(downloader->m_stopDownloadEvent != INVALID_HANDLE_VALUE && WaitForSingleObject(downloader->m_stopDownloadEvent, 0) == WAIT_TIMEOUT, "User cancel.", cancel);
		}

		downloader->m_torrentSession.pop_alerts(&alerts);
		for (lt::alert const* alert : alerts) {
			bool printToLog = false;
			//uprintf("type=[%d] {%s}", alert->type(), alert->message().c_str());

			switch (alert->type()) {
			case lt::add_torrent_alert::alert_type:
			{
				lt::add_torrent_alert* p = (lt::add_torrent_alert*)alert;
				if (p->error.value() == boost::system::errc::success) {
					uprintf("Success on adding '%s'", p->params.url.c_str());
				}
				else {
					uprintf("Error '%s'(%d) on adding '%s'", p->error.message().c_str(), p->error.value(), p->params.url.c_str());
					goto done;
				}

				break;
			}

			case lt::metadata_received_alert::alert_type:
			{
				lt::metadata_received_alert* p = (lt::metadata_received_alert*)alert;
				boost::shared_ptr<const lt::torrent_info> torrentInfo = p->handle.torrent_file();
				int numberOfFiles = torrentInfo->num_files();
				uprintf("Number of files in '%s' is %d", torrentInfo->name().c_str(), numberOfFiles);
				const lt::file_storage &files = torrentInfo->files();
				std::vector<int> file_priorities = p->handle.file_priorities();

				for (int index = 0; index < numberOfFiles; index++) {
					std::string newName = downloader->m_downloadPath + "\\" + files.file_name(index).c_str();

					ListOfStrings::iterator foundItem = std::find_if(
						downloader->m_filesToDownload.begin(), downloader->m_filesToDownload.end(),
						[&newName](const CString &name) { return ConvertUnicodeToUTF8(name) == newName.c_str(); }
					);
					if (foundItem != downloader->m_filesToDownload.end()) {
						uprintf("Downloading file (size=%I64i) at index %d: '%s'", files.file_size(index), index, files.file_name(index).c_str());
						file_priorities[index] = 1;
					}
					else {
						uprintf("Skipping file at index %d: '%s'", index, files.file_name(index).c_str());
						file_priorities[index] = 0;
					}
				}

				p->handle.prioritize_files(file_priorities);
				break;
			}

			case lt::torrent_finished_alert::alert_type:
			{
				printToLog = true;

				// move files to exe folder
				lt::torrent_finished_alert* p = (lt::torrent_finished_alert*)alert;
				boost::shared_ptr<const lt::torrent_info> torrentInfo = p->handle.torrent_file();
				int numberOfFiles = torrentInfo->num_files();
				uprintf("Number of files in '%s' is %d", torrentInfo->name().c_str(), numberOfFiles);
				const lt::file_storage &files = torrentInfo->files();
				std::vector<int> file_priorities = p->handle.file_priorities();

				for (int index = 0; index < numberOfFiles; index++) {
					if (file_priorities[index]) {
						std::string newName = downloader->m_downloadPath + "\\" + files.file_name(index).c_str();
						if (PathFileExistsA(newName.c_str())) {
							uprintf("Deleting file first as it already existed '%s'", newName.c_str());
							BOOL result = DeleteFileA(newName.c_str());
							uprintf("Result on deleting file GLE=%d", GetLastError());
							if (result != ERROR_SUCCESS) continue;
						}
						p->handle.rename_file(index, newName);
					}
				}
				// check if all torrents finished download
				bool allFinished = true;
				std::vector<lt::torrent_handle> &torrents = downloader->m_torrentSession.get_torrents();
				for (auto torrent = torrents.begin(); torrent != torrents.end(); torrent++) {
					if (torrent->status().state != lt::torrent_status::state_t::finished) {
						allFinished = false;
						break;
					}
				}
				if (allFinished) {
					uprintf("Download of required files from all torrents done.");
					torrentFinished = true;
					goto done;
				}
				break;
			}
			case lt::torrent_error_alert::alert_type:
				uprintf("type=[%d] {%s}", alert->type(), alert->message().c_str());
				goto done;
				break;
			case lt::file_renamed_alert::alert_type:
				printToLog = true;
				break;
			case lt::file_rename_failed_alert::alert_type:
				uprintf("type=[%d] {%s}", alert->type(), alert->message().c_str());
				goto done;
				break;

			default:
				printToLog = true;
				break;
			}

			if(printToLog) uprintf("type=[%d] {%s}", alert->type(), alert->message().c_str());
		}
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

cancel:
	uprintf("Torrent download canceled.");
	downloader->m_torrentSession.abort();
	if (downloader->m_downloadCanceled) {
		std::vector<lt::torrent_handle> &torrents = downloader->m_torrentSession.get_torrents();
		for (auto torrent = torrents.begin(); torrent != torrents.end(); torrent++) {
			downloader->m_torrentSession.remove_torrent(*torrent, lt::session::delete_files);
		}
	}

	return 0;

done:
	uprintf("Done downloading torrens.");

	DownloadStatus_t *downloadStatus = new DownloadStatus_t;
	*downloadStatus = DownloadStatusNull;
	downloadStatus->done = torrentFinished;
	downloadStatus->error = !torrentFinished;
	downloadStatus->jobName = downloader->m_jobDownloadName;
	// TODO: find a better way to make sure the torrent library has released the file handle.
	// maybe downloader->m_torrentSession.pause();
	Sleep(3000);

	if(downloader->m_dispatchWindow) ::PostMessage(downloader->m_dispatchWindow, downloader->m_statusMsgId, (WPARAM)downloadStatus, 0);

	return 0;
}

bool TorrentDownloader::GetDownloadProgress(DownloadStatus_t *downloadStatus)
{
	std::vector<lt::torrent_handle> &torrents = m_torrentSession.get_torrents();
	boost::int64_t downloaded = 0, total = 0;
	bool stillDownloading = false;

	downloaded = total = 0;
	for (auto torrent = torrents.begin(); torrent != torrents.end(); torrent++) {
		lt::torrent_status status = torrent->status();
		downloaded += status.total_wanted_done;
		total += status.total_wanted;
		if (status.state != lt::torrent_status::state_t::finished) {
			stillDownloading = true;
		}
		if (status.state == lt::torrent_status::state_t::downloading_metadata) {
			uprintf("Still downloading metadata.");
			return false;
		}
	}

	downloadStatus->progress.BytesTotal = total;
	downloadStatus->progress.BytesTransferred = downloaded;
	downloadStatus->progress.FilesTransferred = 0;
	downloadStatus->progress.FilesTotal = m_filesToDownload.size();
	downloadStatus->jobName = m_jobDownloadName;
	downloadStatus->done = !stillDownloading;

	return true;
}