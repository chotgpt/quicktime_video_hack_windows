#ifndef VIDEOWINDOW_H
#define VIDEOWINDOW_H

#include "AirPlaySource.h"

#include <QWidget>
#include <QTimer>
#include <queue>

namespace Ui {
class VideoWindow;
}

class VideoWindow : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWindow(QWidget *parent = nullptr);
    ~VideoWindow();
    void SetFullWindowSource(AirPlaySource* pAirPlaySource);
    AirPlaySource* GetFullWindowSource() const;

private:
    Ui::VideoWindow *ui;

protected:
    void paintEvent(QPaintEvent *event);
    QImage  m_UpdateImage;
    int     m_nImageWidth   = 0;    // 分辨率
    int     m_nImageHeight  = 0;   // 分辨率
    AirPlaySource*  m_pFullWindowAirPlaySource = nullptr;  // 全屏的窗口
private slots:
    void slotGetOneFrame(const QImage& img);
signals:
    void sig_GetOneFrame(const QImage& img); //获取到一帧图像 就发送此信号
};

extern VideoWindow *g_pVideoWindow;

#endif // VIDEOWINDOW_H
