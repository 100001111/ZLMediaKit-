//
// Created by Neeson on 7/1/25.
//
#pragma once
#ifndef ZLMEDIAKIT_DISPLAY_HELPER_H
#define ZLMEDIAKIT_DISPLAY_HELPER_H

#include <stdexcept>
#include <deque>
#include "Util/onceToken.h"
#include "SDL2/SDL.h"
#include "libavcodec/avcodec.h"

#if defined(_WIN32)
#pragma comment(lib,"SDL2.lib")
#endif //defined(_WIN32)


using namespace std;

#define REFRESH_EVENT (SDL_USEREVENT + 1)

class DisplayHelper {
public:
    /*DisplayHelper(long winId) { this->_winId = winId;
    };*/
    static DisplayHelper &Instance() {
        static DisplayHelper *instance(new DisplayHelper);
        return *instance;
    };

    static void Destory() {
		delete &Instance();
	}

    template <typename FUNC>
    void doTask(FUNC &&f) {
        {
            lock_guard<mutex> lock(mtxTask);
            taskList.emplace_back(f);
        }
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
    }

    void runLoop() {
        bool running = true;
        function<bool()> task;
        SDL_Event event;
        while (running) {
            SDL_WaitEvent(&event);
            switch (event.type) {
                case REFRESH_EVENT: {
                    {
                        lock_guard<mutex> lock(mtxTask);
                        if (taskList.empty()) continue;
                        
                       // InfoL << "Thread Id: " << SDL_GetThreadID(nullptr);
                       // InfoL << "Win Id compare: " << event.window.windowID << " == " << this->_winId << " ?";
                        task = taskList.front();
                        taskList.pop_front();
                    }
                    running = task();
                    break;
                }
                case SDL_QUIT || (SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE): {
                    InfoL << "ZyPlayer core exit: " << event.type;
                    return;
                }
                default: break;
            }
        }
        WarnL << "ZyPlayer run loop exit";
    }

    void shutdown() {
        doTask([]() { return false; });
    }

    ~DisplayHelper() {
        shutdown();
        InfoL << "~DisplayHelper destroyed";
    }

    long _winId = 0;

    private:
    deque<function<bool()>> taskList;
    mutex mtxTask;
   
};


#endif // ZLMEDIAKIT_DISPLAY_HELPER_H
