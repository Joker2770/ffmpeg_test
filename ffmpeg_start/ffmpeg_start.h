#ifndef __FFMPEG_START_H__
#define __FFMPEG_START_H__

#include <stdio.h>

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
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

#ifdef __cplusplus
};
#endif
#endif

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

#endif // __FFMPEG_START_H__
