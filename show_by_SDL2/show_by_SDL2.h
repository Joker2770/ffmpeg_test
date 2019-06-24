#ifndef __FFMPEG_START_H__
#define __FFMPEG_START_H__

#include <stdio.h>
//  [6/24/2019 yjt95]
/*
 *	ffmpeg 4.0.4+
 *	sdl 2.0.8+
 */
#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "SDL.h"
#include "SDL_thread.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <SDL.h>
#include <SDL_thread.h>

#ifdef __cplusplus
};
#endif
#endif

SDL_Event event;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

Uint8 *yPlane, *uPlane, *vPlane;
AVCodec* pCodec = NULL;
AVFrame* pFrame = NULL;
AVFrame* pFrameRGB = NULL;
AVCodecContext* pCodecCtx = NULL;
AVFormatContext* pFormatCtx = NULL;
uint8_t *buffer = NULL;

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
void FreeSrc();

#endif // __FFMPEG_START_H__
