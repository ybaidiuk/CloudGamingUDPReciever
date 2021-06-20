#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <winsock2.h>
#include <SDL.h>
#include "UDPReceive.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}

#define STB_IMAGE_IMPLEMENTATION 
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define SDL_MAIN_HANDLED



const char* outfilename = "screenshot";
AVFrame* picture;
AVCodecContext* avCodecCtx;
AVCodecParserContext* parser;
const AVCodec* avCodec;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Rect r;

void initWSAndAvCodecCtxAndAvParser();
int initSDL(AVCodecContext* ctx);
void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const char* filename);

int main(int argc, char* argv[]) {

	initWSAndAvCodecCtxAndAvParser();

	if (initSDL(avCodecCtx) < 0) {
		fprintf(stderr, "Error while initialising SDL\n");
		return 0;
	}

	UDPReceive receiver = UDPReceive();
	receiver.init(23042);



	while (true) {
		char buf[650000];
		ZeroMemory(buf, sizeof(buf));

		int bufLength = 0;
		double ptoime = clock() / (double)CLOCKS_PER_SEC;
		int out = receiver.receive(buf, &ptoime);
		if (out == -1) {
			continue;
			printf("1 pkt was lost");
		}

		char outname[200];

		uint8_t* received_data = (uint8_t*)buf;
		AVPacket* pkt_out = av_packet_alloc();
		if (!pkt_out)
			exit(1);


		size_t data_size = out;
		if (!data_size)
			break;

		uint8_t* data = received_data;
		while (data_size > 0) {
			int ret = av_parser_parse2(parser, avCodecCtx, &pkt_out->data, &pkt_out->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			std::cout << "ret: " << ret << "\n";

			if (ret < 0) {
				fprintf(stderr, "Error while parsing\n");
				exit(1);
			}

			data += ret;
			data_size -= ret;
			std::cout << pkt_out->size << "pkt_out size\n";
			if (pkt_out->size) {
				printf("Outname %s\n", outname);
				decode(avCodecCtx, picture, pkt_out, outname);
			} else {
				printf("pkt_out size is null!");
			}
		}


	}

	decode(avCodecCtx, picture, NULL, outfilename);

	av_parser_close(parser);
	avcodec_free_context(&avCodecCtx);
	av_frame_free(&picture);

	receiver.closeSock();

	WSACleanup();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void initWSAndAvCodecCtxAndAvParser() {
	WSADATA wsaData;
	int WSStartCode = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (WSStartCode != 0) {
		printf("WSAStartup failed: %d\n", WSStartCode);
		return;
	}

	avCodec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
	if (!avCodec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	parser = av_parser_init(avCodec->id);
	parser->flags = PARSER_FLAG_COMPLETE_FRAMES;
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	avCodecCtx = avcodec_alloc_context3(avCodec);

	if (avcodec_open2(avCodecCtx, avCodec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}
	avCodecCtx->width = 800;
	avCodecCtx->height = 600;
	picture = av_frame_alloc();
}

int initSDL(AVCodecContext* ctx) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init init error");
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

void display_frame_via_SDL(AVFrame* frame, AVCodecContext* ctx) {

	SDL_UpdateYUVTexture(texture, &r, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
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

		snprintf(buf, sizeof(buf), filename, dec_ctx->frame_number);
		display_frame_via_SDL(frame, dec_ctx);
	}
}
