//
// Created by Neeson on 2/1/25.
//

#ifdef _WIN32
#include <io.h>
#include <iostream>
#include <tchar.h>
#endif // _WIN32

#ifndef ZLMEDIAKIT_MK_TUNED_RTSP_SERVER_N_PROXIES_H
#define ZLMEDIAKIT_MK_TUNED_RTSP_SERVER_N_PROXIES_H

#include <string>
#include <memory>
#include "mk_common.h"
#include "mk_util.h"
#include "Player/PlayerProxy.h"

#include "Common/MultiMediaSourceMuxer.h"
#include "Player/MediaPlayer.h"
#include "Util/TimeTicker.h"

using namespace std;
using namespace mediakit;
using namespace toolkit;

#ifdef __cplusplus
extern "C" {
#endif

struct MediaPlus : MediaTuple {
    std::string pull_address; // The original stream address
};

API_EXPORT void API_CALL mk_setup_tuned_rtsp_server_n_proxies();

API_EXPORT void API_CALL test2();

API_EXPORT void API_CALL test();

#ifdef __cplusplus
}
#endif

#endif // ZLMEDIAKIT_MK_TUNED_RTSP_SERVER_N_PROXIES_H
