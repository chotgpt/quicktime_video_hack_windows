#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gcusb.h"
#include "RW.h"

#include <QPainter>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStringListModel>

#define TABLE_INDEX_NAME    (0) // 名称
#define TABLE_INDEX_SERIAL  (1) // 序列号
#define TABLE_INDEX_STATUS  (2) // 投屏状态
#define TABLE_INDEX_DRIVER  (3) // 驱动
#define TABLE_INDEX_VID     (4) // vid
#define TABLE_INDEX_PID     (5) // pid

#define MAX_TABLE_INDEX     (6)


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //开启控制台
    AllocConsole();
    freopen("CONOUT$", "w+t", stdout);

//    if (RW::isProcessExist(USBMUXD_PROCESS_NAME))
//    {
//        qDebug("请先开启usbmuxd.exe");
//        exit(0);
//    }

    if (!RW::isProcessExist(USBMUXD_PROCESS_NAME))
    {
        MessageBoxW(nullptr, QString("请先开启usbmuxd.exe!").toStdWString().c_str(), QString("提示").toStdWString().c_str(), 0);
        qDebug() << "请先开启usbmuxd.exe";
        exit(0);
    }

//    if (!RW::ExecProcessDetached(USBMUXD_PROCESS_PATH, ""))
//    {
//        qDebug("error");
//        exit(0);
//    }

    tableView_DeviceList_model = new QStandardItemModel();
    ui->tableView_DeviceList->setModel(tableView_DeviceList_model);
    //列数
    tableView_DeviceList_model->setColumnCount(MAX_TABLE_INDEX);
    //列名称和列宽
    tableView_DeviceList_model->setHeaderData(TABLE_INDEX_NAME, Qt::Horizontal, "名称");
    ui->tableView_DeviceList->setColumnWidth(TABLE_INDEX_NAME, 80);
    tableView_DeviceList_model->setHeaderData(TABLE_INDEX_SERIAL, Qt::Horizontal, "序列号");
    ui->tableView_DeviceList->setColumnWidth(TABLE_INDEX_SERIAL, 120);
    tableView_DeviceList_model->setHeaderData(TABLE_INDEX_STATUS, Qt::Horizontal, "状态");
    ui->tableView_DeviceList->setColumnWidth(TABLE_INDEX_STATUS, 70);
    tableView_DeviceList_model->setHeaderData(TABLE_INDEX_DRIVER, Qt::Horizontal, "驱动");
    ui->tableView_DeviceList->setColumnWidth(TABLE_INDEX_DRIVER, 80);
    tableView_DeviceList_model->setHeaderData(TABLE_INDEX_VID, Qt::Horizontal, "vid");
    ui->tableView_DeviceList->setColumnWidth(TABLE_INDEX_VID, 60);
    tableView_DeviceList_model->setHeaderData(TABLE_INDEX_PID, Qt::Horizontal, "pid");
    ui->tableView_DeviceList->setColumnWidth(TABLE_INDEX_PID, 60);

    ui->tableView_DeviceList->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

    QGridLayout *pLayout = new QGridLayout(ui->widget_display_list);//网格布局

    for (unsigned int i = 0; i < MAX_DISPLAY_H; i++)
    {
        for (unsigned int j = 0; j < MAX_DISPLAY_W; j++)
        {
            AirPlaySource* pAirPlay = new AirPlaySource(this);
            pLayout->addWidget(pAirPlay, i, j);
            pLayout->setColumnMinimumWidth(i, 100);
            pLayout->setColumnStretch(i, 1);

            AirPlaySourceList.push_back(pAirPlay);
            printf("\n");
        }
    }

    g_pVideoWindow = new VideoWindow;
    g_pVideoWindow->show();

    on_Btn_RefreshDeviceList_clicked();
}

MainWindow::~MainWindow()
{
    // RW::ExitProcess(USBMUXD_PROCESS_NAME);

    for (unsigned int i = 0; i < AirPlaySourceList.size(); i++)
    {
        AirPlaySource* pAirPlay = AirPlaySourceList[i];
        delete pAirPlay;
    }
    AirPlaySourceList.clear();
    delete ui;
    delete g_pVideoWindow;
}

AirPlaySource* MainWindow::GetNotBindSource() const
{
    unsigned int i = 0;
    AirPlaySource* pAirPlay = nullptr;
    for (i = 0; i <  AirPlaySourceList.size(); i++)
    {
        pAirPlay = AirPlaySourceList[i];
        if(!pAirPlay->isBindDevice())
            break;
    }
    qDebug() << "GetNotBindSource : " << i;
    return pAirPlay;
}

AirPlaySource* MainWindow::GetAirPlayBySerial(const QString &qstr_serial) const
{
    for (unsigned int i = 0; i < AirPlaySourceList.size(); i++)
    {
        AirPlaySource* pAirPlay = AirPlaySourceList[i];
        const char* serial_number = pAirPlay->get_serial_number();
        if(qstr_serial == serial_number)
        {
            return pAirPlay;
        }
    }
    return nullptr;
}

QStringList MainWindow::GetRunningAirplayList() const
{
    QStringList qstrList;


    for (unsigned int i = 0; i < AirPlaySourceList.size(); i++)
    {
        AirPlaySource* pAirPlay = AirPlaySourceList[i];
        if(!pAirPlay->isBindDevice()) continue;
        if(!pAirPlay->isRunning()) continue;
        qstrList.push_back(pAirPlay->get_serial_number());
    }
    return qstrList;
}

//刷新设备列表
void MainWindow::on_Btn_RefreshDeviceList_clicked()
{
    QStringList RunningSerialList = GetRunningAirplayList();
    qDebug() << "RunningSerialList" << RunningSerialList;

    std::vector<UsbDeviceData> UsbDeviceDataList = GCUSB::GetUsbDeviceList_ByVidAndComposite(VID_APPLE, true);
    printf("设备数量[%u]\n", (unsigned int)UsbDeviceDataList.size());
    tableView_DeviceList_model->removeRows(0, tableView_DeviceList_model->rowCount());
    for (unsigned int i = 0; i < (unsigned int)UsbDeviceDataList.size(); i++)
    {
        UsbDeviceData& device = UsbDeviceDataList[i];
        printf("serial_number[%s]\n", device.strSerial.c_str());
        printf("driver_name[%s]\n", device.strDriveName.c_str());
        tableView_DeviceList_model->setItem(i, TABLE_INDEX_NAME, new QStandardItem(device.strDeviceName.c_str()));
        tableView_DeviceList_model->setItem(i, TABLE_INDEX_SERIAL, new QStandardItem(device.strSerial.c_str()));
        tableView_DeviceList_model->setItem(i, TABLE_INDEX_STATUS, new QStandardItem(RunningSerialList.indexOf(device.strSerial.c_str()) == -1 ? "stop" : "running"));
        tableView_DeviceList_model->setItem(i, TABLE_INDEX_DRIVER, new QStandardItem(device.strDriveName.c_str()));
        tableView_DeviceList_model->setItem(i, TABLE_INDEX_VID, new QStandardItem(QString::number((int)device.vid)));
        tableView_DeviceList_model->setItem(i, TABLE_INDEX_PID, new QStandardItem(QString::number((int)device.pid)));
    }
}

//全部开始
void MainWindow::on_Btn_AllStart_clicked()
{
    for(int i = 0; i < tableView_DeviceList_model->rowCount(); i++)
    {
        QString qstr_serial = tableView_DeviceList_model->index(i, TABLE_INDEX_SERIAL).data().toString();
        printf("serial_number  [%s]\n", qstr_serial.toLatin1().data());
        AirPlaySource* pAirPlay = GetAirPlayBySerial(qstr_serial);
        if(!pAirPlay)
        {
            pAirPlay = GetNotBindSource();
            if(!pAirPlay)
            {
                qDebug() << "get pAirPlay error!";
                continue;
            }
        }
        if(pAirPlay->isRunning()) continue;
        pAirPlay->SetDeviceInfo(i, qstr_serial.toLatin1().data());
        pAirPlay->Start();
    }
    on_Btn_RefreshDeviceList_clicked();
}


void MainWindow::on_Btn_AllStop_clicked()
{
    for(int i = 0; i < tableView_DeviceList_model->rowCount(); i++)
    {
        QString qstr_serial = tableView_DeviceList_model->index(i, TABLE_INDEX_SERIAL).data().toString();
        printf("serial_number  [%s]\n", qstr_serial.toLatin1().data());
        AirPlaySource* pAirPlay = GetAirPlayBySerial(qstr_serial);
        if(!pAirPlay)
        {
            pAirPlay = GetNotBindSource();
            if(!pAirPlay)
            {
                qDebug() << "get pAirPlay error!";
                continue;
            }
        }
        if(!pAirPlay->isRunning()) continue;

        pAirPlay->Stop();
        qDebug() << "Stop Success" << qstr_serial;
    }
    on_Btn_RefreshDeviceList_clicked();
}

void MainWindow::on_AllInstallDriverLibusb0_clicked()
{
    for(int i = 0; i < tableView_DeviceList_model->rowCount(); i++)
    {
        QString qstr_serial = tableView_DeviceList_model->index(i, TABLE_INDEX_SERIAL).data().toString();
        printf("serial_number  [%s]\n", qstr_serial.toLatin1().data());
        AirPlaySource* pAirPlay = GetAirPlayBySerial(qstr_serial);
        if(!pAirPlay)
        {
            pAirPlay = GetNotBindSource();
            if(!pAirPlay)
            {
                qDebug() << "get pAirPlay error!";
                continue;
            }
        }
        if(pAirPlay->isRunning()) continue;

        QString qstr_vid = tableView_DeviceList_model->index(i, TABLE_INDEX_VID).data().toString();
        QString qstr_pid = tableView_DeviceList_model->index(i, TABLE_INDEX_PID).data().toString();

        qDebug() << "InstallDriver Begin..."<< i << qstr_serial << qstr_vid << qstr_pid;

        bool bRetCode = GCUSB::InstallDriver((unsigned short)qstr_vid.toInt(), (unsigned short)qstr_pid.toInt(), qstr_serial.toStdString(), "LIBUSB0");
        if(!bRetCode)
        {
            qDebug() << "InstallDriver error!"<< i << qstr_serial << qstr_vid << qstr_pid;
            continue;
        }
        qDebug() << "InstallDriver success!"<< i << qstr_serial << qstr_vid << qstr_pid;
    }
    on_Btn_RefreshDeviceList_clicked();
}

void MainWindow::on_AllInstallDriverLibusb0Filter_clicked()
{
    for(int i = 0; i < tableView_DeviceList_model->rowCount(); i++)
    {
        QString qstr_serial = tableView_DeviceList_model->index(i, TABLE_INDEX_SERIAL).data().toString();
        printf("serial_number  [%s]\n", qstr_serial.toLatin1().data());
        AirPlaySource* pAirPlay = GetAirPlayBySerial(qstr_serial);
        if(!pAirPlay)
        {
            pAirPlay = GetNotBindSource();
            if(!pAirPlay)
            {
                qDebug() << "get pAirPlay error!";
                continue;
            }
        }
        if(pAirPlay->isRunning()) continue;

        QString qstr_vid = tableView_DeviceList_model->index(i, TABLE_INDEX_VID).data().toString();
        QString qstr_pid = tableView_DeviceList_model->index(i, TABLE_INDEX_PID).data().toString();

        qDebug() << "InstallDriver Begin..."<< i << qstr_serial << qstr_vid << qstr_pid;

        bool bRetCode = GCUSB::InstallDriver((unsigned short)qstr_vid.toInt(), (unsigned short)qstr_pid.toInt(), qstr_serial.toStdString(), "LIBUSB0_FILTER");
        if(!bRetCode)
        {
            qDebug() << "InstallDriver error!"<< i << qstr_serial << qstr_vid << qstr_pid;
            continue;
        }
        qDebug() << "InstallDriver success!"<< i << qstr_serial << qstr_vid << qstr_pid;
    }
    on_Btn_RefreshDeviceList_clicked();
}


void MainWindow::on_tableView_DeviceList_clicked(const QModelIndex &index)
{
    // QStandardItem* item = tableView_DeviceList_model->itemFromIndex(index);

    qDebug() << "row" << index.row();

    QString qstr_serial = tableView_DeviceList_model->index(index.row(), TABLE_INDEX_SERIAL).data().toString();
    printf("serial_number  [%s]\n", qstr_serial.toLatin1().data());

    AirPlaySource* pAirPlay = GetAirPlayBySerial(qstr_serial);
    if(!pAirPlay)
    {
        qDebug() << "get pAirPlay error!";
    }

    g_pVideoWindow->SetFullWindowSource(pAirPlay);

    qDebug() << "SetFullWindowSource success!"<< index.row() << qstr_serial;
}

