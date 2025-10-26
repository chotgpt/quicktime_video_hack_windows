#include "H264Decoder.h"
#include "ffmpegcommon.h"
#include <QDebug>


h264_decoder_callbacks_t CH264Decoder::g_FuncCallback;
bool CH264Decoder::g_isInit;

static QImage getQImageFromFrame(const AVFrame* pFrame)
{
    // first convert the input AVFrame to the desired format

    SwsContext* img_convert_ctx = sws_getContext(
                                     pFrame->width,
                                     pFrame->height,
                                     AV_PIX_FMT_YUV420P, //(AVPixelFormat)pFrame->format,
                                     pFrame->width,
                                     pFrame->height,
                                     AV_PIX_FMT_RGB24,
                                     SWS_BICUBIC, NULL, NULL, NULL);
    if(!img_convert_ctx){
        qDebug() << "Failed to create sws context";
        return QImage();
    }

    // prepare line sizes structure as sws_scale expects
    int rgb_linesizes[8] = {0};
    rgb_linesizes[0] = 3*pFrame->width;

    // prepare char buffer in array, as sws_scale expects
    unsigned char* rgbData[8];
    int imgBytesSyze = 3*pFrame->height*pFrame->width;
    // as explained above, we need to alloc extra 64 bytes
    rgbData[0] = (unsigned char *)malloc(imgBytesSyze+64);
    if(!rgbData[0]){
        qDebug() << "Error allocating buffer for frame conversion";
        free(rgbData[0]);
        sws_freeContext(img_convert_ctx);
        return QImage();
    }
    if(sws_scale(img_convert_ctx,
                pFrame->data,
                pFrame->linesize, 0,
                pFrame->height,
                rgbData,
                rgb_linesizes)
            != pFrame->height){
        qDebug() << "Error changing frame color range";
        free(rgbData[0]);
        sws_freeContext(img_convert_ctx);
        return QImage();
    }

    // then create QImage and copy converted frame data into it

    QImage image(pFrame->width,
                 pFrame->height,
                 QImage::Format_RGB888);

    for(int y=0; y<pFrame->height; y++){
        memcpy(image.scanLine(y), rgbData[0]+y*3*pFrame->width, 3*pFrame->width);
    }

    free(rgbData[0]);
    sws_freeContext(img_convert_ctx);
    return image;
}

bool CH264Decoder::yuv2rgb(AVFrame* pFrame, std::vector<unsigned char>& vecRgbData)
{
    struct SwsContext* pScxt = nullptr;
    int width = pFrame->width;
    int height = pFrame->height;

    qDebug() << "width" << width;
    qDebug() << "height" << height;

	vecRgbData.clear();

    if (width == 0 || height == 0) return false;

    pScxt = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB32, SWS_POINT, NULL, NULL, NULL);
    if (!pScxt) return false;


    AVFrame* pFrameRGB = av_frame_alloc();
    int nRGBFrameSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, width, height, 1);
    uint8_t* pszRGBBuffer = (uint8_t*)av_malloc(nRGBFrameSize + 64);
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, pszRGBBuffer, AV_PIX_FMT_RGB32, width, height, 1);

    sws_scale(pScxt, pFrame->data, pFrame->linesize, 0, height, pFrameRGB->data, pFrameRGB->linesize);
	vecRgbData.resize(nRGBFrameSize);
	memcpy(vecRgbData.data(), pszRGBBuffer, nRGBFrameSize);
	av_free(pszRGBBuffer);
	av_free(pFrameRGB);

    sws_freeContext(pScxt);
    pScxt = nullptr;

	return true;
}

int CH264Decoder::h264_extradata_to_annexb(const void* src, int size, void* dst)
{
	const int padding = 32;
	uint16_t unit_size;
	uint32_t total_size = 0;
    uint8_t* out = (uint8_t*)dst, unit_nb, sps_done = 0, sps_seen = 0, pps_seen = 0;
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
    m_vframe = NULL;
	codec = NULL;
	parser = NULL;
	c = NULL;
	frame = NULL;
    pkt = NULL;
	m_bInited = false;
    m_bRunning = false;
    user_input_data = NULL;
    m_slice_buffer = NULL;
}

CH264Decoder::~CH264Decoder(void)
{
	UnInit();
}

bool CH264Decoder::GlobalInit(const h264_decoder_callbacks_t* pCallbacks)
{
    if (!pCallbacks) return false;
    if (CH264Decoder::g_isInit) return true;
    memcpy_s(&CH264Decoder::g_FuncCallback, sizeof(h264_decoder_callbacks_t), pCallbacks, sizeof(h264_decoder_callbacks_t));
    CH264Decoder::g_isInit = true;
    return true;
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
    m_slice_buffer = new unsigned char[1024*1024*5];

    m_bInited = true;
	return true;
}

void CH264Decoder::UnInit()
{
    this->Stop();
    m_bInited = false;
    m_bRunning = false;
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
    if(m_slice_buffer)
    {
        delete []m_slice_buffer;
        m_slice_buffer = NULL;
    }
}

void CH264Decoder::run()
{
    QImage image;

    AVFrame* frame = NULL;
    while(m_bRunning)
    {
        this->m_vmutex.lock();
        if(m_vframe)
        {
            frame = dup_vframe_cache(frame, m_vframe);

            if (false)
            {
                std::vector<unsigned char> vecRgbData;
                this->yuv2rgb(frame, vecRgbData);
                QImage tmpImg((uchar *)vecRgbData.data(), frame->width, frame->height, QImage::Format_RGB32);
                image = tmpImg.copy();
            }
            else
            {
                image = getQImageFromFrame(frame);
            }

            av_frame_free(&m_vframe);
            m_vframe = NULL;
        }
        this->m_vmutex.unlock();
        CH264Decoder::g_FuncCallback.video_rgb32_output(this->user_input_data, image);
        msleep(1000 / 60);
    }
}

void CH264Decoder::Start()
{
    m_bRunning = true;
    this->start();
}

void CH264Decoder::Stop()
{
    m_bRunning = false;
}

void CH264Decoder::SetUserData(void* user_input_data)
{
    this->user_input_data = user_input_data;
}

bool CH264Decoder::input_h264_vedio(const void* buffer, int buflen, enum enumH264Type type)
{
    switch (type)
    {
    case EHT_EXTRADATA:
        if(!this->send_extradata(buffer, buflen)) return false;
        break;
    case EHT_SLICE:
        if(!this->send_slice(buffer, buflen)) return false;
        break;
    }

    this->m_vmutex.lock();
    AVFrame* frame = NULL;
    while (this->receive_frame(&frame) && frame)
    {
        m_vframe = dup_vframe_cache(m_vframe, frame);
    }
    this->m_vmutex.unlock();
    return true;
}




bool CH264Decoder::send_extradata(const void* buffer, int buflen)
{
    qDebug() << "h264_extradata_to_annexb";
    unsigned char annexb[1024 * 10];
	int headsize = h264_extradata_to_annexb(buffer, buflen, annexb);
	return this->send_packet(annexb, headsize);
}

bool CH264Decoder::send_slice(const void* buffer, int buflen)
{
    memcpy_s(this->m_slice_buffer, buflen, buffer, buflen);
    int headsize = h264_slice_to_annexb(this->m_slice_buffer, buflen);
    return this->send_packet(this->m_slice_buffer, headsize);
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
