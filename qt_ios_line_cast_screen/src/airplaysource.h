#ifndef AIRPLAYSOURCE_H
#define AIRPLAYSOURCE_H

#include "H264Decoder.h"
#include "SoundPlay.h"
#include "QuickTime.h"
#include <QWidget>
#include <queue>

enum class emAirPlayStatus
{
    unknown, connect, disconnect
};

class AirPlaySource : public QWidget
{
    Q_OBJECT
public:
    AirPlaySource(QWidget *parent = nullptr);
    ~AirPlaySource();
    void SetDeviceInfo(unsigned int device_index, const char* serial_number);
    void Start();
    void Stop();
    bool isBindDevice() const;
    bool isRunning() const;
    unsigned int get_device_index() const;
    const char* get_serial_number() const;
public:
    static void audio_init(void* user_data, int bits, int channels, int samplerate, int isaudio);
    static void audio_process(void* user_data, unsigned char* buffer, int length);
    static void video_connect(void* user_data);
    static void video_dis_connect(void* user_data);
    static void video_h264_extradata(void* user_data, unsigned char* buffer, unsigned int length);
    static void video_h264_slice(void* user_data, unsigned char* buffer, unsigned int length);
    static void message(void* user_data, const char* msg);
    static void log(void* user_data, const char* text);

    static void video_rgb32_output(void* user_data, const QImage &rgb_image);
protected:
    SoundPlay*      m_pSoundPlay    = nullptr;
    CH264Decoder*   m_pH264Decoder  = nullptr;
    QuickTime*      m_pQuickTime    = nullptr;
    unsigned int device_index;	//设备ID
    char serial_number[64];		//设备序列号

protected:
    void paintEvent(QPaintEvent *event);
    emAirPlayStatus status;
    QImage  m_UpdateImage;
    int     m_nImageWidth   = 0;    // 分辨率
    int     m_nImageHeight  = 0;   // 分辨率
private slots:
    void slotGetOneFrame(const QImage& img);
signals:
    void sig_GetOneFrame(const QImage& img); //获取到一帧图像 就发送此信号
};

#endif // AIRPLAYSOURCE_H
