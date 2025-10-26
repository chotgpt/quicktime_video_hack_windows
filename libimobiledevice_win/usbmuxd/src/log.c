/*
 * log.c
 *
 * Copyright (C) 2009 Hector Martin <hector@marcansoft.com>
 * Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
 * Copyright (C) 2014 Frederik Carlier <frederik.carlier@quamotion.mobi>
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
#include <string.h>
#include <stdarg.h>
#include <time.h>
#ifdef _MSC_VER
// Includes a definition for 'timeval'
#include "winsock2.h"
#else
#include <sys/time.h>
#endif

#ifdef WIN32
#else
#include <syslog.h>
#endif

#include "log.h"
#include "utils.h"

#ifdef _DEBUG
unsigned int log_level = LL_FLOOD;
#else
unsigned int log_level = LL_WARNING;
#endif

#ifdef WIN32
#else
int log_syslog = 0;

void log_enable_syslog()
{
	if (!log_syslog) {
		openlog("usbmuxd", LOG_PID, 0);
		log_syslog = 1;
	}
}

void log_disable_syslog()
{
	if (log_syslog) {
		closelog();
	}
}

static int level_to_syslog_level(int level)
{
	int result = level + LOG_CRIT;
	if (result > LOG_DEBUG) {
		result = LOG_DEBUG;
	}
	return result;
}
#endif

static void bmuxd_log_level_to_string(enum loglevel level, char* szLevel, size_t SizeInBytes)
{
	if (level >= LL_ENUM_MAX || level < LL_FATAL)
	{
		return;
	}
	if (!szLevel || 0 == SizeInBytes)
	{
		return;
	}
	static const char* szLevelTable[LL_ENUM_MAX];
	szLevelTable[LL_FATAL]		= "fatal";
	szLevelTable[LL_ERROR]		= "error";
	szLevelTable[LL_WARNING]	= "warning";
	szLevelTable[LL_NOTICE]		= "notice";
	szLevelTable[LL_INFO]		= "info";
	szLevelTable[LL_DEBUG]		= "debug";
	szLevelTable[LL_SPEW]		= "spew";
	szLevelTable[LL_FLOOD]		= "flood";
	strcpy_s(szLevel, SizeInBytes, szLevelTable[level]);
}

void usbmuxd_log(enum loglevel level, const char *fmt, ...)
{
	va_list ap;
	char *fs;

	if(level > (int)log_level)
		return;

	fs = malloc(128 + strlen(fmt));
	if (!fs)
	{
		return;
	}

#ifdef WIN32
	struct tm *tp;
	time_t ltime;
	time(&ltime);
	tp = localtime(&ltime);
	char szLevel[10];
	bmuxd_log_level_to_string(level, szLevel, sizeof(szLevel));

	strftime(fs, 10, "[%H:%M:%S", tp);
	sprintf(fs + 9, "[%d][%s] %s\n", level, szLevel, fmt);


#else
	if(log_syslog) {
		sprintf(fs, "[%d] %s\n", level, fmt);
	} else {
		struct timeval ts;
		struct tm tp_;
		struct tm *tp;

		gettimeofday(&ts, NULL);
#ifdef HAVE_LOCALTIME_R
		tp = localtime_r(&ts.tv_sec, &tp_);
#else
		tp = localtime(&ts.tv_sec);
#endif

		strftime(fs, 10, "[%H:%M:%S", tp);
		sprintf(fs+9, ".%03d][%d] %s\n", (int)(ts.tv_usec / 1000), level, fmt);
	}
#endif

	va_start(ap, fmt);

#ifdef WIN32
	vfprintf(stderr, fs, ap);
#else
	if (log_syslog) {
		vsyslog(level_to_syslog_level(level), fs, ap);
	} else {
		vfprintf(stderr, fs, ap);
	}
#endif

	va_end(ap);

	free(fs);
}
