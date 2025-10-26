/*
 * usb.c
 *
 * Copyright (C) 2009 Hector Martin <hector@marcansoft.com>
 * Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
 * Copyright (C) 2009-2020 Martin Szulecki <martin.szulecki@libimobiledevice.org>
 * Copyright (C) 2014 Mikkel Kamstrup Erlandsen <mikkel.kamstrup@xamarin.com>
 * Copyright (C) 2022 JieDong Feng <djiefeng@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef WIN32
#include <lusb0_usb.h>
#include "usb_win32.h"
#else
#include <libusb.h>
#endif

#include "usb.h"
#include "log.h"
#include "device.h"
#include "utils.h"

#if (defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000102)) || (defined(LIBUSBX_API_VERSION) && (LIBUSBX_API_VERSION >= 0x01000102))
#define HAVE_LIBUSB_HOTPLUG_API 1
#endif

// interval for device connection/disconnection polling, in milliseconds
// we need this because there is currently no asynchronous device discovery mechanism in libusb
#define DEVICE_POLL_TIME 1000

// Number of parallel bulk transfers we have running for reading data from the device.
// Older versions of usbmuxd kept only 1, which leads to a mostly dormant USB port.
// 3 seems to be an all round sensible number - giving better read perf than
// Apples usbmuxd, at least.
#define NUM_RX_LOOPS 3

int libusb_verbose = 0;

struct m_usb_device {
	struct usb_dev_handle *dev;
	uint8_t bus, address;
	char serial[256];
	int alive;
	uint8_t interface, ep_in, ep_out;
	int wMaxPacketSize;
	uint64_t speed;
	struct usb_device_descriptor devdesc;
};

static struct collection device_list;

static struct timeval next_dev_poll_time;

static int device_polling;
static int device_hotplug = 1;

void usb_set_log_level(int level)
{
	libusb_verbose = level;
}

int usb_get_log_level(void)
{
	return libusb_verbose;
}

static void usb_disconnect(struct m_usb_device *dev)
{
	if(!dev->dev) {
		return;
	}

	usb_release_interface(dev->dev, dev->interface);
	usb_close(dev->dev);
	dev->dev = NULL;
	collection_remove(&device_list, dev);
	free(dev);
}

static void reap_dead_devices(void) {
	FOREACH(struct m_usb_device *usbdev, &device_list) {
		if(!usbdev->alive) {
			device_remove(usbdev);
			usb_disconnect(usbdev);
		}
	} ENDFOREACH
}

int usb_send(struct m_usb_device *dev, const unsigned char *buf, int length)
{
	int res;
	res = usb_bulk_write(dev->dev, dev->ep_out, (void*)buf, length, 0);
	if(res < 0) 
	{
		usbmuxd_log(LL_ERROR, "Failed to usb_bulk_write %p len %d to device %d-%d: %d", buf, length, dev->bus, dev->address, res);
		return res;
	}
	return 0;
}

// Callback from read operation
// Under normal operation this issues a new read transfer request immediately,
// doing a kind of read-callback loop
static void rx_callback(struct m_usb_device* dev)
{
	int res;
	int read_len;
	char* buffer = malloc(USB_MRU);
	if (!buffer)
	{
		usbmuxd_log(LL_FATAL, "malloc fail!");
		return;
	}

	while (1)
	{
		res = usb_bulk_read(dev->dev, dev->ep_in, buffer, USB_MRU, 1000);
		if (res < 0)
		{
			if (res == -ETRANSFER_TIMEDOUT)
			{
				continue;
			}
			break;
		}
		read_len = res;
		device_data_input(dev, buffer, read_len);
	}
	usbmuxd_log(LL_DEBUG, "rx_callback exit [%s]", dev->serial);
	free(buffer);
}

// Start a read-callback loop for this device
static int start_rx_loop(struct m_usb_device *dev)
{
	HANDLE hd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)rx_callback, dev, 0, NULL);
	if (hd)
	{
		CloseHandle(hd);
		return 0;
	}
	return -1;
}

static void udid_24_25(char* szUdid)
{
	if (!szUdid)
	{
		return;
	}
	if (strlen(szUdid) == 24)
	{
		for (int i = 24; i >= 8; i--)
		{
			szUdid[i] = szUdid[i - 1];
		}
		szUdid[8] = '-';
		szUdid[25] = '\0';
	}
	else
	{
		NULL;
	}
}

static int usb_device_add(struct usb_device* dev)
{
	int j, res;
	// the following are non-blocking operations on the device list
	uint8_t bus = 0;
	uint8_t address = dev->devnum;
	int found = 0;
	FOREACH(struct m_usb_device *usbdev, &device_list) {
		if(usbdev->bus == bus && usbdev->address == address) {
			usbdev->alive = 1;
			found = 1;
			break;
		}
	} ENDFOREACH
	if(found)
		return 0; //device already found

	if(dev->descriptor.idVendor != VID_APPLE)
		return -1;
	if ((dev->descriptor.idProduct != PID_APPLE_T2_COPROCESSOR)
		&& ((dev->descriptor.idProduct < PID_RANGE_LOW) || (dev->descriptor.idProduct > PID_RANGE_MAX))
		&& (dev->descriptor.idProduct != PID_MACBOOK))
		return -1;
	struct usb_dev_handle *usb_handle;
	usbmuxd_log(LL_INFO, "Found new device with v/p %04x:%04x at %d-%d", dev->descriptor.idVendor, dev->descriptor.idProduct, bus, address);
	// No blocking operation can follow: it may be run in the libusb hotplug callback and libusb will refuse any
	// blocking call
	usb_handle = usb_open(dev);
	if(!usb_handle)
	{
		usbmuxd_log(LL_WARNING, "Could not open device [%d-%d]", bus, address);
		return -1;
	}

	int desired_config = dev->descriptor.bNumConfigurations;
	int current_config = usb_get_configuration_by_devhd(usb_handle);
	usbmuxd_log(LL_INFO, "current_config[%d] desired_config[%d] device [%d-%d]", current_config, desired_config, bus, address);

	if (current_config != desired_config) 
	{
		if ((res = usb_set_configuration(usb_handle, desired_config)) != 0)
		{
			usbmuxd_log(LL_WARNING, "Could not usb_set_configuration for device %d-%d: %d", bus, address, res);
			usb_close(usb_handle);
			return -1;
		}
		usb_close(usb_handle);
		return -2;
	}
	if (current_config != 5)
	{
		usb_control_msg(usb_handle, 0x40, 0x52, 0x00, 0x02, 0, 0, 1000);
		usbmuxd_log(LL_WARNING, "current_config != 5  for device %d-%d", bus, address);
		Sleep(1000);
		usb_close(usb_handle);
		return -3;
	}

	struct usb_config_descriptor* config = &dev->config[dev->descriptor.bNumConfigurations - 1];
	printf("usb_config->iConfiguration[%d]\n", config->iConfiguration);
	printf("usb_config->bNumInterfaces[%d]\n", config->bNumInterfaces);
	printf("usb_config->bConfigurationValue[%d]\n", config->bConfigurationValue);
	struct m_usb_device *usbdev;
	usbdev = malloc(sizeof(struct m_usb_device));
	if (!usbdev)
	{
		usbmuxd_log(LL_FATAL, "malloc fail!");
		usb_close(usb_handle);
		return -1;
	}
	memset(usbdev, 0, sizeof(*usbdev));


	for (j = 0; j < config->bNumInterfaces; j++)
	{
		const struct usb_interface_descriptor* intf = &config->interface[j].altsetting[0];
		printf("usb_interface_descriptor [%d/%d] bInterfaceClass[0x%02x] bInterfaceSubClass[0x%02x] bInterfaceProtocol[0x%02x]\n", j, config->bNumInterfaces, intf->bInterfaceClass, intf->bInterfaceSubClass, intf->bInterfaceProtocol);
		if (intf->bInterfaceClass != INTERFACE_CLASS ||
			intf->bInterfaceSubClass != INTERFACE_SUBCLASS ||
			intf->bInterfaceProtocol != INTERFACE_PROTOCOL)
		{
			continue;
		}
		if (intf->bNumEndpoints != 2) {
			usbmuxd_log(LL_WARNING, "Endpoint count mismatch for interface %d of device %d-%d", intf->bInterfaceNumber, bus, address);
			continue;
		}
		if ((intf->endpoint[0].bEndpointAddress & 0x80) == USB_ENDPOINT_OUT &&
			(intf->endpoint[1].bEndpointAddress & 0x80) == USB_ENDPOINT_IN) {
			usbdev->interface = intf->bInterfaceNumber;
			usbdev->ep_out = intf->endpoint[0].bEndpointAddress;
			usbdev->ep_in = intf->endpoint[1].bEndpointAddress;
			usbdev->wMaxPacketSize = intf->endpoint[1].wMaxPacketSize;
			usbmuxd_log(LL_INFO, "Found interface %d with endpoints %02x/%02x for device %d-%d", usbdev->interface, usbdev->ep_out, usbdev->ep_in, bus, address);
			break;
		}
		else if ((intf->endpoint[1].bEndpointAddress & 0x80) == USB_ENDPOINT_OUT &&
			(intf->endpoint[0].bEndpointAddress & 0x80) == USB_ENDPOINT_IN) {
			usbdev->interface = intf->bInterfaceNumber;
			usbdev->ep_out = intf->endpoint[1].bEndpointAddress;
			usbdev->ep_in = intf->endpoint[0].bEndpointAddress;
			usbdev->wMaxPacketSize = intf->endpoint[0].wMaxPacketSize;
			usbmuxd_log(LL_INFO, "Found interface %d with swapped endpoints %02x/%02x for device %d-%d", usbdev->interface, usbdev->ep_out, usbdev->ep_in, bus, address);
			break;
		}
		else {
			usbmuxd_log(LL_WARNING, "Endpoint type mismatch for interface %d of device %d-%d", intf->bInterfaceNumber, bus, address);
		}
	}
	if (j == config->bNumInterfaces) {
		usbmuxd_log(LL_WARNING, "Could not find a suitable USB interface for device %d-%d", bus, address);
		usb_close(usb_handle);
		free(usbdev);
		return -1;
	}

	printf("usbdev->interface[%d]\n", usbdev->interface);
	printf("usbdev->ep_out[%02x]\n", usbdev->ep_out);
	printf("usbdev->ep_in[%02x]\n", usbdev->ep_in);
	printf("usbdev->wMaxPacketSize[%d]\n", usbdev->wMaxPacketSize);

	if((res = usb_claim_interface(usb_handle, usbdev->interface)) != 0) {
		usbmuxd_log(LL_WARNING, "Could not claim interface %d for device %d-%d: %d", usbdev->interface, bus, address, res);
		usb_close(usb_handle);
		free(usbdev);
		return -1;
	}

	memset(usbdev->serial, 0, sizeof(usbdev->serial));
	res = usb_get_string_simple(usb_handle, dev->descriptor.iSerialNumber, usbdev->serial, sizeof(usbdev->serial));
	if (res <= 0)
	{
		usbmuxd_log(LL_WARNING, "Could not get serial %d for device %d-%d: %d", usbdev->interface, bus, address, res);
		usb_close(usb_handle);
		free(usbdev);
		return -1;
	}
	udid_24_25(usbdev->serial);

	usbdev->bus = bus;
	usbdev->address = address;
	usbdev->devdesc = dev->descriptor;
	usbdev->speed = 480000000;
	usbdev->dev = usb_handle;
	usbdev->alive = 1;
	if (usbdev->wMaxPacketSize <= 0) {
		usbmuxd_log(LL_ERROR, "Could not determine wMaxPacketSize for device %d-%d, setting to 64", usbdev->bus, usbdev->address);
		usbdev->wMaxPacketSize = 64;
	} else {
		usbmuxd_log(LL_INFO, "Using wMaxPacketSize=%d for device %d-%d", usbdev->wMaxPacketSize, usbdev->bus, usbdev->address);
	}
	//usbmuxd_log(LL_INFO, "USB Speed is %g MBit/s for device %d-%d", (double)(usbdev->speed / 1000000.0), usbdev->bus, usbdev->address);

	/* Finish setup now */
	if (device_add(usbdev) < 0)
	{
		usb_disconnect(usbdev);
		//usb_close(usb_handle);
		//free(usbdev);
		return -1;
	}

	if (start_rx_loop(usbdev) < 0)
	{
		usbmuxd_log(LL_WARNING, "Failed to start RX loop number");
		usb_close(usb_handle);
		free(usbdev);
		return -1;
	}

	collection_add(&device_list, usbdev);
	return 0;
}

int usb_discover(void)
{
	int cnt = 0;
	int valid_count = 0;
	struct usb_bus* bus;
	struct usb_device* dev;

	// Mark all devices as dead, and do a mark-sweep like
	// collection of dead devices
	FOREACH(struct m_usb_device* usbdev, &device_list) {
		usbdev->alive = 0;
	} ENDFOREACH


	usb_find_devices();
	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			cnt++;
			if (usb_device_add(dev) < 0) {
				continue;
			}
			valid_count++;
		}
	}
	if (cnt == 0)
	{
		usbmuxd_log(LL_WARNING, "Could not get device list");
		get_tick_count(&next_dev_poll_time);
		next_dev_poll_time.tv_usec += DEVICE_POLL_TIME * 1000;
		next_dev_poll_time.tv_sec += next_dev_poll_time.tv_usec / 1000000;
		next_dev_poll_time.tv_usec = next_dev_poll_time.tv_usec % 1000000;
		return 0;
	}

	// Clean out any device we didn't mark back as live
	reap_dead_devices();

	get_tick_count(&next_dev_poll_time);
	next_dev_poll_time.tv_usec += DEVICE_POLL_TIME * 1000;
	next_dev_poll_time.tv_sec += next_dev_poll_time.tv_usec / 1000000;
	next_dev_poll_time.tv_usec = next_dev_poll_time.tv_usec % 1000000;

	return valid_count;
}

const char *usb_get_serial(struct m_usb_device *dev)
{
	if(!dev->dev) {
		return NULL;
	}
	return dev->serial;
}

uint32_t usb_get_location(struct m_usb_device *dev)
{
	if(!dev->dev) {
		return 0;
	}
	return (dev->bus << 16) | dev->address;
}

uint16_t usb_get_pid(struct m_usb_device *dev)
{
	if(!dev->dev) {
		return 0;
	}
	return dev->devdesc.idProduct;
}

uint64_t usb_get_speed(struct m_usb_device *dev)
{
	if (!dev->dev) {
		return 0;
	}
	return dev->speed;
}

#ifndef WIN32
void usb_get_fds(struct fdlist *list)
{
	const struct libusb_pollfd **usbfds;
	const struct libusb_pollfd **p;
	usbfds = libusb_get_pollfds(NULL);
	if(!usbfds) {
		usbmuxd_log(LL_ERROR, "libusb_get_pollfds failed");
		return;
	}
	p = usbfds;
	while(*p) {
		fdlist_add(list, FD_USB, (*p)->fd, (*p)->events);
		p++;
	}
	free(usbfds);
}
#endif

void usb_autodiscover(int enable)
{
	usbmuxd_log(LL_DEBUG, "usb polling enable: %d", enable);
	device_polling = enable;
	device_hotplug = enable;
}

static int dev_poll_remain_ms(void)
{
	int msecs;
	struct timeval tv;
	if(!device_polling)
		return 100000; // devices will never be polled if this is > 0
	get_tick_count(&tv);
	msecs = (next_dev_poll_time.tv_sec - tv.tv_sec) * 1000;
	msecs += (next_dev_poll_time.tv_usec - tv.tv_usec) / 1000;
	if(msecs < 0)
		return 0;
	return msecs;
}

int usb_process(void)
{
	int res;

	// reap devices marked dead due to an RX error
	reap_dead_devices();

	if(dev_poll_remain_ms() <= 0) {
		res = usb_discover();
		if(res < 0) {
			usbmuxd_log(LL_ERROR, "usb_discover failed: %d", res);
			return res;
		}
	}
	return 0;
}

#ifdef HAVE_LIBUSB_HOTPLUG_API
static libusb_hotplug_callback_handle usb_hotplug_cb_handle;

static int LIBUSB_CALL usb_hotplug_cb(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
		if (device_hotplug) {
			usb_device_add(device);
		}
	} else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
		uint8_t bus = libusb_get_bus_number(device);
		uint8_t address = libusb_get_device_address(device);
		FOREACH(struct m_usb_device *usbdev, &device_list) {
			if(usbdev->bus == bus && usbdev->address == address) {
				usbdev->alive = 0;
				device_remove(usbdev);
				break;
			}
		} ENDFOREACH
	} else {
		usbmuxd_log(LL_ERROR, "Unhandled event %d", event);
	}
	return 0;
}
#endif

int usb_initialize(void)
{
	int res;

#if DEBUG || _DEBUG
	usb_set_debug(255);
#else
	static int verbose = 0;
	static int libusb_verbose = 0;
#endif

#ifdef WIN32
	usb_win32_init();
#endif
	const struct usb_version* usb_version_info = usb_get_version();
	if (usb_version_info)
	{
		usbmuxd_log(LL_NOTICE, "DLL version:\t%d.%d.%d.%d",
			usb_version_info->dll.major, usb_version_info->dll.minor,
			usb_version_info->dll.micro, usb_version_info->dll.nano);
		usbmuxd_log(LL_NOTICE, "Driver version:\t%d.%d.%d.%d",
			usb_version_info->driver.major, usb_version_info->driver.minor,
			usb_version_info->driver.micro, usb_version_info->driver.nano);
}
	device_polling = 1;

	collection_init(&device_list);

#ifdef HAVE_LIBUSB_HOTPLUG_API
	if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
		usbmuxd_log(LL_INFO, "Registering for libusb hotplug events");
		res = libusb_hotplug_register_callback(NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, VID_APPLE, LIBUSB_HOTPLUG_MATCH_ANY, 0, usb_hotplug_cb, NULL, &usb_hotplug_cb_handle);
		if (res == LIBUSB_SUCCESS) {
			device_polling = 0;
		} else {
			usbmuxd_log(LL_ERROR, "ERROR: Could not register for libusb hotplug events. %s", libusb_error_name(res));
		}
	} else {
		usbmuxd_log(LL_ERROR, "libusb does not support hotplug events");
	}
#endif
	if (device_polling) {
		res = usb_discover();
		if (res >= 0) {
		}
	} else {
		res = collection_count(&device_list);
	}
	return res;
}

void usb_shutdown(void)
{
	usbmuxd_log(LL_DEBUG, "usb_shutdown");

#ifdef HAVE_LIBUSB_HOTPLUG_API
	libusb_hotplug_deregister_callback(NULL, usb_hotplug_cb_handle);
#endif

	FOREACH(struct m_usb_device *usbdev, &device_list) {
		device_remove(usbdev);
		usb_disconnect(usbdev);
	} ENDFOREACH
	collection_free(&device_list);
}
