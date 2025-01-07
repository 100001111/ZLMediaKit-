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
    DisplayHelper* helper = nullptr;
    Displayer* displayer = nullptr;
};

extern "C" ZyPlayer* ZyPlayer_Create(void *hwnd, const char *title) { return new ZyPlayer(hwnd, title); }
extern "C" void ZyPlay(ZyPlayer* zyPlayer, const char *url) { zyPlayer->play(url); }
extern "C" void ZyPlayer_Delete(ZyPlayer* zyPlayer) { delete zyPlayer; }

#endif // ZLMEDIAKIT_ZY_PLAYER_H
