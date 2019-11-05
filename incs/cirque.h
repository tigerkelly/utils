/*
 * queue.h
 *
 * Description:
 *  Created on: Mar 10, 2018
 *      Author: nkx
 *   Copyright: Network Kinetix
 */

#ifndef INCS_CIRQUE_H_
#define INCS_CIRQUE_H_

#include <pthread.h>

#define QUEUE_NONBLOCK	0
#define QUEUE_BLOCK	1

typedef struct _cirQueIndex_ {
	unsigned int i;
} CirQueIndex;

typedef struct _CirQue {
    volatile int cqItemCount;
	int arrSize;
	int blkSize;
	pthread_mutex_t cqLock;
	pthread_cond_t cqCond;
	int queIn;
	int queOut;
	void *blocks;
	CirQueIndex array[0];
} CirQue;

CirQue *cirQueCreate(int arrSize, int blkSize);
int cirQueGrow(CirQue **q, int growth);
int cirQueAdd(CirQue *q, void *buf, int bufLen);
void *cirQueGetBlock(CirQue *cq, int index);
int cirQueRemove(CirQue *q, void *buf, int bufLen, int block);
int cirQueRemoveTimed(CirQue *q, void *buf, int bufLen, int timeout);
int cirQueDestroy(CirQue *q);
int cirQueCount(CirQue *q);


#endif /* INCS_CIRQUE_H_ */
