#include "videowindow.h"
#include "ui_videowindow.h"

#include <QPainter>
#include <QDebug>
#include <QTimer>

VideoWindow *g_pVideoWindow = nullptr;

VideoWindow::VideoWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoWindow)
{
    ui->setupUi(this);
    connect(this, SIGNAL(sig_GetOneFrame(QImage)), this, SLOT(slotGetOneFrame(QImage)));
}

VideoWindow::~VideoWindow()
{
    delete ui;
}

void VideoWindow::SetFullWindowSource(AirPlaySource* pAirPlaySource)
{
    m_pFullWindowAirPlaySource = pAirPlaySource;
    hide();
    show();
}

AirPlaySource* VideoWindow::GetFullWindowSource() const
{
    return m_pFullWindowAirPlaySource;
}

void VideoWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (m_UpdateImage.width() <= 0) return;

    //窗口画成黑色
    painter.setBrush(Qt::yellow);
    painter.drawRect(0, 0, this->width(), this->height());

    //将图像按比例缩放成和窗口一样大小
    QImage img = m_UpdateImage.scaled(this->size().width(), this->size().height(), Qt::KeepAspectRatio, Qt::SmoothTransformation); // FastTransformation

    painter.drawImage(QPoint(0, 0), img); //画出图像
}

void VideoWindow::slotGetOneFrame(const QImage& img)
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

        printf("VideoWindow %d x %d\n", m_nImageWidth, m_nImageHeight);
    }

    m_UpdateImage = img;

    update(); //调用update将执行 paintEvent函数
}
