//
// Created by Neeson on 3/1/25.
//

#include "RtspServer.h"
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[]) {
    RtspServer zyMedia(8899, "", false);

    const RtspProxy rp {
        "__defaultVhost__",
        "vms",
        "0",
        "rtsp://127.0.0.1:8554/live/test"
    };

//    std::vector<RtspProxy> preProxies;
//    preProxies.push_back(rp);

//    usleep(5000000); // Play for 15 seconds.

    zyMedia.runProxy(rp);
    usleep(150000000); // Play for 15 seconds.
//    zyMedia.stopProxy(rp);
//    usleep(5000000); // Play for 15 seconds.
//    zyMedia.runProxy(rp);
//    usleep(5000000); // Play for 15 seconds.
//    zyMedia.stopProxy(rp);
}