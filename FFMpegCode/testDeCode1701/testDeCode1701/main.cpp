#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "frame%d.ppm", iFrame);
	pFile=fopen(szFilename, "wb");
	if(pFile==NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for(y=0; y<height; y++)
		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

	// Close file
	fclose(pFile);
}

int main() {
	AVFormatContext *pFormatCtx;
	int             i, videoStream;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	AVFrame         *pFrame; 
	AVFrame         *pFrameRGB;
	AVPacket        packet;
	int             frameFinished;
	int             numBytes;
	uint8_t         *buffer;
	static struct SwsContext *img_convert_ctx;
	char * filePath="test.mp4";
	// Register all formats and codecs
	av_register_all();
	// Open video file
	if(av_open_input_file(&pFormatCtx, filePath, NULL, 0, NULL)!=0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	dump_format(pFormatCtx, 0, filePath, 0);
	// Find the first video stream
	videoStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
		if(videoStream==-1)
			return -1; // Didn't find a video stream
		// Get a pointer to the codec context for the video stream
		pCodecCtx=pFormatCtx->streams[videoStream]->codec;

		// Find the decoder for the video stream
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1; // Codec not found
		}
		// Open codec
		if(avcodec_open(pCodecCtx, pCodec)<0)
			return -1; // Could not open codec

		// Allocate video frame
		pFrame=avcodec_alloc_frame();

		// Allocate an AVFrame structure
		pFrameRGB=avcodec_alloc_frame();
		if(pFrameRGB==NULL)
			return -1;


		// Determine required buffer size and allocate buffer
		numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
			pCodecCtx->height);
		buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

		// Assign appropriate parts of buffer to image planes in pFrameRGB
		// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
		// of AVPicture
		avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);

		// Read frames and save first five frames to disk
		i=0;
		while(av_read_frame(pFormatCtx, &packet)>=0) {
			if(packet.stream_index==videoStream) {
				// Decode video frame
				avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,packet.data, packet.size);
				if(frameFinished) {
					// Convert the image from its native format to RGB
					img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
					// Convert the image from its native format to RGB
					sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
					if(++i<=5)
						SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height,i);				  
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

		// Close the codec
		avcodec_close(pCodecCtx);

		// Close the video file
		av_close_input_file(pFormatCtx);

		printf("Ö´ÐÐÍê±Ï\n");
		system("pause");
		return 0;
}