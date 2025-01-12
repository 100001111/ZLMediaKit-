//
// Created by Neeson on 7/1/25.
//

#ifndef ZLMEDIAKIT_ZY_PLAYER_H
#define ZLMEDIAKIT_ZY_PLAYER_H

#include "Codec/Transcode.h"
#include "Common/config.h"
#include "Player/MediaPlayer.h"
#include "Util/logger.h"
#include "displayer.h"

using namespace std;
using namespace toolkit;
using namespace mediakit;

class ZyPlayer {
public:
    explicit ZyPlayer(void *hwnd, const char *title = "");
    ~ZyPlayer();
    void play(const char* url);
    void stop();
private:
    const char *title = "Hello World!";
    Displayer *displayer = nullptr;
    MediaPlayer *mediaPlayer = nullptr;
    long winId = 0;
};

extern "C" __declspec(dllexport) ZyPlayer* ZyPlayer_Create(void *hwnd, const char *title) { return new ZyPlayer(hwnd, title); }
extern "C" __declspec(dllexport) void ZyPlay(ZyPlayer* zyPlayer, const char *url) { zyPlayer->play(url); }
extern "C" __declspec(dllexport) void ZyPlayer_Delete(ZyPlayer *zyPlayer) {
    delete zyPlayer;
    WarnL << "ZyPlayer, I have shutdown: ";
}
extern "C" __declspec(dllexport) void ZyPlayer_Stop(ZyPlayer *zyPlayer) { zyPlayer->stop(); }

#endif // ZLMEDIAKIT_ZY_PLAYER_H
