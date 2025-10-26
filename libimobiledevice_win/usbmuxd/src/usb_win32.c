#ifdef WIN32
#include "usb_win32.h"
#include <stdint.h>
#include <lusb0_usb.h>
#include "log.h"
#include "usb.h"
#include <Windows.h>

usb_dev_handle *usb_win32_open(const char serial[]);

// Used to protect the list of busses and devices, parallel access is not allowed on
// those objects.
CRITICAL_SECTION usb_lock;

void usb_win32_init()
{
	usb_init();

	// Initialize the log level
	int libusb_verbose = usb_get_log_level();

	if (libusb_verbose > 0) {
		usb_set_debug(libusb_verbose);
	}

	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */

	// Intialize the critical section
	InitializeCriticalSection(&usb_lock);
}

int usb_win32_get_configuration(const char serial[], uint8_t *configuration)
{
	usbmuxd_log(LL_INFO, "Getting the configuration for device %s using libusb-win32", serial);

	usb_dev_handle* device = usb_win32_open(serial);

	if (device == NULL) {
		usbmuxd_log(LL_INFO, "Could not find the device %s using libusb-win32", serial);
		return -1;
	}

	byte config = -1;
	int res = usb_control_msg(device, USB_RECIP_DEVICE | USB_ENDPOINT_IN, USB_REQ_GET_CONFIGURATION, 0, 0, &config, 1, 5000 /* LIBUSB_DEFAULT_TIMEOUT */);

	if (res < 0) {
		usbmuxd_log(LL_ERROR, "Could not get the configuration for device %s using libusb-win32: %d", serial, res);
		return res;
	}
	else {
		*configuration = (uint8_t)config;
		usbmuxd_log(LL_INFO, "The current configuration for device %s is %d", serial, config);
	}

	res = usb_close(device);
	return res;
}

void usb_win32_set_configuration(const char serial[], uint8_t configuration)
{
	usbmuxd_log(LL_INFO, "Setting configuration for device %s using libusb-win32", serial, configuration);

	usb_dev_handle* device = usb_win32_open(serial);

	if (device == NULL) {
		usbmuxd_log(LL_INFO, "Could not find the device %s using libusb-win32", serial, configuration);
		return;
	}

	int res = usb_set_configuration(device, configuration);

	usb_close(device);
}

usb_dev_handle *usb_win32_open(const char serial[])
{
	struct usb_bus *bus;
	struct usb_device *dev;

	usbmuxd_log(LL_INFO, "Finding device %s using libusb-win32", serial);

	EnterCriticalSection(&usb_lock);

	bus = usb_get_busses();

	for (bus; bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor != VID_APPLE
				|| dev->descriptor.idProduct < PID_RANGE_LOW
				|| dev->descriptor.idProduct > PID_RANGE_MAX)
			{
				usbmuxd_log(LL_INFO, "Found device %d on bus %d using libusb-win32, but it is not an Apple device. Skipping", dev->devnum, bus->location);
				continue;
			}

			usb_dev_handle *handle = usb_open(dev);

			char dev_serial[40];
			int ret = usb_get_string_simple(handle, dev->descriptor.iSerialNumber, dev_serial, sizeof(dev_serial));

			if(ret < 0) {
				usbmuxd_log(LL_INFO, "Could not get the UDID for device %d on bus %d using libusb-win32. Skipping", dev->devnum, bus->location);
				usb_close(handle);
				continue;
			}

			if (strcmp(dev_serial, serial) != 0)
			{
				usbmuxd_log(LL_INFO, "The UDID for device %d, %s, on bus %d does not match the requested UDID %s. Skipping", dev->devnum, dev_serial, bus->location, serial);
				usb_close(handle);
				continue;
			}

			usbmuxd_log(LL_INFO, "Found a match on bus %d device %d for serial %s.", bus->location, dev->devnum, serial);
			LeaveCriticalSection(&usb_lock);
			return handle;
		}
	}

	usbmuxd_log(LL_INFO, "A device with serial %s could not be found using libusb-win32", serial);
	LeaveCriticalSection(&usb_lock);
	return NULL;
}
#endif