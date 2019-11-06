/*
 * fqueue.h
 *
 * Description: Generic FIFO queue.
 *  Created on: Dec 1, 2017
 *      Author: Kelly Wiles
 */

#ifndef _GQUEUE_H_
#define _GQUEUE_H_

#include <pthread.h>

typedef struct _gqData {
    unsigned int length;
	unsigned char *data;
	struct _gqData *next;
} GqData;

typedef struct _gq {
	int itemCount;
	pthread_mutex_t listLock;
	pthread_cond_t listCond;
	GqData *head;
	GqData *tail;
} GQ;

typedef struct _gqItem {
	int needsFreeing;
	int length;
	unsigned char *data;
} GqItem;

GQ *gqCreate();
int gqAdd(GQ *qq, const unsigned char *data, int length);
int gqAddString(GQ *gq, unsigned char *data);
int gqRemove(GQ *gq, GqItem *gqItem, int block);
int gqDestory(GQ *gq);

#endif /* _GQUEUE_H_ */
