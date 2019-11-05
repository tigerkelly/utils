
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

#ifndef _IPCUTILS_H
#define _IPCUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#define MASTERSEM_ID	0x2A474B3		// semaphore ID used to lock master
#define SHM_INDEX		0				// index for shared memory semaphore
#define MSG_INDEX		1				// index for message queue semaphore
#define SEM_INDEX		2				// index for semaphore semaphore
#define MEM_LOCK_INDEX	3				// used to control reads and writes to shared memory
										// for memWrite() and memRead() in mem_utils.c

#define MAX_SEGNAME		32
#define MAX_MEMMASTERS	16
#define MEMMASTER_ID	0x54A9FCF		// unique memory ID (hopefully)

#define MAX_SEMNAME		32
#define MAX_SEMMASTERS	16
#define SEMMASTER_ID	0x4A48198		// unique semaphore ID

#define MAX_MSGNAME		32
#define MAX_MSGMASTERS	16
#define MAX_MSG_SIZE	64
#define MSGMASTER_ID	0x5F42A96		// unique message queue ID

typedef struct _memMaster {
	int id;
	int inUse;
	int usingCnt;
	char memName[32];
	long memSize;
	key_t shmId;
	unsigned char *shmPtr;   // This pointer shouldn't be stored in SHM as it varies from process to process
} MemMaster;

typedef struct _semMaster {
	int id;
	int inUse;
	int usingCnt;
	char semName[32];
	long semSize;
	key_t semId;
} SemMaster;

typedef struct _msgMaster {
	int id;
	int inUse;
	int usingCnt;
	char msgName[32];
	key_t msgId;
} MsgMaster;

int memInit(void);
int memList(char *buffer);
int memCreate(const char *segName, long memSize);
int memDestroy(const char *memName);
void *memAttach(const char *segName);
void memDetach(const char *segName);
int memGetNum(const char *memName);
int memWrite(const char *segName, const char *data, int offset, int length);
int memRead(const char *segName, char *data, int offset, int length);
int memReadWL(const char *segName, char *data, int offset, int length);
int memFastWrite(int memNum, const char *data, int offset, int length);
int memFastRead(int memNum, char *data, int offset, int length);
int memAddrWrite(char *shmAddr, const char *data, int length);
int memAddrRead(char *shmAddr, char *data, int length);
int memUnlockMemory();
int memLockMemory();
int memCheckAndSet(const char *memName, int offset, int length, char *checkVal, char *setVal);
int memGetSize(const char *memName);

typedef union _semun {
	int val;                    /* value for SETVAL */
	struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
	unsigned short int *array;  /* array for GETALL, SETALL */
	struct seminfo *__buf;      /* buffer for IPC_INFO */
} Semun;

int semInit(void);
int semCreate(const char *semName, int semSize);
int semGetId(const char *semName);
int semGetNum(const char *semName);
int semLock(const char *semName, int semIdx);
int semUnlock(const char *semName, int semIdx);
int semFastLock(int semNum, int semIdx);
int semFastUnlock(int semNum, int semIdx);

#ifdef _GNU_SOURCE		// use compiler switch -D_GNU_SOURCE
int semTimedLock(const char *semName, int semIdx, int timeout);
int semTimedUnlock(const char *semName, int semIdx, int timeout);
int semFastTimedLock(int semNum, int semIdx, int timeout);
int semFastTimedUnlock(int semNum, int semIdx, int timeout);
#endif

int msgInit();
int msgCreate(const char *msgName);
int msgGetId(const char *msgName);
int msgGetNum(const char *msgName);
int msgSend(const char *msgName, const char *msg, int msgFlags);
int msgNumSend(const char *msgName, const char *msg, int msgLen, int msgFlags);
int msgRecv(const char *msgName, char *buf, int msgFlags);
int msgPriorityRecv(const char *msgName, char *buf, int msgFlags, long msgPriority);

int msgFastSend(int msgNum, const char *msg, int msgFlags);
int msgFastNumSend(int msgNum, const char *msg, int msgLen, int msgFlags);
int msgFastRecv(int msgNum, char *buf, int msgFlags);
int msgFastPriorityRecv(int msgNum, char *buf, int msgFlags, long msgPriority);

#ifdef __cplusplus
} // closing brace for extern "C"

#endif

#endif
