/*
 * Copyright (c) 2019 Richard Kelly Wiles (rkwiles@twc.com)
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
 * timefunc.c
 *
 *  Created on: July 4, 2019
 *      Author: Kelly Wiles
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "miscutils.h"

#define BILLION 1000000000L
#define MILLION 1000000L
#define THOUSAND 1000L

#ifndef TIMEVAL_TO_TIMESPEC
#define  TIMEVAL_TO_TIMESPEC(tv, ts)	      \
	do {                                      \
		(ts)->tv_sec = (tv)->tv_sec;          \
		(ts)->tv_nsec = (tv)->tv_usec * THOUSAND; \
	} while (0)
#endif

#ifndef TIMESPEC_TO_TIMEVAL
#define	TIMESPEC_TO_TIMEVAL(tv, ts)           \
	do {								      \
		(tv)->tv_sec = (ts)->tv_sec;          \
		(tv)->tv_usec = (ts)->tv_nsec / THOUSAND; \
	} while (0)
#endif

/*
 * timeTimestamp converts either timeval or timespec to microseconds.
 *
 * returns time in microseconds.
 */
unsigned long timeTimestamp(void *timeStruct, TimeType timeType) {
	struct timeval *tv = NULL;
	struct timeval ttv;
	unsigned int *s;
	unsigned long timeValue = 0;

	// Convert all three types to microseconds.

	switch (timeType) {
	case TIME_VAL:
		/*
		 * struct timeval {
         *    time_t          tv_sec;         // seconds
         *    suseconds_t     tv_usec;        // and microseconds
         * };
		 */
		tv = (struct timeval *)timeStruct;
		timeValue = (tv->tv_sec * MILLION);
		timeValue += tv->tv_usec;
		break;
	case TIME_SPEC:
		/*
		 * struct timespec {
		 *     time_t   tv_sec;        // seconds
		 *     long     tv_nsec;       // nanoseconds
		 * };
		 */
		TIMESPEC_TO_TIMEVAL(&ttv, (struct timespec *)timeStruct);
		timeValue = (ttv.tv_sec * MILLION);
		timeValue += ttv.tv_usec;
		break;
	case TIME_SECONDS:
		s = (unsigned int *)timeStruct;
		timeValue = ((long)*s * MILLION);
		break;
	default:
		break;
	}

	return timeValue;
}

/*
 * timeTimestamp converts either timeval or timespec to seconds.
 *
 * returns time in seconds.
 */
unsigned int timeEpcoh(void * timeStruct, TimeType timeType) {
	struct timeval *tv = NULL;
	struct timeval ttv;
	unsigned int *s;
	unsigned int timeValue = 0;

	switch(timeType) {
	case TIME_VAL:
		/*
		 * struct timeval {
		 *    time_t          tv_sec;         // seconds
		 *    suseconds_t     tv_usec;        // and microseconds
		 * };
		 */
		tv = (struct timeval *)timeStruct;
		timeValue = tv->tv_sec;
		break;
	case TIME_SPEC:
		/*
		 * struct timespec {
		 *     time_t   tv_sec;        // seconds
		 *     long     tv_nsec;       // nanoseconds
		 * };
		 */
		TIMESPEC_TO_TIMEVAL(&ttv, (struct timespec *)timeStruct);
		timeValue = ttv.tv_sec;
		break;
	case TIME_SECONDS:
		s = (unsigned int *)timeStruct;
		timeValue = *s;
		break;
	default:
		break;
	}

	return timeValue;
}

void timeFunc(char *name, int flag, struct timespec *st) {
	unsigned long ms = 0;
	struct timespec et;

	if (flag == 0) {
		clock_gettime(CLOCK_REALTIME, st);
	} else {
		clock_gettime(CLOCK_REALTIME, &et);
		ms = (et.tv_sec - st->tv_sec) * 1000;
		ms += (et.tv_nsec > st->tv_nsec)? (et.tv_nsec - st->tv_nsec) : (st->tv_nsec - et.tv_nsec);
		pOut("%s: took: %3ld ms\n", name, (ms / MILLION));
	}
}

void timeFuncNano(char *name, int flag, struct timespec *st) {
	unsigned long ns = 0;
	struct timespec et;

	if (flag == 0) {
		clock_gettime(CLOCK_REALTIME, st);
	} else {
		clock_gettime(CLOCK_REALTIME, &et);
		ns = BILLION * (et.tv_sec - st->tv_sec) + et.tv_nsec - st->tv_nsec;
		pOut("%s: took: %6ldns  %fms\n", name, ns, (float)(ns / 1000000.0));
	}
}

unsigned long timeNano(int flag, struct timespec *st) {
	unsigned long ns = 0;
	struct timespec et;

	if (flag == 0) {
		clock_gettime(CLOCK_REALTIME, st);
	} else {
		clock_gettime(CLOCK_REALTIME, &et);
		ns = BILLION * (et.tv_sec - st->tv_sec) + et.tv_nsec - st->tv_nsec;
	}

	return ns;
}

//double timeCPU(int flag, clock_t *st) {
//	double ns = 0;
//
//	if (flag == 0) {
//		*st = clock();
//	} else {
//		ns = (double)(clock() - *st) / CLOCKS_PER_SEC;
//	}
//
//	return ns;
//}

void milliSleep(int milliSeconds) {
	struct timespec ts, rt;

	ts.tv_sec = 0;
	ts.tv_nsec = milliSeconds * MILLION;

	nanosleep(&ts, &rt);
}

/**
 * Compute diff in time in usec
  */
long getTimeDiff (const struct timeval *pEndTime, const struct timeval *pBeginTime) {
    long tvSec = pEndTime->tv_sec - pBeginTime->tv_sec;
    long tvUsec = pEndTime->tv_usec - pBeginTime->tv_usec;

    if (tvSec > 0 && tvUsec < 0) {
        tvUsec += MILLION;
        tvSec--;
    }
    tvUsec += (tvSec * MILLION);

    return tvUsec;
}

