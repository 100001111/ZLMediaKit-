//
// Created by Neeson on 7/1/25.
//

#ifndef ZLMEDIAKIT_ZY_PLAYER_H
#define ZLMEDIAKIT_ZY_PLAYER_H

#if defined(_MSC_VER)
#define MY_LIB_API __declspec(dllexport) // Microsoft
#elif defined(__GNUC__)
#define MY_LIB_API __attribute__((visibility("default"))) // GCC
#else
#define MY_LIB_API // Most compilers export all the symbols by default. We hope for the best here.
#pragma warning Unknown dynamic link import/export semantics.
#endif

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

extern "C" MY_LIB_API ZyPlayer* ZyPlayer_Create(void *hwnd, const char *title) { return new ZyPlayer(hwnd, title); }
extern "C" MY_LIB_API void ZyPlay(ZyPlayer* zyPlayer, const char *url) { zyPlayer->play(url); }
extern "C" MY_LIB_API void ZyPlayer_Delete(ZyPlayer *zyPlayer) {
    delete zyPlayer;
    WarnL << "ZyPlayer, I have shutdown: ";
}
extern "C" MY_LIB_API void ZyPlayer_Stop(ZyPlayer *zyPlayer) { zyPlayer->stop(); }

#endif // ZLMEDIAKIT_ZY_PLAYER_H
