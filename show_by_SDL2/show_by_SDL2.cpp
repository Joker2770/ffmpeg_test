#include "show_by_SDL2.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s input_file\n"
			"API example program to show how to output to the screen "
			"accessed through AVCodecContext.\n", argv[0]);
		return 1;
	}

	av_register_all();

	//Open a video file
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) return -1;	//Could not open file

	//Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) return -1;

	//Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	int i;

	//Find first video stream
	int VideoStream = -1;
	for (i = 0; i < (int)pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			VideoStream = i;
			break;
		}
	}

	if (VideoStream == -1) return -1; // Didn't find a video stream

	//Get a pointer to the codec context for the video stream
	//pCodecCtx = pFormatCtx->streams[VideoStream]->codec;
	pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[VideoStream]->codecpar);

	AVCodec* pCodec = NULL;

	//Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}

	//Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) return -1; // Could not open codec

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

	// set up YV12 pixel array (12 bits per pixel)
	size_t yPlaneSz, uvPlaneSz;
	int uvPitch;
	yPlaneSz = pCodecCtx->width * pCodecCtx->height;
	uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 4;
	yPlane = (Uint8*)malloc(yPlaneSz);
	uPlane = (Uint8*)malloc(uvPlaneSz);
	vPlane = (Uint8*)malloc(uvPlaneSz);
	if (!yPlane || !uPlane || !vPlane) {
		fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
		exit(1);
	}

	uvPitch = pCodecCtx->width / 2;

	i = 0;
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

				//------------SDLœ‘ æ--------
				SDL_UpdateTexture(
					texture,
					NULL,
					pFrameRGB->data[0], pFrameRGB->linesize[0]
				);

				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, NULL, NULL);
				SDL_RenderPresent(renderer);
				//—” ±10ms
				SDL_Delay(10);
				//------------SDL-----------

// 				// Save the frame to disk
// 				if (++i <= 5)
// 					SaveFrame(pFrameRGB, pCodecCtx->width,
// 						pCodecCtx->height, i);
// 				else break;
			}

		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
		//Handle event
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_QUIT)
			{
				FreeSrc();
				exit(0);
			}
			else if (event.type == SDL_KEYUP)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_q:
					FreeSrc();
					exit(0);
					break;
				case SDLK_ESCAPE:
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
		pCodecCtx = NULL;
	}

	// Close the video file
	if (pFormatCtx != NULL)
	{
		avformat_close_input(&pFormatCtx);
		pFormatCtx = NULL;
	}
	printf("Free end\n");
}