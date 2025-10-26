#pragma once
#define USBMUXD_PACKAGE_NAME "usbmuxd_jd"
#define USBMUXD_PACKAGE_STRING "usbmuxd_jd 1.1.1"
#define USBMUXD_PACKAGE_VERSION "1.1.1"
//#define HAVE_STRUCT_TIMESPEC 1
//#define SOCKET_NONBLOCK

#ifndef __func__
# define __func__ __FUNCTION__
#endif

#include <stdlib.h>

char *dirname(char const *file);