/*
 * gqueue.c
 *
 * Description: Generic FIFO like queue.
 *  Created on: July 4, 2019
 *      Author: Kelly Wiles
 *   Copyright: Kelly Wiles
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gqueue.h>
#include <pthread.h>

#include "logutils.h"
#include "miscutils.h"

GQ *gqCreate() {
	GQ *gq = (GQ *)calloc(1, sizeof(GQ));

	// gq->listCond = PTHREAD_COND_INITIALIZER;
	// gq->listLock = PTHREAD_MUTEX_INITIALIZER;

	if (pthread_mutex_init(&gq->listLock, NULL) != 0) {
		pErr("Mutex init lock failed.\n");
		free(gq);
		return NULL;
	}

	if (pthread_cond_init(&gq->listCond, NULL) != 0) {
		pErr("Mutex condition init failed.\n");
		free(gq);
		return NULL;
	}

	return gq;
}

/*
 * This function addGQdata is private to this file.
 *
 *   fq = Queue entry to add data to
 *   fqData = FqData structure to add to queue.
 *
 *   returns void
 */
void addGQdata(GQ *gq, GqData *gqData) {

	if (gq->head == NULL) {
		gq->head = gq->tail = gqData;
	} else {
		gq->tail->next = gqData;
		gq->tail = gqData;
	}
}

/*
 * This function gqAdd adds data to the named queue
 *
 *   queNum = Queue index returned by llqCreate()
 *   data = data to add to queue
 *   length = Length of data
 *
 *   return -1 on error
 *          0 on success
 */
int gqAdd(GQ *gq, const unsigned char *data, int length) {
	int ret = -1;

	pthread_mutex_lock(&gq->listLock);

	GqData *gqData = (GqData *)calloc(1, sizeof(GqData));
	gqData->length = length;
	gqData->data = (unsigned char *)calloc(1, length + 1);
	memcpy(gqData->data, data, length);

	addGQdata(gq, gqData);
	gq->itemCount++;
	pthread_cond_signal(&gq->listCond);
	ret = 0;

	pthread_mutex_unlock(&gq->listLock);

	return ret;
}

/*
 * This function gqAddString is the same as llqAdd but assumes the data is null terminated.
 * See llqAdd() from details.
 *
 *   gq = GQ returned from fqCreate().
 *   data = data to add to queue
 *
 *   return -1 on error
 *          0 on success
 */
int gqAddString(GQ *fq, unsigned char *data) {
    return gqAdd(fq, data, strlen((char *)data));
}

/*
 * This function gqRemove removes the first element in the queue and
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
int gqRemove(GQ *gq, GqItem *fdata, int block) {
	GqData *h = NULL;
	GqData *t = NULL;

	if (fdata == NULL)
		return -1;

	fdata->needsFreeing = 0;
	fdata->length = 0;

	pthread_mutex_lock(&gq->listLock);

	if (gq->itemCount <= 0 && block == 0) {
		pthread_mutex_unlock(&gq->listLock);
		return -1;
	}

	while (gq->itemCount == 0) {
		pthread_cond_wait(&gq->listCond, &gq->listLock);
	}

	h = gq->head;
	t = h->next;
	fdata->data = (unsigned char *)calloc(1, h->length + 1);
	memcpy(fdata->data, h->data, h->length);
	fdata->data[h->length] = '\0';
	free(h->data);
	fdata->needsFreeing = 1;
	fdata->length = h->length;
	free(h);
	gq->head = t;
	if (gq->itemCount > 0)
		gq->itemCount--;
	else
		gq->itemCount = 0;

	pthread_mutex_unlock(&gq->listLock);

	return 0;
}

int qqDestory(GQ *gq) {

	if (gq == NULL) {
		return -1;
	}

	pthread_mutex_lock(&gq->listLock);
	GqData *qd = gq->head;
	while (qd != NULL) {
		GqData *t = qd->next;
		free(qd->data);
		free(qd);
		qd = t;
	}

	pthread_mutex_unlock(&gq->listLock);

	free(gq);

	return 0;
}
