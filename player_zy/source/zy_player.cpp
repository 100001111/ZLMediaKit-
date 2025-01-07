//
// Created by Neeson on 7/1/25.
//

#include "zy_player.h"
#include <iostream>


void ZyPlayer::play(const char* url) {
    std::cout << this->title << std::endl;
    auto mediaPlayer = std::make_shared<MediaPlayer>();
    weak_ptr<MediaPlayer> weakPlayer = mediaPlayer;
    auto dp = this->displayer;
    auto displayHelper = this->helper;

    mediaPlayer->setOnPlayResult([weakPlayer, dp, displayHelper](const SockException &ex) {
        InfoL << "On play start: " << ex.what();
        auto strongPlayer = weakPlayer.lock();
        if (ex || !strongPlayer) return;

        auto videoTrack = dynamic_pointer_cast<VideoTrack>(strongPlayer->getTrack(TrackVideo, false));
        if (videoTrack) {
            auto decoder = make_shared<FFmpegDecoder>(videoTrack);
            decoder->setOnDecode([dp, displayHelper](const FFmpegFrame::Ptr &fFrame) {
                displayHelper->doTask([fFrame, dp]() {
                    dp->display(fFrame->get());
                    return true;
                });
            });
            videoTrack->addDelegate([decoder](const Frame::Ptr &frame) { return decoder->inputFrame(frame, false, true); });
        }
    });

    mediaPlayer->setOnShutdown([](const SockException &ex) {
        WarnL << "ZyPlayer shutdown: " << ex.what();
    });

    (*mediaPlayer)[Client::kRtpType] = 0; // use tcp
    (*mediaPlayer)[Client::kWaitTrackReady] = false; // No waiting for track ready.

    mediaPlayer->play(url);
    this->helper->runLoop();
}

ZyPlayer::ZyPlayer(void *hwnd, const char *title) {
    this->helper = new DisplayHelper();
    this->displayer = new Displayer(hwnd, title);

    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());
    InfoL << "ZyPlayer initialized";
}

ZyPlayer::~ZyPlayer() {
    this->stop();

    delete this->helper;
    delete this->displayer;

    this->helper = nullptr;
    this->displayer = nullptr;

    InfoL << "ZyPlayer destroyed";
}

void ZyPlayer::stop() {
    this->helper->shutdown();
}
