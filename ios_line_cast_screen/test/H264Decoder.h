#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}

#include <iostream>
#include <vector>

class CH264Decoder
{
public:
	static int h264_extradata_to_annexb(const void* src, int size, void* dst);
	static int h264_slice_to_annexb(const void* src, int size);
public:
	CH264Decoder(void);
	~CH264Decoder(void);

	bool Init();
	void UnInit();
	bool send_extradata(const void* buffer, int buflen);
	bool send_slice(const void* buffer, int buflen);
	bool send_packet(const void* buffer, int buflen);
	bool receive_frame(AVFrame **frame);
	bool yuv2rgb(AVFrame* pFrame, std::vector<unsigned char>& vecRgbData);
protected:
	AVCodec *codec;
	AVCodecParserContext *parser;
	AVCodecContext *c;
	AVFrame *frame;
	AVPacket *pkt;
	struct SwsContext* pScxt;
	bool m_bInited;
};

