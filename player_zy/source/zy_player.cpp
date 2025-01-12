//
// Created by Neeson on 7/1/25.
//

#include "zy_player.h"
#include <iostream>


void ZyPlayer::play(const char* url) {
    std::cout << this->title << std::endl;
    auto mediaPlayer = this->mediaPlayer;
    auto displayer = this->displayer;

    auto win_id = this->winId;
    //auto displayHelper = this->displayHelper;
    //auto displayer = std::make_shared<Displayer>(hwnd, url); 
    mediaPlayer->setOnPlayResult([mediaPlayer, displayer, win_id](const SockException &ex) {
        InfoL << "On play start: " << ex.what();
        /*auto strongPlayer = weakMediaPlayer.lock();
        if (ex || !strongPlayer) return;*/

        auto videoTrack = dynamic_pointer_cast<VideoTrack>(mediaPlayer->getTrack(TrackVideo, false));
        if (videoTrack) {
            auto decoder = make_shared<FFmpegDecoder>(videoTrack);
            decoder->setOnDecode([displayer, win_id](const FFmpegFrame::Ptr &fFrame) {
                DisplayHelper::Instance().doTask([fFrame, displayer, win_id]() {
                    displayer->display(fFrame->get(), win_id);
                    InfoL << "Thread Id: " << SDL_GetThreadID(nullptr);
					return true;
				});
                /*displayHelper->doTask([fFrame, displayer]() {
                    displayer->display(fFrame->get());
                    return true;
                });*/
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
    InfoL << "ZyPlayer Started playing";
    //this->displayHelper->runLoop();
    //this->helper->runLoop();
    DisplayHelper::Instance().runLoop();
}

ZyPlayer::ZyPlayer(void *hwnd, const char *title) {
    this->displayer = new Displayer(hwnd, title);
    this->winId = this->displayer->getWinId();

    this->mediaPlayer = new MediaPlayer();

    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());
    InfoL << "ZyPlayer initialized";
}

ZyPlayer::~ZyPlayer() {

    //this->displayHelper->shutdown();
    this->stop();
    /*this->mediaPlayer->getPoller()->clear();
    this->mediaPlayer->getPoller().reset();
    this->mediaPlayer->teardown();
    this->mediaPlayer->clear();*/
    

    //delete this->mediaPlayer;
    //delete this->displayHelper;
    //delete this->displayer;

    //this->mediaPlayer = nullptr;
    //this->displayer = nullptr;
    //this->displayHelper = nullptr;

   
    InfoL << "ZyPlayer destroyed";
    //exit(0);
}

void ZyPlayer::stop() {
    //this->displayHelper->shutdown();
    DisplayHelper::Instance().shutdown();
}
