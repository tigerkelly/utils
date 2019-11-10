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
 * These functions are used to create a FIFO type linked list or work queue.
 * New data is placed at the end of the queue and can only remove
 * data from the head of the queue.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "logutils.h"
#include "miscutils.h"

int maxQueues;
Queue *queues = NULL;

pthread_mutex_t createLock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t listLock = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t listCond = PTHREAD_COND_INITIALIZER;

/*
 * This function initializes the Linked List Queue array.
 *
 *   queueCnt = max number of named llqueues you can have.
 *
 *   return -1 on error
 *          0 on success
 */
int llqInit(int queueCnt) {
    if (queueCnt == 1)
        maxQueues = 2;
    else if (queueCnt <= 0)
		maxQueues = MAX_LLQUEUES;
	else
		maxQueues = queueCnt;

	queues = (Queue *)calloc(maxQueues, sizeof (Queue));
	if (queues == NULL) {
		Err("Can not allocate Queue memory.\n");
		return -1;
	}

	Queue *qp = queues;
	for (int i = 0; i < maxQueues; i++, qp++) {
		//qp->listCond = PTHREAD_COND_INITIALIZER;
		//qp->listLock = PTHREAD_MUTEX_INITIALIZER;

		if (pthread_mutex_init(&qp->listLock, NULL) != 0) {
			Err("Mutex init lock failed.\n");
			return -1;
		}

		if (pthread_cond_init(&qp->listCond, NULL) != 0) {
			Err("Mutex condition init failed.\n");
			return -1;
		}
	}

	return 0;
}

/*
 * This function llqCreate creates a named Linked List Queue
 *
 *   llqName = Queue name to create.
 *
 *   returns -1 on error
 *           else 0 or greater on success
 */
int llqCreate(const char *llqName) {
	int ret = -1;

	if (queues == NULL) {
		Err("Must call llqInit() first.\n");
		return ret;
	}

	pthread_mutex_lock(&createLock);

	// Look for empty slot or an exiting entry.
	int foundIt = -1;
	int freeSpot = -1;
	Queue *foundSpot = NULL;
	Queue *qp = queues;
	for (int i = 0; i < maxQueues; i++, qp++) {
		if (qp->inUse == 0 && foundSpot == NULL) {
			foundSpot = qp;
			freeSpot = i;
		} else if (qp->inUse == 1 && strcmp(qp->llqName, llqName) == 0) {
			// already been created.
			ret = i;
			foundIt = i;
		}
	}

	if (foundIt == -1) {
		if (freeSpot == -1) {
			Err("No empty slots in Queue array.\n");
			pthread_mutex_unlock(&createLock);
			return ret;
		}

		foundIt = freeSpot;

		foundSpot->inUse = 1;
        foundSpot->itemCount = 0;
		strcpy(foundSpot->llqName, llqName);

		ret = foundIt;
	}

	pthread_mutex_unlock(&createLock);

	return ret;
}

/*
 * This function addQdata is private to this file.
 *
 *   que = Queue entry to add data to
 *   qData = Qdata structure to add to queue.
 *
 *   returns void
 */
void addQdata(Queue *que, Qdata *qData) {

	if (que->head == NULL) {
		que->head = que->tail = qData;
	} else {
		que->tail->next = qData;
		que->tail = qData;
	}
}

/*
 * This function llqAdd adds data to the named queue
 *
 *   queNum = Queue index returned by llqCreate()
 *   data = data to add to queue
 *   length = Length of data
 *
 *   return -1 on error
 *          0 on success
 */
int llqAdd(int queNum, const unsigned char *data, int length) {
	int ret = -1;

	if (queues == NULL) {
		Err("Must call llqInit() first.\n");
		return ret;
	}

	Queue *qp = &queues[queNum];

	if (qp->inUse == 1) {
		pthread_mutex_lock(&qp->listLock);

		Qdata *qData = (Qdata *)calloc(1, sizeof(Qdata));
		qData->length = length;
		qData->data = (unsigned char *)calloc(1, length + 1);
		memcpy(qData->data, data, length);

		addQdata(qp, qData);
		qp->itemCount++;
		pthread_cond_signal(&qp->listCond);
		ret = 0;

		pthread_mutex_unlock(&qp->listLock);
	}

	return ret;
}

/*
 * This function llqAddString is the same as llqAdd but assumes the data is null terminated.
 * See llqAdd() from details.
 *
 *   llqName = Queue name
 *   data = data to add to queue
 *
 *   return -1 on error
 *          0 on success
 */
int llqAddString(int queNum, unsigned char *data) {
    return llqAdd(queNum, data, strlen((char *)data));
}


/*
 * This function llqRemove removes the first element in the queue and
 * returns the data to you.
 * NOTE: The data returned MUST be freed by the caller.
 *
 *   queNum = Queue index
 *   length = Pointer to int to place length into.
 *   block = if true then block waiting on an item else return null if queue empty
 *
 *   returns -1 on error or empty queue
 *           else 0
 */
int llqRemove(int queNum, FData *fdata, int block) {
	Qdata *h = NULL;
	Qdata *t = NULL;

	if (queues == NULL) {
		Err("Must call llqInit() first.\n");
		return -1;
	}

	if (fdata == NULL)
		return -1;

	fdata->needsFreeing = 0;
	fdata->length = 0;

	Queue *qp = &queues[queNum];
	if (qp->inUse == 1) {
		pthread_mutex_lock(&qp->listLock);

		if (qp->itemCount <= 0 && block == 0) {
			pthread_mutex_unlock(&qp->listLock);
			return -1;
		}

		while (qp->itemCount == 0) {
			pthread_cond_wait(&qp->listCond, &qp->listLock);
		}

		h = qp->head;
		t = h->next;
		fdata->data = (unsigned char *)calloc(1, h->length + 1);
		memcpy(fdata->data, h->data, h->length);
		fdata->data[h->length] = '\0';
		free(h->data);
		fdata->needsFreeing = 1;
		fdata->length = h->length;
		free(h);
		qp->head = t;
		if (qp->itemCount > 0)
			qp->itemCount--;
		else
			qp->itemCount = 0;

		pthread_mutex_unlock(&qp->listLock);
	}

	return 0;
}

int llqDestory(int queNum) {

	if (queues == NULL) {
		Err("Must call llqInit() first.\n");
		return -1;
	}

	Queue *qp = &queues[queNum];
	if (qp->inUse == 1) {
		pthread_mutex_lock(&qp->listLock);

		Qdata *qd = qp->head;
		while (qd != NULL) {
			Qdata *t = qd->next;
			free(qd->data);
			free(qd);
			qd = t;
		}

		qp->inUse = 0;

		pthread_mutex_unlock(&qp->listLock);
	}

	return 0;
}

/*
 * This function llqCount return the number of item in the named queue.
 *
 *   llqName = Queue name
 *
 *   returns number items in queue.
 */
int llqCount(int queNum) {
	int count = 0;

	if (queues == NULL) {
		Err("Must call llqInit() first.\n");
		return 0;
	}

	Queue *qp = &queues[queNum];
	if (qp->inUse == 1) {
		pthread_mutex_lock(&qp->listLock);

		count = qp->itemCount;

		pthread_mutex_unlock(&qp->listLock);
	}

	return count;
}

char *llqQueName(int queNum) {
	char *name = NULL;

	if (queues == NULL) {
		Err("Must call llqInit() first.\n");
		return 0;
	}

	Queue *qp = &queues[queNum];
	if (qp->inUse == 1) {
		pthread_mutex_lock(&qp->listLock);

		name = qp->llqName;

		pthread_mutex_unlock(&qp->listLock);
	}

	return name;
}

/*
 * This function llqQueNum returns the index number for the given name.
 *
 *   llqName = Name to find index of.
 *
 * returns queue number or
 *         -1 on error
 */
int llqQueNum(char *llqName) {
	int queNum = -1;

	Queue *qp = queues;
	for (int i = 0; i < maxQueues; i++, qp++) {
		if (qp->inUse == 1 && strcmp(qp->llqName, llqName) == 0) {
			queNum = i;
			break;
		}
	}

	return queNum;
}

void llqFree(FData *fdata) {
	if (fdata->needsFreeing == 1) {
		fdata->needsFreeing = 0;
		fdata->length = 0;
		free(fdata->data);
	}
}
