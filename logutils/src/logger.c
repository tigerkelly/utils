
/*
 * Copyright (c) 2015 Richard Kelly Wiles (rkwiles@twc.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Originally written in 1993, modified to bring it up to coding and format standards. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/syslog.h>

#include "logutils.h"

/*
 * This function logOpen opens the system logging.
 *
 *   prgName = Program name to log messages with, if NULL then use program name.
 *   stderrFlag = if 1 then send all messages to stderr also.
 *
 *   returns void
 */
void logOpen(const char *prgName, int stderrFlag, int facility) {
    char *lstp = strrchr(prgName, '/');
    if (lstp == NULL) {
        lstp = (char *)prgName;
	}

	if (stderrFlag)
		openlog(lstp, LOG_PID | LOG_PERROR | LOG_CONS, facility);
	else
		openlog(lstp, LOG_PID | LOG_CONS, facility);
}

/*
 * This function logClose closes the logging system.
 *
 * returns void
 */
void logClose() {
	closelog();
}

/*
 * This function logInfo logs message to system log file as a Informational message.
 *
 *   fmt = String or string with %s or %d like syntax.
 *   ... = variable number of arguments for fmt above if needed.
 *
 *   return void
 */
void logInfo(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsyslog(LOG_INFO, fmt, ap);
	va_end(ap);
}

/*
 * This function logErr logs message to system log file as a Error message.
 *
 *   fmt = String or string with %s or %d like syntax.
 *   ... = variable number of arguments for fmt above if needed.
 *
 *   return void
 */
void logErr(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
}

/*
 * This function logDebug logs message to system log file as a Debug message.
 *
 *   fmt = String or string with %s or %d like syntax.
 *   ... = variable number of arguments for fmt above if needed.
 *
 *   return void
 */
void logDebug(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsyslog(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

/*
 * This function logNotice logs message to system log file as a Notice message.
 *
 *   fmt = String or string with %s or %d like syntax.
 *   ... = variable number of arguments for fmt above if needed.
 *
 *   return void
 */
void logNotice(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsyslog(LOG_NOTICE, fmt, ap);
	va_end(ap);
}

/*
 * This function logWarn logs message to system log file as a Warning message.
 *
 *   fmt = String or string with %s or %d like syntax.
 *   ... = variable number of arguments for fmt above if needed.
 *
 *   return void
 */
void logWarn(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsyslog(LOG_WARNING, fmt, ap);
	va_end(ap);
}
