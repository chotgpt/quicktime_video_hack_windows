#include "airplaysource.h"
#include "videowindow.h"

#include <QImage>
#include <QPainter>
#include <QDebug>

#define INIT_SIZE_WIDTH (1024)
#define INIT_SIZE_HEIGHT (768)
//#define INIT_SIZE_WIDTH (300)
//#define INIT_SIZE_HEIGHT (400)

AirPlaySource::AirPlaySource(QWidget *parent) : QWidget(parent)
{
    this->status = emAirPlayStatus::unknown;
    this->device_index = -1;
    memset(this->serial_number, 0, sizeof(this->serial_number));

    this->setMinimumSize(INIT_SIZE_WIDTH, INIT_SIZE_HEIGHT);
    // this->setMaximumSize(INIT_SIZE_WIDTH, INIT_SIZE_HEIGHT);
    this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    this->show();
    connect(this, SIGNAL(sig_GetOneFrame(QImage)), this, SLOT(slotGetOneFrame(QImage)));

    quick_time_callbacks_t quick_time_callbacks;
    quick_time_callbacks.audio_init = audio_init;
    quick_time_callbacks.audio_process = audio_process;
    quick_time_callbacks.video_connect = video_connect;
    quick_time_callbacks.video_dis_connect = video_dis_connect;
    quick_time_callbacks.video_h264_extradata = video_h264_extradata;
    quick_time_callbacks.video_h264_slice = video_h264_slice;
    quick_time_callbacks.message = message;
    quick_time_callbacks.log = log;
    QuickTime::Init(&quick_time_callbacks);

    h264_decoder_callbacks_t h264_decoder_callbacks;
    h264_decoder_callbacks.video_rgb32_output = video_rgb32_output;
    CH264Decoder::GlobalInit(&h264_decoder_callbacks);

    this->m_pQuickTime = new QuickTime;

    this->m_pH264Decoder = new CH264Decoder;
    this->m_pH264Decoder->Init();

    m_pSoundPlay = new SoundPlay;
    m_pSoundPlay->Init();
}

AirPlaySource::~AirPlaySource()
{
    if(m_pSoundPlay)
    {
        m_pSoundPlay->UnInit();
        delete m_pSoundPlay;
        m_pSoundPlay = nullptr;
    }
    if(m_pH264Decoder)
    {
        m_pH264Decoder->UnInit();
        delete m_pH264Decoder;
        m_pH264Decoder = nullptr;
    }
    if(m_pQuickTime)
    {
        delete m_pQuickTime;
        m_pQuickTime = nullptr;
    }
}

void AirPlaySource::SetDeviceInfo(unsigned int device_index, const char* serial_number)
{
    assert(serial_number);
    this->device_index = device_index;
    strcpy_s(this->serial_number, sizeof(this->serial_number), serial_number);
}

void AirPlaySource::Start()
{
    this->m_pQuickTime->SetUserData(this);
    this->m_pQuickTime->Start(serial_number);
    this->m_pH264Decoder->SetUserData(this);
    this->m_pH264Decoder->Start();
}

void AirPlaySource::Stop()
{
    this->m_pH264Decoder->Stop();
    this->m_pQuickTime->Stop();
}

bool AirPlaySource::isBindDevice() const
{
    if(this->device_index != -1 && this->serial_number[0] != '\0')
    {
        return true;
    }
    return false;
}

bool AirPlaySource::isRunning() const
{
     if(this->m_pQuickTime->GetRunningState() == EM_RUN_STATE_STOP)
         return false;
     return true;
}

unsigned int AirPlaySource::get_device_index() const
{
    return this->device_index;
}

const char* AirPlaySource::get_serial_number() const
{
    return this->serial_number;
}

void AirPlaySource::audio_init(void* user_data, int bits, int channels, int samplerate, int isaudio)
{
//    static int count = 0;
//    if (count++ % 60 == 0)
//        printf("user_data[%u|%s] %s: %d|%d|%d|%d\n", user_data->device_index, user_data->serial_number, __FUNCTION__, bits, channels, samplerate, isaudio);

    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    m_AirPlaySource->m_pSoundPlay->SetFormat(channels, samplerate);
    m_AirPlaySource->m_pSoundPlay->PauseAudioDevice(0);
}

void AirPlaySource::audio_process(void* user_data, unsigned char* buffer, int length)
{
//    static int count = 0;
//    if (count++ % 60 == 0)
//        printf("user_data[%u|%s] %s: %d\n", user_data->device_index, user_data->serial_number, __FUNCTION__, length);
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;

    if (g_pVideoWindow->GetFullWindowSource() == m_AirPlaySource)
    {
        m_AirPlaySource->m_pSoundPlay->PushData(buffer, length);
    }
    m_AirPlaySource->m_pSoundPlay->PauseAudioDevice(0);
}

void AirPlaySource::video_connect(void* user_data)
{
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    m_AirPlaySource->status = emAirPlayStatus::connect;
}

void AirPlaySource::video_dis_connect(void* user_data)
{
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    m_AirPlaySource->status = emAirPlayStatus::disconnect;
    emit m_AirPlaySource->sig_GetOneFrame(QImage());
}

void AirPlaySource::video_h264_extradata(void* user_data, unsigned char* buffer, unsigned int length)
{
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    CH264Decoder* m_pH264Decoder = (CH264Decoder*)m_AirPlaySource->m_pH264Decoder;
    m_pH264Decoder->input_h264_vedio(buffer, length, EHT_EXTRADATA);
}

void AirPlaySource::video_h264_slice(void* user_data, unsigned char* buffer, unsigned int length)
{
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    CH264Decoder* m_pH264Decoder = (CH264Decoder*)m_AirPlaySource->m_pH264Decoder;
    m_pH264Decoder->input_h264_vedio(buffer, length, EHT_SLICE);
}

void AirPlaySource::message(void* user_data, const char* msg)
{
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    printf("user_data[%u|%s] %s: text[%s]\n", m_AirPlaySource->device_index, m_AirPlaySource->serial_number, __FUNCTION__, msg);
}

void AirPlaySource::log(void* user_data, const char* text)
{
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    printf("user_data[%u|%s] %s: text[%s]\n", m_AirPlaySource->device_index, m_AirPlaySource->serial_number, __FUNCTION__, text);
}

void AirPlaySource::video_rgb32_output(void* user_data, const QImage& rgb_image)
{
    AirPlaySource* m_AirPlaySource = (AirPlaySource*)user_data;
    emit m_AirPlaySource->sig_GetOneFrame(rgb_image);

    if (g_pVideoWindow->GetFullWindowSource() == m_AirPlaySource)
    {
        emit g_pVideoWindow->sig_GetOneFrame(rgb_image);
    }
}


void AirPlaySource::paintEvent(QPaintEvent *event)
{
    //this->setGeometry(mImage.width() * device_index, 0, mImage.width(), mImage.height());
    //this->setFixedSize(mImage.width(), mImage.height());

    QPainter painter(this);

    switch (this->status) {
    case emAirPlayStatus::connect:
        break;
    case emAirPlayStatus::unknown:
        //初始为黑色
        painter.setBrush(Qt::yellow);
        painter.drawRect(0, 0, this->width(), this->height());
        return;
    case emAirPlayStatus::disconnect:
        //窗口画成黄色表示掉线
        painter.setBrush(Qt::yellow);
        painter.drawRect(0, 0, this->width(), this->height());
        return;
    default:
        //初始为黑色
        painter.setBrush(Qt::yellow);
        painter.drawRect(0, 0, this->width(), this->height());
        return;
        break;
    }

    //窗口画成黑色
    painter.setBrush(Qt::yellow);
    painter.drawRect(0, 0, this->width(), this->height());

    if (m_UpdateImage.width() <= 0) return;

    ///将图像按比例缩放成和窗口一样大小
    QImage img = m_UpdateImage.scaled(this->size().width(), this->size().height(), Qt::KeepAspectRatio, Qt::SmoothTransformation); // FastTransformation
    painter.drawImage(QPoint(0, 0), img); //画出图像
}

void AirPlaySource::slotGetOneFrame(const QImage& img)
{
    static int nActiveLoop = 0;
    nActiveLoop++;

    int nNewWidth = img.width();
    int nNewHeight = img.height();

    // qDebug() << "nNewWidth" << nNewWidth;
    // qDebug() << "nNewHeight" << nNewHeight;

    if (m_nImageWidth != nNewWidth || m_nImageHeight != nNewHeight)
    {
        m_nImageWidth = nNewWidth;
        m_nImageHeight = nNewHeight;

        printf("airplaysource %d x %d\n", m_nImageWidth, m_nImageHeight);
    }
    m_UpdateImage = img;
    update();
}
