﻿/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "mk_common.h"
#include <stdarg.h>
#include <unordered_map>
#include "Util/logger.h"
#include "Util/SSLBox.h"
#include "Util/File.h"
#include "Network/TcpServer.h"
#include "Network/UdpServer.h"
#include "Thread/WorkThreadPool.h"

#include "Rtsp/RtspSession.h"
#include "Rtmp/RtmpSession.h"
#include "Http/HttpSession.h"
#include "Shell/ShellSession.h"
using namespace std;
using namespace toolkit;
using namespace mediakit;

static TcpServer::Ptr rtsp_server[2];
static TcpServer::Ptr rtmp_server[2];
static TcpServer::Ptr http_server[2];
static TcpServer::Ptr shell_server;

#ifdef ENABLE_RTPPROXY
#include "Rtp/RtpServer.h"
static RtpServer::Ptr rtpServer;
#endif

#ifdef ENABLE_WEBRTC
#include "../webrtc/WebRtcSession.h"
static UdpServer::Ptr rtcServer_udp;
static TcpServer::Ptr rtcServer_tcp;
#endif

#if defined(ENABLE_SRT)
#include "../srt/SrtSession.hpp"
#include "mk_util.h"
static UdpServer::Ptr srtServer;
#endif

//////////////////////////environment init///////////////////////////

API_EXPORT void API_CALL mk_env_init(const mk_config *cfg) {
    assert(cfg);
    mk_env_init1(cfg->thread_num,
                 cfg->log_level,
                 cfg->log_mask,
                 cfg->log_file_path,
                 cfg->log_file_days,
                 cfg->ini_is_path,
                 cfg->ini,
                 cfg->ssl_is_path,
                 cfg->ssl,
                 cfg->ssl_pwd);
}

extern void stopAllTcpServer();

API_EXPORT void API_CALL mk_stop_all_server(){
    CLEAR_ARR(rtsp_server);
    CLEAR_ARR(rtmp_server);
    CLEAR_ARR(http_server);
    shell_server = nullptr;
#ifdef ENABLE_RTPPROXY
    rtpServer = nullptr;
#endif
#ifdef ENABLE_WEBRTC
    rtcServer_udp = nullptr;
    rtcServer_tcp = nullptr;
#endif
#ifdef ENABLE_SRT
    srtServer = nullptr;
#endif
    stopAllTcpServer();
}

API_EXPORT void API_CALL mk_env_init2(int thread_num,
                                      int log_level,
                                      int log_mask,
                                      const char *log_file_path,
                                      int log_file_days,
                                      int ini_is_path,
                                      const char *ini,
                                      int ssl_is_path,
                                      const char *ssl,
                                      const char *ssl_pwd) {
    // 确保只初始化一次  [AUTO-TRANSLATED:e4b32b0f]
    // Ensure initialization only happens once
    static onceToken token([&]() {
        if (log_mask & LOG_CONSOLE) {
            // 控制台日志  [AUTO-TRANSLATED:5c00e83f]
            // Console log
            Logger::Instance().add(std::make_shared<ConsoleChannel>("ConsoleChannel", (LogLevel) log_level));
        }

        if (log_mask & LOG_CALLBACK) {
            // 广播日志  [AUTO-TRANSLATED:67556df8]
            // Broadcast log
            Logger::Instance().add(std::make_shared<EventChannel>("EventChannel", (LogLevel) log_level));
        }

        if (log_mask & LOG_FILE) {
            // 日志文件  [AUTO-TRANSLATED:afacc934]
            // Log file
            auto channel = std::make_shared<FileChannel>("FileChannel",
                                                         log_file_path ? File::absolutePath("", log_file_path) :
                                                         exeDir() + "log/", (LogLevel) log_level);
            channel->setMaxDay(log_file_days ? log_file_days : 1);
            Logger::Instance().add(channel);
        }

        // 异步日志线程  [AUTO-TRANSLATED:1cc193a1]
        // Asynchronous log thread
        Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

        // 设置线程数  [AUTO-TRANSLATED:22ec5cc9]
        // Set thread count
        EventPollerPool::setPoolSize(thread_num);
        WorkThreadPool::setPoolSize(thread_num);

        if (ini && ini[0]) {
            // 设置配置文件  [AUTO-TRANSLATED:2216856d]
            // Set configuration file
            if (ini_is_path) {
                try {
                    mINI::Instance().parseFile(ini);
                } catch (std::exception &) {
                    InfoL << "dump ini file to:" << ini;
                    mINI::Instance().dumpFile(ini);
                }
            } else {
                mINI::Instance().parse(ini);
            }
        }

        if (ssl && ssl[0]) {
            // 设置ssl证书  [AUTO-TRANSLATED:e441027c]
            // Set SSL certificate
            SSL_Initor::Instance().loadCertificate(ssl, true, ssl_pwd ? ssl_pwd : "", ssl_is_path);
        }
    });
}

API_EXPORT void API_CALL mk_set_log(int file_max_size, int file_max_count) {
    auto channel = dynamic_pointer_cast<FileChannel>(Logger::Instance().get("FileChannel"));
    if (channel) {
        channel->setFileMaxSize(file_max_size);
        channel->setFileMaxCount(file_max_count);
    }
}

API_EXPORT void API_CALL mk_set_option(const char *key, const char *val) {
    assert(key && val);
    if (mINI::Instance().find(key) == mINI::Instance().end()) {
        WarnL << "key:" << key << " not existed!";
        return;
    }
    mINI::Instance()[key] = val;
    // 广播配置文件热加载  [AUTO-TRANSLATED:7ae561f3]
    // Broadcast configuration file hot reload
    NOTICE_EMIT(BroadcastReloadConfigArgs, Broadcast::kBroadcastReloadConfig);
}

API_EXPORT const char * API_CALL mk_get_option(const char *key)
{
    assert(key);
    if (mINI::Instance().find(key) == mINI::Instance().end()) {
        WarnL << "key:" << key << " not existed!";
        return nullptr;
    }
    return mINI::Instance()[key].data();
}


API_EXPORT uint16_t API_CALL mk_http_server_start(uint16_t port, int ssl) {
    ssl = MAX(0,MIN(ssl,1));
    try {
        http_server[ssl] = std::make_shared<TcpServer>();
        if(ssl){
            http_server[ssl]->start<SessionWithSSL<HttpSession> >(port);
        } else{
            http_server[ssl]->start<HttpSession>(port);
        }
        return http_server[ssl]->getPort();
    } catch (std::exception &ex) {
        http_server[ssl] = nullptr;
        WarnL << ex.what();
        return 0;
    }
}

API_EXPORT uint16_t API_CALL mk_rtsp_server_start(uint16_t port, int ssl) {
    ssl = MAX(0,MIN(ssl,1));
    try {
        rtsp_server[ssl] = std::make_shared<TcpServer>();
        if(ssl){
            rtsp_server[ssl]->start<SessionWithSSL<RtspSession> >(port);
        }else{
            rtsp_server[ssl]->start<RtspSession>(port);
        }
        return rtsp_server[ssl]->getPort();
    } catch (std::exception &ex) {
        rtsp_server[ssl] = nullptr;
        WarnL << ex.what();
        return 0;
    }
}

API_EXPORT uint16_t API_CALL mk_tuned_rtsp_server_start(uint16_t port, const char* server_id, int ssl, int auto_close) {
    ssl = MAX(0, MIN(ssl,1));

    // Create tuned configuration.
    mk_ini ini = mk_ini_default();
    mk_ini_set_option(ini, "api.apiDebug", "0"); // Disable http logs
    mk_ini_set_option(ini, "api.secret", "035c73f7-bb6b-4889-a715-d9eb2d1925c1");

    // [protocol]
    mk_ini_set_option_int(ini, "protocol.modify_stamp", 0); // Use original timestamp
    mk_ini_set_option_int(ini, "protocol.enable_audio", 0); // Disable audio
    mk_ini_set_option_int(ini, "protocol.auto_close", auto_close); // Enable auto close when no consumer
    mk_ini_set_option_int(ini, "protocol.enable_hls", 0); // Disable hls
    mk_ini_set_option_int(ini, "protocol.enable_hls_fmp4", 0); // Disable hls fmp4
    mk_ini_set_option_int(ini, "protocol.enable_mp4", 0); // Disable mp4 recording
    mk_ini_set_option_int(ini, "protocol.enable_rtsp", 1); // Enable RTSP/webrtc
    mk_ini_set_option_int(ini, "protocol.enable_rtmp", 0); // Disable RTMP/flv
    mk_ini_set_option_int(ini, "protocol.enable_ts", 0); // Disable http-ts/ws-ts
    mk_ini_set_option_int(ini, "protocol.enable_fmp4", 0); // Disable http-fmp4/ws-fmp4
    mk_ini_set_option_int(ini, "protocol.enable_rtsp", 1); // Enable RTSP/webrtc

    mk_ini_set_option_int(ini, "protocol.hls_demand", 1); // For performance, make hls on demand
    mk_ini_set_option_int(ini, "protocol.rtmp_demand", 1); // For performance, make rtmp on demand
    mk_ini_set_option_int(ini, "protocol.ts_demand", 1); // For performance, make ts on demand
    mk_ini_set_option_int(ini, "protocol.fmp4_demand", 1); // For performance, make fmp4 on demand

    mk_ini_set_option_int(ini, "general.maxStreamWaitMS", 2000); // If within 2 seconds no steam established, just return fail.
    mk_ini_set_option_int(ini, "general.mergeWriteMS", 0); // Disable pack writing of socket.

    mk_ini_set_option(ini, "general.mediaServerId", server_id);
    mk_ini_set_option_int(ini, "general.wait_audio_track_data_ms", 200); // > 0.2 second if audio track not loaded, then ignore it.
    mk_ini_set_option_int(ini, "general.unready_frame_cache", 50); // if track is yet ready, cache the frames, 50 is them maximum number, to avoid memory overflow.

    mk_ini_set_option_int(ini, "rtp.audioMtuSize", 10);
    mk_ini_set_option_int(ini, "rtp.videoMtuSize", 700);
    mk_ini_set_option_int(ini, "rtp.lowLatency", 1);
    mk_ini_set_option_int(ini, "rtp.gop_cache", 1);

    mk_ini_set_option_int(ini, "rtsp.directProxy", 0); // Disable direct proxy.
    mk_ini_set_option_int(ini, "rtsp.lowLatency", 1); // Enable low latency.

    mk_config config;
    config.thread_num = 4;
    config.log_level = 0;
    config.log_mask = LOG_FILE;
    config.ini_is_path = false;
    config.log_file_days = 30;
    config.ini = mk_ini_dump_string(ini);
    config.ssl_is_path = false;
    config.ssl = nullptr;
    config.ssl_pwd = nullptr;

    mk_env_init(&config);
    return mk_rtsp_server_start(port, ssl);
}

API_EXPORT uint16_t API_CALL mk_rtmp_server_start(uint16_t port, int ssl) {
    ssl = MAX(0,MIN(ssl,1));
    try {
        rtmp_server[ssl] = std::make_shared<TcpServer>();
        if(ssl){
            rtmp_server[ssl]->start<SessionWithSSL<RtmpSession> >(port);
        }else{
            rtmp_server[ssl]->start<RtmpSession>(port);
        }
        return rtmp_server[ssl]->getPort();
    } catch (std::exception &ex) {
        rtmp_server[ssl] = nullptr;
        WarnL << ex.what();
        return 0;
    }
}

API_EXPORT uint16_t API_CALL mk_rtp_server_start(uint16_t port){
#ifdef ENABLE_RTPPROXY
    try {
        // 创建rtp 服务器  [AUTO-TRANSLATED:480fda83]
        // Create RTP server
        rtpServer = std::make_shared<RtpServer>();
        rtpServer->start(port);
        return rtpServer->getPort();
    } catch (std::exception &ex) {
        rtpServer = nullptr;
        WarnL << ex.what();
        return 0;
    }
#else
    WarnL << "未启用该功能!";
    return 0;
#endif
}

API_EXPORT uint16_t API_CALL mk_rtc_server_start(uint16_t port) {
#ifdef ENABLE_WEBRTC
    try {
        // 创建rtc udp服务器  [AUTO-TRANSLATED:9287972e]
        // Create RTC UDP server
        rtcServer_udp = std::make_shared<UdpServer>();
        rtcServer_udp->setOnCreateSocket([](const EventPoller::Ptr &poller, const Buffer::Ptr &buf, struct sockaddr *, int) {
            if (!buf) {
                return Socket::createSocket(poller, false);
            }
            auto new_poller = WebRtcSession::queryPoller(buf);
            if (!new_poller) {
                // 该数据对应的webrtc对象未找到，丢弃之  [AUTO-TRANSLATED:d401f8cb]
                // The WebRTC object corresponding to this data was not found, discard it
                return Socket::Ptr();
            }
            return Socket::createSocket(new_poller, false);
        });
        rtcServer_udp->start<WebRtcSession>(port);
        // 创建rtc tcp服务器  [AUTO-TRANSLATED:1eefd92f]
        // Create RTC TCP server
        rtcServer_tcp = std::make_shared<TcpServer>();
        rtcServer_tcp->start<WebRtcSession>(rtcServer_udp->getPort());
        return rtcServer_udp->getPort();

    } catch (std::exception &ex) {
        rtcServer_udp = nullptr;
        rtcServer_tcp = nullptr;
        WarnL << ex.what();
        return 0;
    }
#else
    WarnL << "未启用webrtc功能, 编译时请开启ENABLE_WEBRTC";
    return 0;
#endif
}

#ifdef ENABLE_WEBRTC
class WebRtcArgsUrl : public mediakit::WebRtcArgs {
public:
    WebRtcArgsUrl(std::string url) { _url = std::move(url); }

    toolkit::variant operator[](const std::string &key) const override {
        if (key == "url") {
            return _url;
        }
        return "";
    }

private:
    std::string _url;
};
#endif

API_EXPORT void API_CALL mk_webrtc_get_answer_sdp(void *user_data,  on_mk_webrtc_get_answer_sdp cb, const char *type,
                                                   const char *offer, const char *url) {
    mk_webrtc_get_answer_sdp2(user_data, nullptr, cb, type, offer, url);
}
API_EXPORT void API_CALL mk_webrtc_get_answer_sdp2(void *user_data, on_user_data_free user_data_free, on_mk_webrtc_get_answer_sdp cb, const char *type,
                                                  const char *offer, const char *url) {
#ifdef ENABLE_WEBRTC
    assert(type && offer && url && cb);
    auto session = std::make_shared<HttpSession>(Socket::createSocket());
    std::string offer_str = offer;
    std::shared_ptr<void> ptr(user_data, user_data_free ? user_data_free : [](void *) {});
    auto args = std::make_shared<WebRtcArgsUrl>(url);
    WebRtcPluginManager::Instance().negotiateSdp(*session, type, *args, [offer_str, session, ptr, cb](const WebRtcInterface &exchanger) mutable {
        auto &handler = const_cast<WebRtcInterface &>(exchanger);
        try {
            auto sdp_answer = handler.getAnswerSdp(offer_str);
            cb(ptr.get(), sdp_answer.data(), nullptr);
        } catch (std::exception &ex) {
            cb(ptr.get(), nullptr, ex.what());
        }
    });
#else
    WarnL << "未启用webrtc功能, 编译时请开启ENABLE_WEBRTC";
#endif
}

API_EXPORT uint16_t API_CALL mk_srt_server_start(uint16_t port) {
#ifdef ENABLE_SRT
    try {
        srtServer = std::make_shared<UdpServer>();
        srtServer->setOnCreateSocket([](const EventPoller::Ptr &poller, const Buffer::Ptr &buf, struct sockaddr *, int) {
            if (!buf) {
                return Socket::createSocket(poller, false);
            }
            auto new_poller = SRT::SrtSession::queryPoller(buf);
            if (!new_poller) {
                // 握手第一阶段  [AUTO-TRANSLATED:6b3abcd4]
                // Handshake stage one
                return Socket::createSocket(poller, false);
            }
            return Socket::createSocket(new_poller, false);
        });
        srtServer->start<SRT::SrtSession>(port);
        return srtServer->getPort();

    } catch (std::exception &ex) {
        srtServer = nullptr;
        WarnL << ex.what();
        return 0;
    }
#else
    WarnL << "未启用该功能!";
    return 0;
#endif
}

API_EXPORT uint16_t API_CALL mk_shell_server_start(uint16_t port){
    try {
        shell_server =  std::make_shared<TcpServer>();
        shell_server->start<ShellSession>(port);
        return shell_server->getPort();
    } catch (std::exception &ex) {
        shell_server = nullptr;
        WarnL << ex.what();
        return 0;
    }
}
