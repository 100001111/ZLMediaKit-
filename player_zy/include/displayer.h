//
// Created by Neeson on 7/1/25.
//

#pragma once
#ifndef ZLMEDIAKIT_DISPLAYER_H
#define ZLMEDIAKIT_DISPLAYER_H
#include "display_helper.h"

using namespace std;
using namespace toolkit;

class Displayer {
public:
    using Ptr = shared_ptr<Displayer>;
    explicit Displayer(void *_hwnd = nullptr, const char *_title = "untitled") {
        static toolkit::onceToken token([](){
            if (SDL_Init(SDL_INIT_VIDEO) == -1) {
                string err = "init SDL failed: ";
                err += SDL_GetError();
                ErrorL << err;
                throw runtime_error(err);
            }
            SDL_LogSetAllPriority(SDL_LOG_PRIORITY_CRITICAL);
            InfoL << "SDL initialized.";
        }, [](){
            SDL_Quit();
        });

        title = _title;
        hwnd = _hwnd;
    }

    bool display(AVFrame *frame) {
        if (!win) {
            if (hwnd) {
                win = SDL_CreateWindowFrom(hwnd);
            } else {
                win = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, frame->width, frame->height, SDL_WINDOW_OPENGL);
            }
        }

        if (win && !render) {
            // Try to create accelerated render
            render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
            if (!render) {
                render = SDL_CreateRenderer(win, -1, SDL_RENDERER_TARGETTEXTURE);
            }
            if (!render) {
                render = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
            }
        }

        if (render && !texture) {
            if (frame -> format == AV_PIX_FMT_NV12) {
                texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
            } else {
                texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
            }
        }

        if (texture) {
#if SDL_VERSION_ATLEAST(2,0,16)
            if (frame->format == AV_PIX_FMT_NV12) {
                SDL_UpdateNVTexture(texture, nullptr, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1]);
            } else
#endif
            {
                SDL_UpdateYUVTexture(texture, nullptr, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
            }

            SDL_RenderClear(render);
            SDL_RenderCopy(render, texture, nullptr, nullptr);
            SDL_RenderPresent(render);

            return true;
        }

        return false;
    }

    ~Displayer() {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (render) {
            SDL_DestroyRenderer(render);
            render = nullptr;
        }
        if (win) {
            SDL_DestroyWindow(win);
            win = nullptr;
        }
    }

private:
    string title;
    SDL_Window *win = nullptr;
    SDL_Renderer *render = nullptr;
    SDL_Texture *texture = nullptr;
    void *hwnd = nullptr;
};

#endif // ZLMEDIAKIT_DISPLAYER_H
