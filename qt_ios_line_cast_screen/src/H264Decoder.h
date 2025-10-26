#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}

#include <iostream>
#include <mutex>
#include <vector>
#include <QObject>
#include <QThread>
#include <QImage>

enum enumH264Type
{
    EHT_EXTRADATA = 1,
    EHT_SLICE = 2,
};

typedef struct
{
    void(*video_rgb32_output)(void* user_data, const QImage& rgb_image);
}h264_decoder_callbacks_t;

class CH264Decoder : public QThread
{
    Q_OBJECT
public:
	CH264Decoder(void);
	~CH264Decoder(void);

    static bool GlobalInit(const h264_decoder_callbacks_t* pCallbacks);
    bool Init();
    void UnInit();
    void Start();
    void Stop();
    void SetUserData(void* user_input_data);
    bool input_h264_vedio(const void* buffer, int buflen, enum enumH264Type type);

private:
    int h264_extradata_to_annexb(const void* src, int size, void* dst);
    int h264_slice_to_annexb(const void* src, int size);
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
    bool m_bInited;
    bool m_bRunning;
protected:
    void run();
private:
    std::mutex m_vmutex;
    AVFrame *m_vframe;
    unsigned char* m_slice_buffer;
    void* user_input_data;
    static h264_decoder_callbacks_t g_FuncCallback;
    static bool g_isInit;
};

