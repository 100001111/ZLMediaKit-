//
// Created by Neeson on 3/1/25.
//

#ifndef ZLMEDIAKIT_RTSPSERVER_H
#define ZLMEDIAKIT_RTSPSERVER_H

#include "mk_common.h"
#include "mk_events.h"
#include "mk_util.h"
#include <string>

struct RtspProxy {
    std::string vHost;
    std::string app;
    std::string streamId;
    std::string pullingAddr; // The original streaming address, usually is a CAM's rtsp address.
};

class RtspServer {
public:
    RtspServer(short portId, const std::string &serverId, bool autoClose);
    void setupRtspServer(short portId, const std::string &serverId, const std::vector<RtspProxy>& proxies);
    void runProxy(const RtspProxy &rtspProxy);
    void stopProxy(const RtspProxy &rtspProxy);
};

#endif // ZLMEDIAKIT_RTSPSERVER_H
