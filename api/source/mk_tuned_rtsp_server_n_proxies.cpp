//
// Created by Neeson on 2/1/25.
//

#include "mk_tuned_rtsp_server_n_proxies.h"
#include "Player/PlayerProxy.h"

using namespace std;
using namespace mediakit;

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

    template<class ..._Args>
    Pointer make(const std::string &key, _Args&& ...__args) {
        // assert(!find(key));

        auto server = std::make_shared<PlayerProxy>(std::forward<_Args>(__args)...);
        std::lock_guard<std::recursive_mutex> lck(_mtx);
        auto it = _map.emplace(key, server);
        assert(it.second);
        return server;
    }
};
static ProxyContainer proxy_container;

API_EXPORT void API_CALL mk_setup_tuned_rtsp_server_n_proxies() {

    // Create stream server
    char server_id[] = "123";
    mk_tuned_rtsp_server_start(8899, server_id, false);
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