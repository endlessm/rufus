#include "stdafx.h"

extern "C" {
#include "rufus.h"
}

#include "TorrentDownloader.h"

#define INTMAX_MAX   INT64_MAX

///////////////////////////////////////////
//#define BOOST_CB_DISABLE_DEBUG
#undef malloc
#undef free
#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>

#include "boost/asio/impl/src.hpp"

namespace lt = libtorrent;

void DownloadTorrent()
{
    lt::session ses;

    lt::add_torrent_params atp;
    atp.url = "https://torrents.endlessm.com/torrents/eos-i386-i386-pt_BR-2.6.5-full+creator.torrent";
    atp.save_path = "."; // save in current dir
    lt::error_code error_code;
    lt::torrent_handle h = ses.add_torrent(atp, error_code);

    for (;;) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        for (lt::alert const* a : alerts) {
            //std::cout << a->message() << std::endl;
            uprintf("%s", a->message().c_str());
            // if we receive the finished alert or an error, we're done
            if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
                goto done;
            }
            if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                goto done;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
done:
    std::cout << "done, shutting down" << std::endl;
}


///////////////////////////////////////////