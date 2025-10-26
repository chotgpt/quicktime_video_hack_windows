#include "H264Decoder.h"


bool CH264Decoder::yuv2rgb(AVFrame* pFrame, std::vector<unsigned char>& vecRgbData)
{
	int width = pFrame->width;
	int height = pFrame->height;
	vecRgbData.clear();
	if (width == 0 || height == 0) return false;
	if (pScxt == NULL)
	{
		pScxt = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB32, SWS_POINT, NULL, NULL, NULL);
	}

	AVFrame* pFrameRGB = av_frame_alloc();
	int nRGBFrameSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, width, height, 1);
	uint8_t* pszRGBBuffer = (uint8_t*)av_malloc(nRGBFrameSize);
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, pszRGBBuffer, AV_PIX_FMT_RGB32, pFrame->width, pFrame->height, 1);

	sws_scale(pScxt, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0, height, pFrameRGB->data, pFrameRGB->linesize);
	vecRgbData.resize(nRGBFrameSize);
	memcpy(vecRgbData.data(), pszRGBBuffer, nRGBFrameSize);
	av_free(pszRGBBuffer);
	av_free(pFrameRGB);
	return true;
}

int CH264Decoder::h264_extradata_to_annexb(const void* src, int size, void* dst)
{
	const int padding = 32;
	uint16_t unit_size;
	uint32_t total_size = 0;
	uint8_t* out = (uint8_t*)dst, unit_nb, sps_done = 0,
		sps_seen = 0, pps_seen = 0;
	const uint8_t* extradata = (uint8_t*)src + 4;
	static const uint8_t nalu_header[4] = { 0, 0, 0, 1 };
	int length_size = (*extradata++ & 0x3) + 1; // retrieve length coded size

	int sps_offset = -1;
	int pps_offset = -1;

	/* retrieve sps and pps unit(s) */
	unit_nb = *extradata++ & 0x1f; /* number of sps unit(s) */
	if (!unit_nb) {
		goto pps;
	}
	else {
		sps_offset = 0;
		sps_seen = 1;
	}

	while (unit_nb--) {
		unit_size = uint16_t(*extradata) << 8 | *(extradata + 1);
		total_size += unit_size + 4;
		if (total_size > INT_MAX - padding) {
			return -1;
		}
		if (extradata + 2 + unit_size > (uint8_t*)src + size) {
			return -1;
		}

		memcpy(out + total_size - unit_size - 4, nalu_header, 4);
		memcpy(out + total_size - unit_size, extradata + 2, unit_size);
		extradata += 2 + unit_size;
	pps:
		if (!unit_nb && !sps_done++) {
			unit_nb = *extradata++; /* number of pps unit(s) */
			if (unit_nb) {
				pps_offset = total_size;
				pps_seen = 1;
			}
		}
	}

	if (out)
		memset(out + total_size, 0, padding);
	return total_size;
}

int CH264Decoder::h264_slice_to_annexb(const void* src, int size)
{
	int rLen = 0;
	unsigned char* data = (unsigned char*)src + rLen;
	while (rLen < size)
	{
		rLen += 4;
		rLen += (((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3]);
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 1;
		data = (unsigned char*)src + rLen;
	}
	return size;
}

CH264Decoder::CH264Decoder(void)
{
	codec = NULL;
	parser = NULL;
	c = NULL;
	frame = NULL;
	pkt = NULL;
	pScxt = NULL;
	m_bInited = false;
}


CH264Decoder::~CH264Decoder(void)
{
	UnInit();
}

bool CH264Decoder::Init()
{
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        return false;
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        return false;
    }

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
         return false;
    }

	frame = av_frame_alloc();
	pkt = av_packet_alloc();

	m_bInited = true;
	return true;
}

void CH264Decoder::UnInit()
{
	av_frame_free(&frame);
	av_packet_free(&pkt);
	if (c)
	{
		avcodec_close(c);
		avcodec_free_context(&c);
	}
	if (parser)
	{
		av_parser_close(parser);
		parser = NULL;
	}
}

bool CH264Decoder::send_extradata(const void* buffer, int buflen)
{
	unsigned char annexb[1024];
	int headsize = h264_extradata_to_annexb(buffer, buflen, annexb);
	return this->send_packet(annexb, headsize);
}

bool CH264Decoder::send_slice(const void* buffer, int buflen)
{
	unsigned char* slice = (unsigned char*)malloc(buflen);
	if (!slice) return false;
	memcpy_s(slice, buflen, buffer, buflen);
	int headsize = h264_slice_to_annexb(slice, buflen);
	bool bRet = this->send_packet(slice, headsize);
	free(slice);
	return bRet;
}

bool CH264Decoder::send_packet( const void *buffer, int buflen )
{
	if(!m_bInited)
		return false;
	uint8_t *data = (uint8_t *)buffer;
	int data_size = buflen;
	while (data_size > 0) {
		int ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
			data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			return false;
		}
		data      += ret;
		data_size -= ret;

		if (pkt->size)
		{
			ret = avcodec_send_packet(c, pkt);
			if (ret < 0) {
				return false;
			}
			else
			{
				continue;
			}
		}
	}
	return true;
}

bool CH264Decoder::receive_frame(AVFrame **frame_out)
{
    int ret = avcodec_receive_frame(c, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
		*frame_out = NULL;
		return  true;
	}
    else if (ret < 0) {
        return false;
    }

	*frame_out = frame;
	return  true;
}
