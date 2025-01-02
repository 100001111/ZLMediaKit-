//
// Created by Neeson on 2/1/25.
//

#include "mk_tuned_rtsp_server_n_proxies.h"

// 参考: WebApi.cpp -> ServiceController
class ProxyContainer {
public:
    using Pointer = std::shared_ptr<PlayerProxy>;
    std::unordered_map<std::string, Pointer> _map;
    mutable std::recursive_mutex _mtx;

    void clear() {
        decltype(_map) copy;
        {
            std::lock_guard<std::recursive_mutex> lck(_mtx);
            copy.swap(_map);
        }
    }

    size_t erase(const std::string &key) {
        std::lock_guard<std::recursive_mutex> lck(_mtx);
        return _map.erase(key);
    }

    size_t size() {
        std::lock_guard<std::recursive_mutex> lck(_mtx);
        return _map.size();
    }

    Pointer find(const std::string &key) const {
        std::lock_guard<std::recursive_mutex> lck(_mtx);
        auto it = _map.find(key);
        if (it == _map.end()) {
            return nullptr;
        }
        return it->second;
    }

    void for_each(const std::function<void(const std::string&, const Pointer&)>& cb) {
        std::lock_guard<std::recursive_mutex> lck(_mtx);
        auto it = _map.begin();
        while (it != _map.end()) {
            cb(it->first, it->second);
            it++;
        }
    }

    template<class ...Args>
    Pointer make(const std::string &key, Args&& ..._args) {
        // assert(!find(key));

        auto server = std::make_shared<PlayerProxy>(std::forward<Args>(_args)...);
        std::lock_guard<std::recursive_mutex> lck(_mtx);
        auto it = _map.emplace(key, server);
        assert(it.second);
        return server;
    }
};
static ProxyContainer proxy_container;

ProtocolOption create_default_option() {
    ProtocolOption option;
    option.enable_rtsp = true;
    option.add_mute_audio = false;
    option.auto_close = false; // turn off this to enable no consumer hooker.
    option.continue_push_ms = 15000;
    option.modify_stamp = 0;
    option.enable_audio = false;
    option.paced_sender_ms = 0;
    option.enable_fmp4 = false;
    option.enable_hls = false;
    option.enable_hls_fmp4 = false;
    option.enable_mp4 = false;
    option.enable_rtmp = false;
    option.enable_ts = false;
    option.hls_demand = true;
    option.rtsp_demand = false;
    option.rtmp_demand = true;
    option.ts_demand = true;
    option.fmp4_demand = true;

    return option;
}

const int MAX_CACHED_PROXY = 500;

API_EXPORT void API_CALL mk_setup_tuned_rtsp_server_n_proxies() {

    // Create stream server
    char server_id[] = "123";
    mk_tuned_rtsp_server_start(8899, server_id, false);

    // Process proxies
    proxy_container.clear();

    //// Create a demo media tuple.
    MediaPlus exampleStream;
    exampleStream.pull_address = "rtsp://127.0.0.1:8554/live/test";
    exampleStream.vhost = "__defaultVhost__";
    exampleStream.app = "vms";
    exampleStream.stream = "0";

    MediaPlus streams[1];
    streams[0] = exampleStream;

    //// Predefined proxies
    for (long i = 0; i < sizeof streams; i ++) {
        if (i > MAX_CACHED_PROXY) break; // Max cache MAX_CACHED_PROXY proxies.

        auto mt = streams[i];
        auto proxy = proxy_container.make(mt.shortUrl(), mt, create_default_option(), -1);
        (*proxy)[Client::kRtpType] = 0; // rtsp pulling protocol, 0: tcp, 1: udp, 2: broadcast
        (*proxy)[Client::kTimeoutMS] = 2000; // within 2 seconds must establish connection to original stream.

        auto key = mt.shortUrl();
        proxy->setOnClose([key](const SockException &ex) {
            proxy_container.erase(key);
        });
    }
}

API_EXPORT void API_CALL pull_stream(MediaPlus &media) {
    // Check if streaming already to ensure one stream has only one proxy.
    auto key = media.shortUrl();
    auto proxy = proxy_container.find(key);
    if (proxy && proxy->getStatus() == 1) {
        // It has a proxy already.
        return;
    }

    if (proxy == nullptr) {
        // The proxy has yet been created, create it now.
        proxy = proxy_container.make(key, media, create_default_option(), -1);
        (*proxy)[Client::kRtpType] = 0; // rtsp pulling protocol, 0: tcp, 1: udp, 2: broadcast
        (*proxy)[Client::kTimeoutMS] = 2000; // within 2 seconds must establish connection to original stream.
        proxy->setOnClose([key](const SockException &ex) {
            proxy_container.erase(key);
        });
    }

    proxy->play(media.pull_address);
}

API_EXPORT void API_CALL stop_pulling(MediaPlus &media) {

    // Reference: PlayerProxy.cpp bool PlayerProxy::close(MediaSource &sender).
    // Note: The difference is that, this function will never trigger _on_close call.

    auto key = media.shortUrl();
    auto proxy = proxy_container.find(key);

    if (proxy && proxy->getStatus() == 1) {
        shared_ptr<MediaSource> src = MediaSource::find("rtsp", media.vhost, media.app, media.stream);
        proxy->getPoller()->async_first([src, proxy]() {
            src->getMuxer().reset();
            proxy->setMediaSource(nullptr);
            proxy->teardown();
        });

        WarnL << "close media: " << media.pull_address;
    }
}

API_EXPORT void API_CALL test2() {
    //// Pull stream proxy list
    MediaTuple tuple = {"__defaultVhost__", "vms", "0", ""};
    ProtocolOption option;
    proxy_container.make("abc", tuple, option, 1);

}

API_EXPORT void API_CALL test() {
    //// Pull stream proxy list
    auto pt = proxy_container.find("abc");
    if (pt == nullptr) {
        std::cout << "abc not found" << std::endl;
    } else {
        std::cout << "abc found!" << std::endl;
    }
}