/*
 * Copyright 2003-2017 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "LogBackend.hxx"
#include "Log.hxx"
#include "util/Domain.hxx"
#include "util/StringUtil.hxx"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_SYSLOG
#include <syslog.h>
#endif

#ifdef ANDROID
#include <android/log.h>

static int
ToAndroidLogLevel(LogLevel log_level)
{
	switch (log_level) {
	case LogLevel::DEBUG:
		return ANDROID_LOG_DEBUG;

	case LogLevel::INFO:
	case LogLevel::DEFAULT:
		return ANDROID_LOG_INFO;

	case LogLevel::WARNING:
		return ANDROID_LOG_WARN;

	case LogLevel::ERROR:
		return ANDROID_LOG_ERROR;
	}

	assert(false);
	gcc_unreachable();
}

#else

static LogLevel log_threshold = LogLevel::INFO;

static bool enable_timestamp;

#ifdef HAVE_SYSLOG
static bool enable_syslog;
#endif

void
SetLogThreshold(LogLevel _threshold)
{
	log_threshold = _threshold;
}

void
EnableLogTimestamp()
{
#ifdef HAVE_SYSLOG
	assert(!enable_syslog);
#endif
	assert(!enable_timestamp);

	enable_timestamp = true;
}

static const char *log_date(void)
{
	static constexpr size_t LOG_DATE_BUF_SIZE = 32;
	static char buf[LOG_DATE_BUF_SIZE];
	time_t t = time(nullptr);
	strftime(buf, LOG_DATE_BUF_SIZE, "%Y %b %d %H:%M:%S : ", localtime(&t));
	return buf;
}

/**
 * Determines the length of the string excluding trailing whitespace
 * characters.
 */
static int
chomp_length(const char *p)
{
	size_t length = strlen(p);
	return StripRight(p, length);
}

#ifdef HAVE_SYSLOG

static int
ToSysLogLevel(LogLevel log_level)
{
	switch (log_level) {
	case LogLevel::DEBUG:
		return LOG_DEBUG;

	case LogLevel::INFO:
		return LOG_INFO;

	case LogLevel::DEFAULT:
		return LOG_NOTICE;

	case LogLevel::WARNING:
		return LOG_WARNING;

	case LogLevel::ERROR:
		return LOG_ERR;
	}

	assert(false);
	gcc_unreachable();
}

static void
SysLog(const Domain &domain, LogLevel log_level, const char *message)
{
	syslog(ToSysLogLevel(log_level), "%s: %.*s",
	       domain.GetName(),
	       chomp_length(message), message);
}

void
LogInitSysLog()
{
	openlog(PACKAGE, 0, LOG_DAEMON);
	enable_syslog = true;
}

void
LogFinishSysLog()
{
	if (enable_syslog)
		closelog();
}

#endif

static void
FileLog(const Domain &domain, const char *message)
{
	fprintf(stderr, "%s%s: %.*s\n",
		enable_timestamp ? log_date() : "",
		domain.GetName(),
		chomp_length(message), message);

#ifdef _WIN32
	/* force-flush the log file, because setvbuf() does not seem
	   to have an effect on WIN32 */
	fflush(stderr);
#endif
}

#endif /* !ANDROID */

void
Log(const Domain &domain, LogLevel level, const char *msg)
{
#ifdef ANDROID
	__android_log_print(ToAndroidLogLevel(level), "MPD",
			    "%s: %s", domain.GetName(), msg);
#else

	if (level < log_threshold)
		return;

#ifdef HAVE_SYSLOG
	if (enable_syslog) {
		SysLog(domain, level, msg);
		return;
	}
#endif

	FileLog(domain, msg);
#endif /* !ANDROID */
}
