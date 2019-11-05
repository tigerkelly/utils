
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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>

#include "ipcutils.h"

MsgMaster *msgMaster = NULL;

key_t msgMasterId;

typedef struct _msgBuf {
	long type;
	char data[MAX_MSG_SIZE + 1];
} MsgBuf;

key_t masterSemId;

/*
 * This function msgInit attaches or creates the master memory segment.
 * This segment keeps track of all segments created through this library.
 *
 * returns  0 on success.
 *         -1 on failure.
 */
int msgInit() {
	// Attach or create the message queue master memory segment.
	msgMasterId = shmget(MSGMASTER_ID,
			(MAX_MSGMASTERS * sizeof(MsgMaster)),
			0666);

	if (msgMasterId == -1) {
		if (errno == ENOENT) {
			// Memory master not allocated yet.

			msgMasterId = shmget(MSGMASTER_ID,
					(MAX_MSGMASTERS * sizeof(MsgMaster)),
					(IPC_CREAT | IPC_EXCL | 0666));
		} else {
			fprintf(stderr, "%s (%d): Could create master shared memory segment for message queues.\n", __FILE__, __LINE__);
			return -1;
		}
	}

	msgMaster = (MsgMaster *)shmat(msgMasterId, NULL, 0);
	if (msgMaster == (void*)-1) {
		fprintf(stderr, "%s (%d): failed to attach to master shared memory for message queues.", __FILE__, __LINE__);
		return -1;
	}

	/*
	 * The master semaphore set has three semaphore for each of the IPCs.
	 * Each semaphore controls access to the master memory segment for each IPCs.
	 */

	masterSemId = semget(MASTERSEM_ID, 4, 0666);

	if (masterSemId == -1) {
		if (errno == ENOENT) {
			masterSemId = semget(MASTERSEM_ID, 4, (IPC_CREAT | IPC_EXCL | 0666));
			Semun cmd;

			cmd.val = 0;

			if (semctl(masterSemId, MEM_LOCK_INDEX, SETVAL, cmd) == -1) {
				fprintf(stderr, "%s (%d): Could not initialize semaphores.  errno: %d\n",
						__FILE__, __LINE__, errno);
				perror("memInit:");
				return -1;
			}
		} else {
			fprintf(stderr, "%s (%d): Could not create master semaphore.\n", __FILE__, __LINE__);
			return -1;
		}
	}

	return 0;
}

/*
 * This function is used to lock the master semaphore.
 * This single threads access to the master memory array.
 * This function is private to this file.
 *
 * returns -1 on error
 *         0 on success
 */
static int lockMasterSem() {
	struct sembuf sb[2];

	sb[0].sem_num = MSG_INDEX;
	sb[0].sem_op = 0;			// wait for semaphore to become zero.
	sb[0].sem_flg = 0;

	sb[1].sem_num = MSG_INDEX;
	sb[1].sem_op = 1;			// lock semaphore
	sb[1].sem_flg = SEM_UNDO;

	if (semop(masterSemId, sb, 2) == -1) {
		fprintf(stderr, "%s (%d): Could not lock master semaphore number [%d].\n",
				__FILE__, __LINE__, MSG_INDEX);
		return -1;
	}

	return 0;
}

/*
 * This function unlockMasterSem is used to unlock the master semaphore.
 * This function is private to this file.
 *
 * returns -1 on error
 *         0 on success
 */
static int unlockMasterSem() {
	struct sembuf sb;

	sb.sem_num = MSG_INDEX;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;

	if (semop(masterSemId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number [%d].\n",
				__FILE__, __LINE__, MSG_INDEX);
		return -1;
	}

	return 0;
}

/*
 * This function allocateMsg updates the MsgMasters segment and creates
 * the user message queue if needed.
 * NOTE: This function is private to this library and should not be called by user.
 *
 *   mmp = Pointer to MsgMaster entry for this user segment.
 *
 *   returns -1 on failure
 *           0 on success
 */
int allocateMsg(MsgMaster *mmp) {
	// Attach or create the user message queue.
	mmp->msgId = msgget(mmp->id, 0666);

	if (mmp->msgId == -1) {
		if (errno == ENOENT) {
			// Message queue not allocated yet.

			mmp->msgId = msgget(mmp->id, (IPC_CREAT | IPC_EXCL | 0666));
		} else {
			fprintf(stderr, "%s (%d): Could not create message queue '%s'.\n",
					__FILE__, __LINE__, mmp->msgName);
			return -1;
		}
	}

	return 0;
}

/*
 * This function msgCreate creates or attaches to existing message queue.
 *
 *   msgName = Segment name to attach to or create.
 *
 *   returns -1 on failure
 *           0 if segment already exists
 *           1 if segment was created.
 */
int msgCreate(const char *msgName) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	if (strlen(msgName) > (MAX_MSGNAME -1)) {
		fprintf(stderr, "%s (%d): Message queue name is too long, %d max.\n", __FILE__, __LINE__, MAX_MSGNAME);
		return ret;
	}

	lockMasterSem();

	int foundIt = 0;
	MsgMaster *foundSpot = NULL;
	MsgMaster *mp = msgMaster;
	for (int i = 0; i < MAX_MSGMASTERS; i++, mp++) {
		if (mp->inUse == 0 && foundSpot == NULL) {
			foundSpot = mp;
			foundSpot->id = i + 100;
		} else if (mp->inUse) {
			if (strcmp(msgName, mp->msgName) == 0) {
				foundIt = 1;
				ret = 0;
				break;
			}
		}
	}

	if (foundIt == 0) {
		if (foundSpot == NULL) {
			fprintf(stderr, "%s (%d): No free slot left in master memory segment.\n", __FILE__, __LINE__);
			ret = -1;
		} else {
			foundSpot->inUse = 1;
			strcpy(foundSpot->msgName, msgName);

			allocateMsg(foundSpot);
			ret = 1;
		}
	}

	unlockMasterSem();

	return ret;
}

/*
 * This function msgGetId returns a message queue msgId.
 * Use this function to get the msgId so that you can use msgsnd and msgrcv yourself.
 *
 *   msgName = Message queue name to get.
 *
 *   returns -1 on error or not found
 *           else semId.
 */
int msgGetId(const char *msgName) {
	int msgId = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return msgId;
	}

	MsgMaster *smp = msgMaster;
	for (int i = 0; i < MAX_MSGMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->msgName, msgName) == 0) {
				msgId = smp->msgId;
				break;
			}
		}
	}

	return msgId;
}

/*
 * This function msgGetId returns a message queue msgId.
 * Use this function to get the array offset so that you can use the fast functions.
 *
 *   msgName = Message queue name to get.
 *
 *   returns -1 on error or not found
 *           else master message array offset.
 */
int msgGetNum(const char *msgName) {
	int msgNum = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return msgNum;
	}

	MsgMaster *smp = msgMaster;
	for (int i = 0; i < MAX_MSGMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->msgName, msgName) == 0) {
				msgNum = i;
				break;
			}
		}
	}

	return msgNum;
}

/*
 * This function msgSend places a message on the queue.
 *
 *   msgName = Message queue name.
 *   msg = Message to place on queue.
 *
 *   returns -1 on error
 *           0 on success
 */
int msgSend(const char *msgName, const char *msg, int msgFlags) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	int msgLen = strlen(msg);

	if (msgLen > MAX_MSG_SIZE) {
		fprintf(stderr, "%s (%d): Max message size is %d.\n", __FILE__, __LINE__, MAX_MSG_SIZE);
		return ret;
	}

	MsgBuf msgBuf;
	msgBuf.type = 1;
	strcpy(msgBuf.data, msg);

	MsgMaster *mqp = msgMaster;
	for (int i = 0; i < MAX_MSGMASTERS; i++, mqp++) {
		if (mqp->inUse) {
			if (strcmp(mqp->msgName, msgName) == 0) {
				if (msgsnd(mqp->msgId, &msgBuf, msgLen + 1, msgFlags) == -1) {
					fprintf(stderr, "%s (%d): Failed to place message on queue.\n", __FILE__, __LINE__);
					perror("msgsnd");
				} else {
					ret = 0;
				}
				break;
			}
		}
	}

	return ret;
}

/*
 * This function msgNumSend places a message on the queue.
 *
 *   msgName = Message queue name.
 *   msg = Message to place on queue.
 *   msgLen = number of bytes form msg to place on message queue.
 *
 *   returns -1 on error
 *           0 on success
 */
int msgNumSend(const char *msgName, const char *msg, int msgLen, int msgFlags) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	if (msgLen > MAX_MSG_SIZE) {
		fprintf(stderr, "%s (%d): Max message size is %d.\n", __FILE__, __LINE__, MAX_MSG_SIZE);
		return ret;
	}

	MsgBuf msgBuf;
	msgBuf.type = 1;
	memcpy(msgBuf.data, msg, msgLen);

	MsgMaster *mqp = msgMaster;
	for (int i = 0; i < MAX_MSGMASTERS; i++, mqp++) {
		if (mqp->inUse) {
			if (strcmp(mqp->msgName, msgName) == 0) {
				if (msgsnd(mqp->msgId, &msgBuf, msgLen + 1, msgFlags) == -1) {
					fprintf(stderr, "%s (%d): Failed to place message on queue.\n", __FILE__, __LINE__);
					perror("msgsnd");
				} else {
					ret = 0;
				}
				break;
			}
		}
	}

	return ret;
}

/*
 * This function msgFastSend places a message on the queue.
 * Uses the msgNum gotten from the msgGetNum()function.
 *
 *   msgNum = Master message queue array offset.
 *   msg = Message to place on queue.
 *
 *   returns -1 on error
 *           0 on success
 */
int msgFastSend(int msgNum, const char *msg, int msgFlags) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	int msgLen = strlen(msg);

	if (msgLen > MAX_MSG_SIZE) {
		fprintf(stderr, "%s (%d): Max message size is %d.\n", __FILE__, __LINE__, MAX_MSG_SIZE);
		return ret;
	}

	MsgBuf msgBuf;
	msgBuf.type = 1;
	strcpy(msgBuf.data, msg);

	if (msgNum < 0 || msgNum >= MAX_MSGMASTERS ) {
		fprintf(stderr, "%s (%d): msgNum out of range %d.\n", __FILE__, __LINE__, msgNum);
		return ret;
	}

	MsgMaster *mqp = &msgMaster[msgNum];
	if (mqp->inUse) {
		if (msgsnd(mqp->msgId, &msgBuf, msgLen + 1, msgFlags) == -1) {
			fprintf(stderr, "%s (%d): Failed to place message on queue.\n", __FILE__, __LINE__);
			perror("msgsnd");
		} else {
			ret = 0;
		}
	} else {
		fprintf(stderr, "%s (%d): msgNum not in use %d.\n", __FILE__, __LINE__, msgNum);
	}

	return ret;
}

/*
 * This function msgNumSend places a message on the queue.
 * Uses the msgNum gotten from the msgGetNum()function.
 *
 *   msgNum = Master message queue array offset.
 *   msg = Message to place on queue.
 *   msgLen = number of bytes form msg to place on message queue.
 *
 *   returns -1 on error
 *           0 on success
 */
int msgFastNumSend(int msgNum, const char *msg, int msgLen, int msgFlags) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	if (msgLen > MAX_MSG_SIZE) {
		fprintf(stderr, "%s (%d): Max message size is %d.\n", __FILE__, __LINE__, MAX_MSG_SIZE);
		return ret;
	}

	MsgBuf msgBuf;
	msgBuf.type = 1;
	memcpy(msgBuf.data, msg, msgLen);

	if (msgNum < 0 || msgNum >= MAX_MSGMASTERS ) {
		fprintf(stderr, "%s (%d): msgNum out of range %d.\n", __FILE__, __LINE__, msgNum);
		return ret;
	}

	MsgMaster *mqp = &msgMaster[msgNum];
	if (mqp->inUse) {
		if (msgsnd(mqp->msgId, &msgBuf, msgLen + 1, msgFlags) == -1) {
			fprintf(stderr, "%s (%d): Failed to place message on queue.\n", __FILE__, __LINE__);
			perror("msgsnd");
		} else {
			ret = 0;
		}
	} else {
		fprintf(stderr, "%s (%d): msgNum not in use %d.\n", __FILE__, __LINE__, msgNum);
	}

	return ret;
}

/*
 * This function msgRecv reads first message on queue without priority.
 * Read the next message on the queue regardless if type.
 *
 *   msgName = Message queue name.
 *   buf = Buffer to place message into.
 *
 *   returns -1 on error
 *           0 if no messages in queue.
 *           else number of bytes placed into buf.
 */
int msgRecv(const char *msgName, char *buf, int msgFlags) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	MsgBuf msgBuf;
	long msgPriority = 0;

	MsgMaster *mqp = msgMaster;
	for (int i = 0; i < MAX_MSGMASTERS; i++, mqp++) {
		if (mqp->inUse) {
			if (strcmp(mqp->msgName, msgName) == 0) {
				size_t r = msgrcv(mqp->msgId, &msgBuf, sizeof(msgBuf.data), msgPriority, msgFlags);
				if (r == -1 && errno != ENOMSG) {
					fprintf(stderr, "%s (%d): Failed to read message from queue.\n", __FILE__, __LINE__);
					perror("msgrcv");
				} else if (r == -1 && errno == ENOMSG) {
					ret = 0;
				} else {
					memcpy(buf, msgBuf.data, r);
					ret = r;
				}
				break;
			}
		}
	}

	return ret;
}

/*
 * This function msgRecv reads first message on queue with priority.
 * If no priority message in queue then returns next message.
 *
 *   msgName = Message queue name.
 *   buf = Buffer to place message into.
 *   msgPriority = Message type to read.
 *
 *   returns -1 on error
 *            0 no priority or any message found on queue.
 *            else number of bytes placed into buf.
 */
int msgPriorityRecv(const char *msgName, char *buf, int msgFlags, long msgPriority) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	MsgBuf msgBuf;

	MsgMaster *mqp = msgMaster;
	for (int i = 0; i < MAX_MSGMASTERS; i++, mqp++) {
		if (mqp->inUse) {
			if (strcmp(mqp->msgName, msgName) == 0) {
				size_t r = msgrcv(mqp->msgId, &msgBuf, sizeof(msgBuf.data), msgPriority, msgFlags);
				if (r == -1 && errno != ENOMSG) {
					fprintf(stderr, "%s (%d): Failed to read message from queue.\n", __FILE__, __LINE__);
					perror("msgrcv");
				} else if (r == -1 && errno == ENOMSG) {
					// No priority messages, try and read the next message in queue.
					size_t r2 = msgrcv(mqp->msgId, &msgBuf, sizeof(msgBuf.data), 0, msgFlags);
					if (r2 == -1 && errno != ENOMSG) {
						fprintf(stderr, "%s (%d): Failed to read message from queue.\n", __FILE__, __LINE__);
						perror("msgrcv");
					} else if (r2 == -1 && errno == ENOMSG) {
						ret = 0;
					} else {
						memcpy(buf, msgBuf.data, r2);
						ret = r2;
					}
				} else {
					memcpy(buf, msgBuf.data, r);
					ret = r;
				}
				break;
			}
		}
	}

	return ret;
}

/*
 * This function msgRecv reads first message on queue without priority.
 * Read the next message on the queue regardless if type.
 *
 *   msgName = Message queue name.
 *   buf = Buffer to place message into.
 *
 *   returns -1 on error
 *           0 if no messages in queue.
 *           else number of bytes placed into buf.
 */
int msgFastRecv(int msgNum, char *buf, int msgFlags) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	MsgBuf msgBuf;
	long msgPriority = 0;

	if (msgNum < 0 || msgNum >= MAX_MSGMASTERS ) {
		fprintf(stderr, "%s (%d): msgNum out of range %d.\n", __FILE__, __LINE__, msgNum);
		return ret;
	}

	MsgMaster *mqp = &msgMaster[msgNum];
	if (mqp->inUse) {
		size_t r = msgrcv(mqp->msgId, &msgBuf, sizeof(msgBuf.data), msgPriority, msgFlags);
		if (r == -1 && errno != ENOMSG) {
			fprintf(stderr, "%s (%d): Failed to read message from queue.\n", __FILE__, __LINE__);
			perror("msgrcv");
		} else if (r == -1 && errno == ENOMSG) {
			ret = 0;
		} else {
			memcpy(buf, msgBuf.data, r);
			ret = r;
		}
	} else {
		fprintf(stderr, "%s (%d): msgNum not in use %d.\n", __FILE__, __LINE__, msgNum);
	}

	return ret;
}

/*
 * This function msgRecv reads first message on queue with priority.
 * If no priority message in queue then returns next message.
 *
 *   msgName = Message queue name.
 *   buf = Buffer to place message into.
 *   msgPriority = Message type to read.
 *
 *   returns -1 on error
 *            0 no priority or any message found on queue.
 *            else number of bytes placed into buf.
 */
int msgFastPriorityRecv(int msgNum, char *buf, int msgFlags, long msgPriority) {
	int ret = -1;

	if (msgMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call msgInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	MsgBuf msgBuf;

	if (msgNum < 0 || msgNum >= MAX_MSGMASTERS ) {
		fprintf(stderr, "%s (%d): msgNum out of range %d.\n", __FILE__, __LINE__, msgNum);
		return ret;
	}

	MsgMaster *mqp = &msgMaster[msgNum];
	if (mqp->inUse) {
		size_t r = msgrcv(mqp->msgId, &msgBuf, sizeof(msgBuf.data), msgPriority, msgFlags);
		if (r == -1 && errno != ENOMSG) {
			fprintf(stderr, "%s (%d): Failed to read message from queue.\n", __FILE__, __LINE__);
			perror("msgrcv");
		} else if (r == -1 && errno == ENOMSG) {
			// No priority messages, try and read the next message in queue.
			size_t r2 = msgrcv(mqp->msgId, &msgBuf, sizeof(msgBuf.data), 0, msgFlags);
			if (r2 == -1 && errno != ENOMSG) {
				fprintf(stderr, "%s (%d): Failed to read message from queue.\n", __FILE__, __LINE__);
				perror("msgrcv");
			} else if (r2 == -1 && errno == ENOMSG) {
				ret = 0;
			} else {
				memcpy(buf, msgBuf.data, r2);
				ret = r2;
			}
		} else {
			memcpy(buf, msgBuf.data, r);
			ret = r;
		}
	} else {
		fprintf(stderr, "%s (%d): msgNum not in use %d.\n", __FILE__, __LINE__, msgNum);
	}

	return ret;
}
