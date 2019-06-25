#include "Playing_Sound.h"
#include <assert.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s input_file\n"
			"API example program to show how to output to the screen "
			"accessed through AVCodecContext.\n", argv[0]);
		return 1;
	}

	av_register_all();
	pFormatCtx = avformat_alloc_context();
	//Open a video file
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) return -1;	//Could not open file

	//Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) return -1;

	//Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	int i;

	//Find first video stream and audio stream
	int VideoStream = -1;
	int AudioStream = -1;
	for (i = 0; i < (int)pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && VideoStream < 0)
		{
			VideoStream = i;
		}
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && AudioStream < 0)
		{
			AudioStream = i;
		}
	}

	printf("\n");
	printf("VideoStream = %d\n", VideoStream);
	printf("AudioStream = %d\n", AudioStream);
	if (VideoStream == -1) return -1; //Didn't find a video stream
	if (AudioStream == -1) return -1; //Didn't find a audio stream

	//Get a pointer to the codec context for the video stream
	//pCodecCtx = pFormatCtx->streams[VideoStream]->codec;
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[VideoStream]->codecpar);

	//Get a pointer to the codec context for the audio stream
	aCodecCtx = avcodec_alloc_context3(aCodec);
	if (aCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(aCodecCtx, pFormatCtx->streams[AudioStream]->codecpar);

	//Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	//Find the decoder for the audio stream
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	if (!aCodec) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}

	//Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) return -1; // Could not open codec
	if (avcodec_open2(aCodecCtx, aCodec, NULL) < 0) return -1; // Could not open codec

	//////////////////////////////////////////////////////////////////////////
	//Setting Up the Audio
	wanted_spec.freq = aCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = aCodecCtx->channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = aCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		return -1;
	}
	// audio_st = pFormatCtx->streams[index]
	packet_queue_init(&audioq);
	SDL_PauseAudio(0);

	//////////////////////////////////////////////////////////////////////////
	//Store the data

	//Allocate video frame
	pFrame = av_frame_alloc();
	// Allocate an AVFrame structure
	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
		return -1;


	int numBytes = 0;
	//Determine required buffer size and allocate buffer
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
		pCodecCtx->height, 1);

	buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width,
		pCodecCtx->height, 1);

	//////////////////////////////////////////////////////////////////////////
	//Reading the data
	struct SwsContext* sws_ctx = NULL;
	int frameFinished;
	AVPacket packet;

	//Initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width,
		pCodecCtx->height,
		AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);

	//////////////////////////////////////////////////////////////////////////
	//SDL and Video
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	//Creating a Display

	window = SDL_CreateWindow("Video Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_RESIZABLE/* SDL_WINDOW_HIDDEN*/ | SDL_WINDOW_OPENGL);
	if (window == NULL)
	{
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "SDL: could not create renderer - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_STREAMING,
		pCodecCtx->width,
		pCodecCtx->height
	);
	if (!texture) {
		fprintf(stderr, "SDL: could not create texture - exiting\n");
		exit(1);
	}

// 	// set up YV12 pixel array (12 bits per pixel)
// 	size_t yPlaneSz, uvPlaneSz;
// 	int uvPitch;
// 	yPlaneSz = pCodecCtx->width * pCodecCtx->height;
// 	uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 4;
// 	yPlane = (Uint8*)malloc(yPlaneSz);
// 	uPlane = (Uint8*)malloc(uvPlaneSz);
// 	vPlane = (Uint8*)malloc(uvPlaneSz);
// 	if (!yPlane || !uPlane || !vPlane) {
// 		fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
// 		exit(1);
// 	}
// 
// 	uvPitch = pCodecCtx->width / 2;

	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		//Is this a packet from the video stream?
		if (packet.stream_index == VideoStream)
		{
			//Decode video frame
			int iRet = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			if (iRet < 0) {
				printf("decode error\n");
				return -1;
			}
			//Did we get a video frame?
			if (frameFinished)
			{

				//Convert the image from its format to RGB
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);

				//------------SDL show--------
				SDL_UpdateTexture(
					texture,
					NULL,
					pFrameRGB->data[0], pFrameRGB->linesize[0]
				);

				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, NULL, NULL);
				SDL_RenderPresent(renderer);
				//10ms
				//SDL_Delay(10);
				//------------SDL-----------

// 				// Save the frame to disk
// 				if (++i <= 5)
// 					SaveFrame(pFrameRGB, pCodecCtx->width,
// 						pCodecCtx->height, i);
// 				else break;
			}

		}
		else if (packet.stream_index == AudioStream) {
			packet_queue_put(&audioq, &packet);
		}
		else {
			av_free_packet(&packet);
		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		//Handle event
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_QUIT)
			{
				quit = 1;
				FreeSrc();
				exit(0);
			}
			else if (event.type == SDL_KEYUP)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_q:
					quit = 1;
					FreeSrc();
					exit(0);
					break;
				case SDLK_ESCAPE:
					quit = 1;
					FreeSrc();
					exit(0);
					break;
				default:
					break;
				}
			}
		}
	}

	return 0;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "frame%d.ppm", iFrame);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y < height; y++)
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}

void FreeSrc()
{
	printf("Free...\n");
	if (texture != NULL)
	{
		SDL_DestroyTexture(texture);
		texture = NULL;
	}
	if (renderer != NULL)
	{
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
		window = NULL;
	}

	SDL_Quit();

	// Free the YUV frame
	if (pFrame != NULL)
	{
		av_frame_free(&pFrame);
		pFrame = NULL;
	}
	if (pFrameRGB != NULL)
	{
		av_frame_free(&pFrameRGB);
		pFrameRGB = NULL;
	}
	if (yPlane != NULL)
	{
		free(yPlane);
		yPlane = NULL;
	}
	if (uPlane != NULL)
	{
		free(uPlane);
		uPlane = NULL;
	}
	if (vPlane != NULL)
	{
		free(vPlane);
		vPlane = NULL;
	}

	// Free the RGB image
	if (buffer != NULL)
	{
		av_free(buffer);
		buffer = NULL;
	}

	// Close the codecs
	if (pCodecCtx != NULL)
	{
		avcodec_close(pCodecCtx);
		av_free(pCodecCtx);
		pCodecCtx = NULL;
	}
	if (aCodecCtx != NULL)
	{
		avcodec_close(aCodecCtx);
		av_free(aCodecCtx);
		aCodecCtx = NULL;
	}

	// Close the video file
	if (pFormatCtx != NULL)
	{
		avformat_close_input(&pFormatCtx);
		avformat_free_context(pFormatCtx);
		pFormatCtx = NULL;
	}
	printf("Free end\n");
}


void audio_callback(void *userdata, Uint8 *stream, int len) {

	AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
	int len1, audio_size;

	static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;

	while (len > 0) {
		if (audio_buf_index >= audio_buf_size) {
			/* We have already sent all our data; get more */
			audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
			if (audio_size < 0) {
				/* If error, output silence */
				audio_buf_size = 1024; // arbitrary?
				memset(audio_buf, 0, audio_buf_size);
			}
			else {
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;
		}
		len1 = audio_buf_size - audio_buf_index;
		if (len1 > len)
			len1 = len;
		memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}
}
int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {

	static AVPacket pkt;
	static uint8_t *audio_pkt_data = NULL;
	static int audio_pkt_size = 0;
	static AVFrame frame;

	int len1, data_size = 0;

	for (;;) {
		while (audio_pkt_size > 0) {
			int got_frame = 0;
			len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
			if (len1 < 0) {
				/* if error, skip frame */
				audio_pkt_size = 0;
				break;
			}
			audio_pkt_data += len1;
			audio_pkt_size -= len1;
			data_size = 0;
			if (got_frame) {
				data_size = av_samples_get_buffer_size(NULL,
					aCodecCtx->channels,
					frame.nb_samples,
					aCodecCtx->sample_fmt,
					1);
				assert(data_size <= buf_size);
				memcpy(audio_buf, frame.data[0], data_size);
			}
			if (data_size <= 0) {
				/* No data yet, get more frames */
				continue;
			}
			/* We have data, return it and come back for more later */
			return data_size;
		}
		if (pkt.data)
			av_free_packet(&pkt);

		if (quit) {
			return -1;
		}

		if (packet_queue_get(&audioq, &pkt, 1) < 0) {
			return -1;
		}
		audio_pkt_data = pkt.data;
		audio_pkt_size = pkt.size;
	}
}


void packet_queue_init(PacketQueue *q)
{
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}
int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{

	AVPacketList *pkt1;
	if (av_dup_packet(pkt) < 0) {
		return -1;
	}
	pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;


	SDL_LockMutex(q->mutex);

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);

	//av_free(pkt1);
	return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	int ret;

	SDL_LockMutex(q->mutex);

	for (;;) {

		if (quit) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else if (!block) {
			ret = 0;
			break;
		}
		else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}