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

#define MAX_AUDIO_FRAME_SIZE		(192000)  //1 second of 48khz 32bit audio
#define SDL_AUDIO_BUFFER_SIZE		(1024)    //

typedef struct PacketQueue {
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;

int quit = 0;

SDL_Event event;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_AudioSpec wanted_spec;
SDL_AudioSpec spec;

Uint8 *yPlane, *uPlane, *vPlane;
AVCodec* pCodec = NULL;
AVCodec* aCodec = NULL;
AVFrame* pFrame = NULL;
AVFrame* pFrameRGB = NULL;
AVCodecContext* pCodecCtx = NULL;
AVCodecContext* aCodecCtx = NULL;
AVFormatContext* pFormatCtx = NULL;
uint8_t *buffer = NULL;

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
void FreeSrc();
void packet_queue_init(PacketQueue *q);
int packet_queue_put(PacketQueue *q, AVPacket *pkt);
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);
void audio_callback(void *userdata, Uint8 *stream, int len);
int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size);

#endif // __FFMPEG_START_H__