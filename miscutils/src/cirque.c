/*
 * Copyright (c) 2018 Richard Kelly Wiles (rkwiles@twc.com)
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
 * cirque.c
 *
 * This is just like the cqueue.c file but more user friendly, hopefully.
 *
 *  Created on: March 10, 2018
 *      Author: Kelly Wiles
 */

#include <cirque.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdatomic.h>
#include <errno.h>

#include "miscutils.h"
#include "cirque.h"

#define MILLION		1000000L
#define BILLION		1000000000L

/*
 * This function cirQueCreate creates a Circular Queue with storage blocks.
 *
 *   arrSize = Size of number of messages
 *   blkSize = Size of blocks.
 *
 * NOTE: The max size of the queue will be able to handle is (arrSize - 1)
 *
 *   returns -1 on error
 *           else 0 or greater on success
 */
CirQue *cirQueCreate(int arrSize, int blkSize) {
	CirQue *cq = NULL;

	int size = arrSize * sizeof(CirQueIndex);

	cq = (CirQue *)calloc(1, (sizeof(CirQue) + size));
	if (cq == NULL) {
		pErr("Unable to allocate memory for Queue structure\n");
		return NULL;
	}

	if (blkSize <= 0) {
		pErr("blkSize must be greater than zero.\n");
		free(cq);
		return NULL;
	}

	cq->blocks = (void *)calloc(arrSize, blkSize);
	if (cq->blocks == NULL) {
		pErr("Could not allocate blocks.\n");
		free(cq);
		return NULL;
	}

	pthread_mutex_init(&cq->cqLock, NULL);
	pthread_cond_init(&cq->cqCond, NULL);

	if (pthread_mutex_init(&cq->cqLock, NULL) != 0) {
		pErr("Mutex init lock failed.\n");
		return NULL;
	}

	if (pthread_cond_init(&cq->cqCond, NULL) != 0) {
		pErr("Mutex condition init failed.\n");
		return NULL;
	}

	cq->cqItemCount = 0;
	cq->queIn = 0;
	cq->queOut = arrSize - 1;
	cq->arrSize = arrSize;
	cq->blkSize = blkSize;

	return cq;
}

/*
 * This function cirQueAdd adds a value to the circular list.
 *
 *   cqNum = Number returned by the cqCreate().
 *   buf = data to copy into blocks.
 *   bufLen = length of data to copy
 *
 *   return 0 on success
 *   		-3 bufLen too large
 *          -2 on queue full
 *   		-1 on error;
 */
int cirQueAdd(CirQue *cq, void *buf, int bufLen) {

	if (cq == NULL) {
		pErr("Must call cirQueCreate() first.\n");
		return -1;
	}

	pthread_mutex_lock(&cq->cqLock);

	if (cq->queIn == cq->queOut) {
		// queue is full.
		pthread_cond_broadcast(&cq->cqCond);
		pthread_mutex_unlock(&cq->cqLock);
		return -2;
	}

	if (cq->blocks != NULL) {
		void *p = cq->blocks + (cq->cqItemCount * cq->blkSize);

		if (bufLen <= cq->blkSize)
			memcpy(p, buf, bufLen);
		else {
			pthread_cond_broadcast(&cq->cqCond);
			pthread_mutex_unlock(&cq->cqLock);
			return -3;
		}
	}

	cq->array[cq->queIn].i = cq->cqItemCount;

	// move queIn to next slot.
	cq->queIn = (cq->queIn + 1) % cq->arrSize;
	cq->cqItemCount++;
	pthread_cond_broadcast(&cq->cqCond);

	pthread_mutex_unlock(&cq->cqLock);

	return 0;
}

/*
 * This function cirQueRemove returns the first element in the queue.
 *
 *   cq = Pointer returned by cirQueCreate
 *   value = Place value from queue here.
 *   block = if true then block waiting on an item else return null if queue empty
 *
 *   returns -1 on error or empty queue
 *           else 0
 */
int cirQueRemove(CirQue *cq, void *buf, int bufLen, int block) {
	int ret = -1;

	if (cq == NULL) {
		pErr("Must call cirQueCreate() first.\n");
		return ret;
	}

	pthread_mutex_lock(&cq->cqLock);
	if (((cq->queOut + 1) % cq->arrSize) == cq->queIn) {
		// queue is empty.
		if (cq->cqItemCount <= 0 && block == QUEUE_NONBLOCK) {
			pthread_mutex_unlock(&cq->cqLock);
//			pErr("Queue empty and non blocking is turned on.\n");
			return -1;
		}

		// block waiting on data to arrive.
//		pOut("Waiting on queue %s  %d\n", cqGetName(cqNum), _cqueues[cqNum]->cqItemCount);
		while (cq->cqItemCount == 0) {
			pthread_cond_wait(&cq->cqCond, &cq->cqLock);
		}
//		pOut("Awake from queue. %s %d\n", cqGetName(cqNum), cp->cqItemCount);
	}

	cq->queOut = (cq->queOut + 1) % cq->arrSize;

	int index = cq->array[cq->queOut].i;

	void *p = cq->blocks + (index * cq->blkSize);

	if (cq->blkSize <= bufLen)
		memcpy(buf, p, bufLen);

	int exp = -1;
	int d = 0;

	cq->cqItemCount--;
	if (AtomicExchange(&cq->cqItemCount, &exp, &d) == 1) {
		pOut("Warning: q->cqItemCount was -1.\n");
	}

	pthread_mutex_unlock(&cq->cqLock);

	return 0;
}

/*
 * This function cirQueGrow grows the queue size by growth.
 *
 * NOTE: No other thread should be doing anything to the queue
 * 		 while this function is being called.
 *
 * SPECIAL NOTE: Use the following code snippet on how to call this function.
 * 		int r = cirQueGrow(&cq, additionalQueues);
 *
 *  cq is the original pointer returned by cirQueCreate().
 *
 * 	cq = Pointer returned by cirQueCreate
 * 	growth = number of CirQueIndex structures to grow by.
 *
 * 	returns 0 if successful
 * 		-1 if cirQueCreate not called first.
 * 		-2 realloc failed.
 */
int cirQueGrow(CirQue **cq, int growth) {
	CirQue *p = NULL;
	int retFlag = -1;

	if (cq == NULL) {
		pErr("Must call cirQueCreate() first.\n");
		return retFlag;
	}

	CirQue *cqp = *cq;

	pthread_mutex_lock(&cqp->cqLock);

	int newSize = (cqp->arrSize + growth) * sizeof(CirQueIndex);
	p = (CirQue *)realloc(*cq, (sizeof(CirQue) + newSize));

	retFlag = -2;
	if (p != NULL) {
		retFlag = 0;
		p->arrSize += growth;

		// increase blocks
		void *p2 = (void *)realloc(p->blocks, (p->arrSize * p->blkSize));
		if (p2 != NULL) {
			p->blocks = p2;
		} else {
			// It failed, what do I do now?
			// Lets just hope this never happens.
		}
		*cq = p;
	}



	pthread_mutex_unlock(&cqp->cqLock);

	return retFlag;
}

void *cirQueGetBlock(CirQue *cq, int index) {
	if (cq->blocks == NULL)
		return NULL;
	else
		return cq->blocks + (index * cq->blkSize);
}

/*
 * This function cirQueRemoveTimed returns the first element in the queue.
 *
 *   cq = Pointer returned by cirQueCreate
 *   value = Place value from queue here.
 *   timeout = Block N milliseconds waiting on an item.
 *
 *   returns -1 on error
 *   		 -2 timed out
 *           else 0
 */
int cirQueRemoveTimed(CirQue *cq, void *buf, int bufLen, int timeout) {
	int ret = -1;
	int rc;
	unsigned long t;
	struct timeval tv;			// microseconds 1 millionth of a second
	struct timespec ts;			// nanoseconds 1 billionth of a second

	t = timeout * MILLION;		// convert milliseconds to nanoseconds.

	if (cq == NULL) {
		pErr("Must call cirQueCreate() first.\n");
		return ret;
	}

	pthread_mutex_lock(&cq->cqLock);
	if (((cq->queOut + 1) % cq->arrSize) == cq->queIn) {
		// queue is empty.

		gettimeofday(&tv, NULL);

		// Convert from timeval to timespec
		ts.tv_sec  = tv.tv_sec;
		ts.tv_nsec = tv.tv_usec * 1000;
		// add timeout (t) into timespec
		ts.tv_sec += (t / BILLION);
		ts.tv_nsec += (t % BILLION);

		// block waiting on data to arrive or time out.
//		pOut("Waiting on queue %s  %d\n", cqGetName(cqNum), _cqueues[cqNum]->cqItemCount);
		while (cq->cqItemCount == 0) {
			rc = pthread_cond_timedwait(&cq->cqCond, &cq->cqLock, &ts);
			if (rc == ETIMEDOUT) {
				pthread_mutex_unlock(&cq->cqLock);
				return -2;
			}
		}
//		pOut("Awake from queue. %s %d\n", cqGetName(cqNum), cp->cqItemCount);
	}

	cq->queOut = (cq->queOut + 1) % cq->arrSize;

	int index = cq->array[cq->queOut].i;

	void *p = cq->blocks + (index * cq->blkSize);

	if (cq->blkSize <= bufLen)
		memcpy(buf, p, bufLen);

	int exp = -1;
	int d = 0;

	cq->cqItemCount--;
	if (AtomicExchange(&cq->cqItemCount, &exp, &d) == 1) {
		pOut("Warning: cp->cqItemCount was -1.\n");
	}

	pthread_mutex_unlock(&cq->cqLock);

	return 0;
}

int cirQueDestroy(CirQue *cq) {

	if (cq == NULL) {
		pErr("Must call cirQueCreate() first.\n");
		return -1;
	}

	if (cq->blocks != NULL)
		free(cq->blocks);
	free(cq);

	return 0;
}

/*
 * This function cirQueCount return the number of item in the named queue.
 *
 *   cq = Pointer returned by cirQueCreate
 *
 *   returns number items in queue,
 *           -1 on error.
 */
int cirQueCount(CirQue *cq) {

	if (cq == NULL) {
		pErr("Must call cirQueCreate() first.\n");
		return -1;
	}

	return cq->cqItemCount;
}
