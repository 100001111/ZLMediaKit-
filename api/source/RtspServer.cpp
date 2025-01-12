//
// Created by Neeson on 3/1/25.
//

#include "RtspServer.h"

#include "Player/PlayerProxy.h"
#include "mk_events.h"
#include "mk_proxyplayer.h"

using namespace std;
using namespace toolkit;
using namespace mediakit;

const int MAX_RETRY = -1;
const int RETRY_WAITING = 10000; // 10 seconds

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
        // assert(it.second);
        return server;
    }
};

static ProxyContainer proxy_container;

ProtocolOption defaultProtocol() {
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

shared_ptr<PlayerProxy> setup_new_proxy(const std::string& key, MediaTuple &mt) {
    auto proxy = proxy_container.make(key, mt, defaultProtocol(), MAX_RETRY);
    (*proxy)[Client::kRtpType] = 0; // rtsp pulling protocol, 0: tcp, 1: udp, 2: broadcast
    (*proxy)[Client::kTimeoutMS] = RETRY_WAITING; // within 10 seconds must establish connection to original stream.

    proxy->setOnClose([key](const SockException &ex) {
        // If the stream is closed by the original remote side, clear the queue.
        proxy_container.erase(key);
    });
    return proxy;
}

void on_mk_media_no_reader(mk_media_source sender) {
    // TODO: make media source cached able for fast establishing.
    auto src = (MediaSource *)sender;
    const std::string key = src->getMediaTuple().shortUrl();
    log_printf(0, "I received no watching signal of %s, "
                  "by current implementation there is no cache "
                  "mechanism for fast establishing, I will force "
                  "close the stream by now", &key);

    auto proxy = proxy_container.find(key);

    if (proxy) {
        // Stop media source first
        proxy->getPoller()->async_first([src, proxy]() {
            src->close(true);
            proxy->setMediaSource(nullptr);
            proxy->teardown();
            proxy->clear();
        });

        proxy_container.erase(key);
    }
}

void RtspServer::start(short portId, const string &serverId, bool autoClose, const std::vector<RtspProxy>& proxies) {
    // Create rtsp server
    mk_tuned_rtsp_server_start(portId, serverId.c_str(), false, autoClose);
    proxy_container.clear();

    if (!autoClose) {
        mk_events events;
        events.on_mk_media_no_reader = on_mk_media_no_reader;
        mk_events_listen(&events);
    }
}

void RtspServer::startProxy(const RtspProxy &rtspProxy) {
    auto mediaTuple = MediaTuple { rtspProxy.vHost, rtspProxy.app, rtspProxy.streamId };
    auto key = mediaTuple.shortUrl();
    auto proxy = proxy_container.find(key);
    if (proxy) {
        // The pulling proxy is already started.
        log_printf(0, "The proxy for %s is already running, so I skip here.", &key);
        return;
    }

    // Create new proxy
    proxy = setup_new_proxy(key, mediaTuple);
    proxy->play(rtspProxy.pullingAddr);
}

void RtspServer::stopProxy(const RtspProxy &rtspProxy) {
    auto mediaTuple = MediaTuple { rtspProxy.vHost, rtspProxy.app, rtspProxy.streamId };
    auto key = mediaTuple.shortUrl();
    auto proxy = proxy_container.find(key);

    if (proxy) {
        // Stop media source first
        shared_ptr<MediaSource> src = MediaSource::find("rtsp", mediaTuple.vhost, mediaTuple.app, mediaTuple.stream);
        proxy->getPoller()->async_first([src, proxy]() {
            src->close(true);
            proxy->setMediaSource(nullptr);
            proxy->teardown();
            proxy->clear();
        });

        proxy_container.erase(key);
    }
}
std::vector<RtspProxy> RtspServer::getLiveProxies() {

    std::vector<RtspProxy> proxies;
    proxy_container.for_each([&proxies](const std::string& key, const PlayerProxy::Ptr& p) {
        auto tuple = p->getMediaTuple();
        proxies.push_back({ tuple.vhost, tuple.app, tuple.stream, p->getUrl() });
    });

    return proxies;
}

void RtspServer::stop() {
    cout << "Now I am stopping the rtsp server" << endl;
    log_printf(0, "Now I am stopping the rtsp server");
    mk_stop_all_server();
    log_printf(0, "I have stopped the rtsp server");
}
