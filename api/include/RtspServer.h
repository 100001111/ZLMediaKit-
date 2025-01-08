//
// Created by Neeson on 3/1/25.
//

#ifndef ZLMEDIAKIT_RTSPSERVER_H
#define ZLMEDIAKIT_RTSPSERVER_H

#include "mk_common.h"
#include "mk_events.h"
#include "mk_util.h"
#include <string>
#include <vector>

struct RtspProxy {
    std::string vHost;
    std::string app;
    std::string streamId;
    std::string pullingAddr; // The original streaming address, usually is a CAM's rtsp address.
};

class RtspServer {
public:
    static void start(short portId, const std::string &serverId, bool autoClose, const std::vector<RtspProxy>& proxies);
    static void stop();
    static void startProxy(const RtspProxy &rtspProxy);
    static void stopProxy(const RtspProxy &rtspProxy);
    static std::vector<RtspProxy> getLiveProxies();
};

#endif // ZLMEDIAKIT_RTSPSERVER_H
