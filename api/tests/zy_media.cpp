//
// Created by Neeson on 3/1/25.
//

#include "RtspServer.h"
#include <unistd.h>
#include <vector>
#include <iostream>

int main(int argc, char *argv[]) {

    std::vector<RtspProxy> proxies;
    RtspServer::start(8899, "", false, proxies);

    const RtspProxy rp {
        "__defaultVhost__",
        "vms",
        "0",
        "rtsp://127.0.0.1:8554/live/test"
    };

    RtspServer::startProxy(rp);
    usleep(15000000); // Play for 5 seconds.

    RtspServer::stop();

    std::cout << "This should have reached after get killed message" << std::endl;
    usleep(25000000); // Play for 15 seconds.
//    zyMedia.stopProxy(rp);
//    usleep(5000000); // Play for 15 seconds.
//    zyMedia.startProxy(rp);
//    usleep(5000000); // Play for 15 seconds.
//    zyMedia.stopProxy(rp);
}