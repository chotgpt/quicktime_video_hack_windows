#pragma once

#define ETRANSFER_TIMEDOUT 116

#ifdef WIN32
#include <stdint.h>

void usb_win32_init();

int usb_win32_get_configuration(const char serial[], uint8_t *configuration);
void usb_win32_set_configuration(const char serial[], uint8_t configuration);
#endif