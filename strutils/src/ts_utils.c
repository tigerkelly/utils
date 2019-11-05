
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

/* Originally written in 2004, modified to bring it up to coding and format standards. */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/*
 * This function getTimestamp returns a static pointer to a timestamp string with milliseconds.
 * In the format of YYYY/MM/DD HH:mm:SS.ms were YYYY = year, MM = month, DD = day,
 * HH = hours, mm = minutes, SS = seconds and ms = milliseconds (1-4 digits)
 *
 *  return pointer to static string.
 */
char *getTimestamp() {
	static char ts[64];

	struct timeval tv;

	gettimeofday(&tv, NULL);

	struct tm *lt = localtime(&tv.tv_sec);

	long year = lt->tm_year + 1900;
	long month = lt->tm_mon;
	long day = lt->tm_mday;

	long hours = ((tv.tv_sec / 3600) % 24);
	long t = tv.tv_sec % 3600;
	long mins = t / 60;
	long secs = t % 60;
	long msecs = tv.tv_usec / 1000;

	sprintf(ts, "%ld/%02ld/%02ld %02ld:%02ld:%02ld.%ld",
			year, month, day,
			hours, mins, secs, msecs);

	return ts;
}
