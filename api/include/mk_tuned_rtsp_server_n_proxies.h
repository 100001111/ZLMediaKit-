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


#include "mk_common.h"
#include "mk_util.h"

#ifdef __cplusplus
extern "C" {
#endif


API_EXPORT void API_CALL mk_setup_tuned_rtsp_server_n_proxies();

API_EXPORT void API_CALL pull_proxy_stream();

API_EXPORT void API_CALL stop_proxy_pulling();

API_EXPORT void API_CALL test2();

API_EXPORT void API_CALL test();

#ifdef __cplusplus
}
#endif

#endif // ZLMEDIAKIT_MK_TUNED_RTSP_SERVER_N_PROXIES_H
