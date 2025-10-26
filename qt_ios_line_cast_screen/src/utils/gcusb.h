#ifndef GCUSB_H
#define GCUSB_H

#include <iostream>
#include <vector>

struct UsbDeviceData
{
	bool isCompositeParent;	//�Ƿ��Ǹ����豸�ĸ��ڵ�
	bool isComposite;		//�Ƿ��Ǹ����豸���ӽڵ�
	std::string strDeviceName;	//�豸��
	unsigned short vid;		//����ID (ƻ��Ϊ0x05AC)
	std::string strVid;
	unsigned short pid;		//�����Զ�����豸ID
	std::string strPid;
	std::string strSerial;		//���к�
	std::string strDriveName;	//������
	std::string strLocationPath;//λ��·��(Port_#0002.Hub_#0001)
	std::string strLocationInfo;//λ����Ϣ()
	std::string strCompatibleID;//����ID
};

class GCUSB
{
public:
	//ȡ����usb�豸�б�
	static std::vector<UsbDeviceData> GetUsbDeviceList();
	//ȡ����usb�豸�б�(������)
	static std::vector<UsbDeviceData> GetUsbDeviceList_ByVidAndComposite(unsigned short vid, bool isCompositeParent);
    // ��װlibusb����
    // crstrDriverType: WINUSB, LIBUSB0, LIBUSB0_FILTER, LIBUSBK
    static bool InstallDriver(unsigned short vid, unsigned short pid, std::string strSerial, const std::string& crstrDriverType);
	//ж��libusb����
	//��װlibusbɸѡ������
	//ж��libusbɸѡ������

};

#endif // GCUSB_H
