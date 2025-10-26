#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "airplaysource.h"

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QStandardItemModel>
#include "videowindow.h"

//#define MAX_DISPLAY (3 * 4)
#define MAX_DISPLAY_W (2)   // 横向2个
#define MAX_DISPLAY_H (1)   // 纵向1个
#define USBMUXD_PROCESS_NAME    "usbmuxd.exe"
#define USBMUXD_PROCESS_PATH    "Frameworks\\usbmuxd.exe"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_Btn_RefreshDeviceList_clicked();

    void on_Btn_AllStart_clicked();

    void on_Btn_AllStop_clicked();

    void on_AllInstallDriverLibusb0_clicked();

    void on_AllInstallDriverLibusb0Filter_clicked();

    void on_tableView_DeviceList_clicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *tableView_DeviceList_model;
    QVector<AirPlaySource*> AirPlaySourceList;
    AirPlaySource* GetNotBindSource() const;
    AirPlaySource* GetAirPlayBySerial(const QString &qstr_serial) const;
    QStringList GetRunningAirplayList() const;  //取运行中的所有设备，返回序列号
};
#endif // MAINWINDOW_H
