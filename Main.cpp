#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <winsock2.h>
#define STB_IMAGE_IMPLEMENTATION 
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION



// using SDL for showing frames in window (idea from moodle forum)
// Date: 14th June 2021
#define SDL_MAIN_HANDLED
#include <SDL.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}

#ifdef __cplusplus 
extern "C" {
#endif
#include "UDPReceive.h"
#ifdef __cplusplus 
}
#endif

#define INBUF_SIZE 4096

const char* outfilename = "screenshot";
AVFrame* picture;
AVCodecContext* c;
AVCodecParserContext* parser;
const AVCodec* codec;
int num_frame = 0;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Rect r;

// I used some code from a tutorial to get started with sdl and initialise
// Source: https://joshbeam.com/2019/01/03/getting-started-with-sdl/
// Date: 14th June 2021
int initialiseSDL(AVCodecContext* ctx) {
	// Initialize SDL with video rendering only.
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init failed");
		return -1;
	}

	r.x = 0;
	r.y = 0;
	r.w = ctx->width;
	r.h = ctx->height;

	window = SDL_CreateWindow("Hello",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640, 480, 0);
	if (!window) {
		fprintf(stderr, "SDL_CreateWindow failed");
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "SDL_CreateRenderer failed");
		return -1;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, r.w, r.h);

	return 0;
}

// 
// Source: https://wiki.libsdl.org/MigrationGuide
// Date: 14th June 2021
// updates texture via current frame data, clears renderer, moves textures data to new renderer and then presents int on screen
void display_frame_via_SDL(AVFrame* frame, AVCodecContext* ctx) {
	//SDL_UpdateTexture(texture, &r, )						// from moodle
	SDL_UpdateYUVTexture(texture, &r, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
	SDL_RenderClear(renderer);								// wipes out existing video framebuffer
	SDL_RenderCopy(renderer, texture, NULL, NULL);			// moves texture's content to the video framebuffer
	SDL_RenderPresent(renderer);							// puts it on screen
}


void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const char* filename) {

	char buf[1024];

	int ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		printf("ret: %d \n", ret);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}

		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);

		// the picture is allocated by the decoder. no need to
		//      free it
		snprintf(buf, sizeof(buf), filename, dec_ctx->frame_number);
		//pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
		display_frame_via_SDL(frame, dec_ctx);
	}
}

void initialiseReceivingAndDecodingUtilities() {
	// Winsock initialization from https://docs.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
	// Lines cited: next 9
	// date: 10th may 2021
	WSADATA wsaData;
	int iResult;

	int num_frame = 0;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return;
	}

	// decode utilities

	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	parser = av_parser_init(codec->id);
	parser->flags = PARSER_FLAG_COMPLETE_FRAMES;
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	// open it
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}
	//c->bit_rate = 400000;

	// resolution must be a multiple of two
	c->width = 800;
	c->height = 600;
	// frames per second
/*	c->framerate.num = 25;
	c->framerate.den = 1;
	c->time_base.num = 1;
	c->time_base.den = 25;
	c->pix_fmt = AV_PIX_FMT_YUV420P;*/

	picture = av_frame_alloc();
}


int main(int argc, char* argv[]) {



	initialiseReceivingAndDecodingUtilities();


	if (initialiseSDL(c) < 0) {
		fprintf(stderr, "Error while initialising SDL\n");
		return 0;
	}


	UDPReceive receiver = UDPReceive();
	receiver.init(23042);



	while (true) {

		/// receiving starts here //////////
		char buf[650000];
		ZeroMemory(buf, sizeof(buf));

		int bufLength = 0;
		double ptoime = clock() / (double)CLOCKS_PER_SEC;			// the header time will be stored inside ptoime
		int out = receiver.receive(buf, &ptoime);		// 1st packet has 9 fragments -> all frags are now in buf
																	// pkt->data stored in buf OHNE Header
																	// out is the size of all the bytes sent
		if (out == -1)		// if frags get lost, just continue with next package
			continue;

		/////////// receiving ends here ////

		/// decode starts here /////////////

		char outname[200];


		uint8_t* received_data = (uint8_t*)buf;

		// set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams)
		//memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
		//memset(received_data + sizeof(received_data), 0, AV_INPUT_BUFFER_PADDING_SIZE);

		AVPacket* pkt_out = av_packet_alloc();
		if (!pkt_out)
			exit(1);


		size_t data_size = out;
		if (!data_size)
			break;
		// use the parser to split the data into frames
		uint8_t* data = received_data;
		while (data_size > 0) {
			int ret = av_parser_parse2(parser, c, &pkt_out->data, &pkt_out->size,
				data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			std::cout << "ret: " << ret << "\n";
			if (ret < 0) {
				fprintf(stderr, "Error while parsing\n");
				exit(1);
			}

			data += ret;
			data_size -= ret;
			std::cout << pkt_out->size << " pkt_out size\n";
			if (pkt_out->size) {
				printf(outname, "%s%d.pgm", outfilename, num_frame++);
				printf("Outname %s\n", outname);
				decode(c, picture, pkt_out, outname);
			}
		}


	}

	// flush the decoder
	decode(c, picture, NULL, outfilename);

	av_parser_close(parser);
	avcodec_free_context(&c);
	av_frame_free(&picture);
	//av_packet_free(&pkt_out);

	/////////// decode ends here ///////


	receiver.closeSock();



	WSACleanup();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}