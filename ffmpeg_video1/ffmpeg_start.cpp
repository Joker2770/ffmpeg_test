#include "ffmpeg_start.h"

int main(int argc, char *argv[])
{
	av_register_all();
	AVFormatContext* pFormatCtx = NULL;

	//Open a video file
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) return -1;	//Could not open file

	//Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) return -1;

	//Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	int i;
	AVCodecContext* pCodecCtx = NULL;

	//Find first video stream
	int VideoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			VideoStream = i;
			break;
		}
	}

	printf("VideoStream = %d\n", VideoStream);
	if (VideoStream == -1) return -1; // Didn't find a video stream

	//Get a pointer to the codec context for the video stream
	pCodecCtx = pFormatCtx->streams[VideoStream]->codec;

	AVCodec* pCodec = NULL;

	//Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		fprintf(stderr, "Unsupported codec!\n");
		printf("Unsupported codec!\n");
		return -1; // Codec not found
	}
	//Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) return -1; // Could not open codec

	//////////////////////////////////////////////////////////////////////////
	//Store the data
	AVFrame* pFrame = NULL;
	AVFrame* pFrameRGB = NULL;

	//Allocate video frame
	pFrame = av_frame_alloc();
	// Allocate an AVFrame structure
	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
		return -1;

	uint8_t *buffer = NULL;
	
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
	int frameFinished = 0;
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

	i = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		//Is this a packet from the video stream?
		if (packet.stream_index == VideoStream)
		{
			//Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			//Did we get a video frame?
			if (frameFinished)
			{
				//Convert the image from its format to RGB
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);

				// Save the frame to disk
				if (++i <= 5)
					SaveFrame(pFrameRGB, pCodecCtx->width,
						pCodecCtx->height, i);
				else break;
			}

		}

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}
	// Free the RGB image
	av_free(buffer);
	av_free(pFrameRGB);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

    return 0;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "frame%d.ppm", iFrame);
	printf("frame%d.ppm", iFrame);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);
	printf("P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y < height; y++)
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}