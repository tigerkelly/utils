
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
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "ipcutils.h"

SemMaster *semMaster = NULL;

key_t semMasterId;

key_t masterSemId;

#if __linux__
union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};
#endif

/*
 * This function semInit attaches or create the semaphore master memory segment.
 * This segment keeps track of all semaphores created through this library.
 *
 * returns  0 on success.
 *         -1 on failure.
 */
int semInit() {
	// Attach or create the master memory segment.
	semMasterId = shmget(SEMMASTER_ID,
			(MAX_SEMMASTERS * sizeof(SemMaster)),
			0666);

	if (semMasterId == -1) {
		if (errno == ENOENT) {
			// Memory master not allocated yet.

			semMasterId = shmget(SEMMASTER_ID,
					(MAX_SEMMASTERS * sizeof(SemMaster)),
					(IPC_CREAT | IPC_EXCL | 0666));
		} else {
			fprintf(stderr, "%s (%d): Could create master shared memory segment for semaphores.\n", __FILE__, __LINE__);
			return -1;
		}
	}

	semMaster = (SemMaster *)shmat(semMasterId, NULL, 0);
	if (semMaster == (void*)-1) {
		fprintf(stderr, "%s (%d): failed to attach to master shared memory for semaphores.", __FILE__, __LINE__);
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
int lockMasterSem() {
	struct sembuf sb[2];

	sb[0].sem_num = SEM_INDEX;
	sb[0].sem_op  = 0;			// wait for semaphore to become zero.
	sb[0].sem_flg = 0;

	sb[1].sem_num = SEM_INDEX;
	sb[1].sem_op  = 1;			// lock semaphore
	sb[1].sem_flg = SEM_UNDO;

	if (semop(masterSemId, sb, 2) == -1) {
		fprintf(stderr, "%s (%d): Could not lock master semaphore number [%d]  errno: %d.\n",
				__FILE__, __LINE__, SEM_INDEX, errno);
		perror("lockMasterSem:");
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
int unlockMasterSem() {
	struct sembuf sb;

	sb.sem_num = SEM_INDEX;
	sb.sem_op  = -1;
	sb.sem_flg = SEM_UNDO;

	if (semop(masterSemId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number [%d].\n",
				__FILE__, __LINE__, SEM_INDEX);
		return -1;
	}

	return 0;
}

/*
 * This function allocateSem updates the SemMasters segment and creates
 * the user semaphore set if needed.
 * NOTE: This function is private to this library and should not be called by user.
 *
 *   mmp = Pointer to MemMaster entry for this user segment.
 *   semSize = number of semaphores to allocate for semaphore set.
 *
 *   returns -1 on failure
 *           0 on success
 */
int allocateSem(SemMaster *smp, int semSize) {
	union semun arg;
	struct sembuf sb;
	struct semid_ds buf;

	// Attach or create the user semaphore set.
	smp->semId = semget(smp->id, semSize, 0666 | IPC_CREAT | IPC_EXCL);

	if (smp->semId >= 0) {		// we created it first.
		sb.sem_op = 1;
		sb.sem_flg = 0;
		arg.val = 1;



		for (sb.sem_num = 0; sb.sem_num < semSize; sb.sem_num++) {
			if (semop(smp->semId, &sb, 1) == -1) {
				int e = errno;
				semctl(smp->semId, 0, IPC_RMID);
				errno = e;
				return -1;
			}
		}
	} else if (errno == EEXIST) {		// someone created it first.
		int ready = 0;

		smp->semId = semget(smp->id, semSize, 0);
		if (smp->semId < 0) {
			fprintf(stderr, "%s (%d): Can not get semaphore ID.\n", __FILE__, __LINE__);
			return -1;
		}

		arg.buf = &buf;

		for(int i = 0; i < 5 && ready == 0; i++) {
			semctl(smp->semId, semSize-1, IPC_STAT, arg);
			if (arg.buf->sem_otime != 0) {
				ready = 1;
			} else {
				sleep(1);
			}
		}

		if (ready == 0) {
			fprintf(stderr, "%s (%d): Could not get semaphore ID.\n", __FILE__, __LINE__);
			// errno = ETIME;
			return -1;
		}
	}

	return 0;
}

/*
 * This function semCreate creates the semaphore set if needed.
 *
 *   segName = Semaphore set name to create.
 *   semSize = Number of semaphores in the semaphore set to create.
 *
 *   returns -1 on failure
 *           0 if semaphore set already exists
 *           1 if semaphore set was created.
 */
int semCreate(const char *semName, int semSize) {
	int ret = -1;

	if (semMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call semInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	if (strlen(semName) > (MAX_SEGNAME -1)) {
		fprintf(stderr, "%s (%d): Semaphore name is too long, %d max.\n", __FILE__, __LINE__, MAX_SEGNAME);
		return ret;
	}

	lockMasterSem();

	int foundIt = 0;
	SemMaster *foundSpot = NULL;
	SemMaster *mp = semMaster;
	for (int i = 0; i < MAX_SEMMASTERS; i++, mp++) {
		if (mp->inUse == 0 && foundSpot == NULL) {
			foundSpot = mp;
			foundSpot->id = i + 200;
		} else if (mp->inUse) {
			if (strcmp(semName, mp->semName) == 0) {
				foundIt = 1;
				ret = 0;
				break;
			}
		}
	}

	if (foundIt == 0) {
		if (foundSpot == NULL) {
			fprintf(stderr, "%s (%d): No free slot left in semaphore master memory segment.\n", __FILE__, __LINE__);
			ret = -1;
		} else {
			foundSpot->inUse = 1;
			foundSpot->semSize = semSize;
			strcpy(foundSpot->semName, semName);

			allocateSem(foundSpot, semSize);
			ret = 1;
		}
	}

	unlockMasterSem();

	return ret;
}

/*
 * This function semGetId returns a semaphore sets semId.
 *
 *   semName = Semaphore set name to get pointer to.
 *
 *   returns -1 on error or not found
 *           else semId.
 */
int semGetId(const char *semName) {
	int semId = -1;

	if (semMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call semInit() first.\n", __FILE__, __LINE__);
		return semId;
	}

	SemMaster *smp = semMaster;
	for (int i = 0; i < MAX_SEMMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->semName, semName) == 0) {
				semId = smp->semId;
				break;
			}
		}
	}

	return semId;
}

/*
 * This function semGetId returns a semaphore sets semId.
 *
 *   semName = Semaphore set name to get pointer to.
 *
 *   returns -1 on error or not found
 *           else master semaphore array offset.
 */
int semGetNum(const char *semName) {
	int semNum = -1;

	if (semMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call semInit() first.\n", __FILE__, __LINE__);
		return semNum;
	}

	SemMaster *smp = semMaster;
	for (int i = 0; i < MAX_SEMMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->semName, semName) == 0) {
				semNum = i;
				break;
			}
		}
	}

	return semNum;
}

/*
 * This function semLock will try to lock a semaphore.
 * This function only allows you to lock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 *
 * This will block until operation can be done.
 *
 *   semName = Name of the semaphore set to use.
 *   semIdx = index of the semaphore in the semaphore set to lock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semLock(const char *semName, int semIdx) {
	struct sembuf sb;

	sb.sem_num = semIdx;
	sb.sem_op  = -1;
	sb.sem_flg = SEM_UNDO;

	int semId = -1;
	SemMaster *smp = semMaster;
	for (int i = 0; i < MAX_SEMMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->semName, semName) == 0) {
				semId = smp->semId;
				break;
			}
		}
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set '%s'.\n",
				__FILE__, __LINE__, semName);
		return -1;
	}

	if (semop(semId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not lock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}

/*
 * This function semUnlock will try to unlock a semaphore.
 * This function only allows you to unlock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 *
 * This will block until operation can be done.
 *
 *   semName = Name of the semaphore set to use.
 *   semIdx = Index of the semaphore in the semaphore set to unlock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semUnlock(const char *semName, int semIdx) {
	struct sembuf sb;

	sb.sem_num = semIdx;
	sb.sem_op  = 1;
	sb.sem_flg = SEM_UNDO;

	int semId = -1;
	SemMaster *smp = semMaster;
	for (int i = 0; i < MAX_SEMMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->semName, semName) == 0) {
				semId = smp->semId;
				break;
			}
		}
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set '%s'.\n",
				__FILE__, __LINE__, semName);
		return -1;
	}

	if (semop(semId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}

/*
 * This function semFastLock will try to lock a semaphore.
 * This function only allows you to lock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 * Use the semGteNum() function to get semNum.
 *
 * This will block until operation can be done.
 *
 *   semNum = Master semaphore array offset
 *   semIdx = index of the semaphore in the semaphore set to lock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semFastLock(int semNum, int semIdx) {
	struct sembuf sb;

	if (semNum < 0 || semNum >= MAX_SEMMASTERS ) {
		fprintf(stderr, "%s (%d): semNum out of range %d.\n", __FILE__, __LINE__, semNum);
		return -1;
	}

	sb.sem_num = semNum;
	sb.sem_op  = -1;			// lock semaphore
	sb.sem_flg = SEM_UNDO;

	int semId = -1;
	SemMaster *smp = &semMaster[semNum];
	if (smp->inUse) {
		semId = smp->semId;
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set.\n",	__FILE__, __LINE__);
		return -1;
	}

	if (semop(semId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not lock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}

/*
 * This function semUnlock will try to unlock a semaphore.
 * This function only allows you to unlock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 * Use the semGteNum() function to get semNum.
 *
 * This will block until operation can be done.
 *
 *   semNum = Master semaphore array offset.
 *   semIdx = index of the semaphore in the semaphore set to unlock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semFastUnlock(int semNum, int semIdx) {
	struct sembuf sb;

	sb.sem_num = semIdx;
	sb.sem_op  = 1;
	sb.sem_flg = SEM_UNDO;

	if (semNum < 0 || semNum >= MAX_SEMMASTERS ) {
		fprintf(stderr, "%s (%d): semNum out of range %d.\n", __FILE__, __LINE__, semNum);
		return -1;
	}

	int semId = -1;
	SemMaster *smp = &semMaster[semNum];
	if (smp->inUse) {
		semId = smp->semId;
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set.\n",	__FILE__, __LINE__);
		return -1;
	}

	if (semop(semId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}

#ifdef _GNU_SOURCE		// use compiler switch -D_GNU_SOURCE
/*
 * This function semTimedLock will try to lock a semaphore.
 * This function only allows you to lock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 *
 * This will block until operation can be done or timeout in seconds has been reached.
 *
 *   semNum = Master semaphore array offset
 *   semIdx = number of the semaphore in the semaphore set to lock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semTimedLock(const char *semName, int semIdx, int timeout) {
	struct sembuf sb;
	struct timespec ts;

	sb.sem_num = semIdx;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;

	int semId = -1;
	SemMaster *smp = semMaster;
	for (int i = 0; i < MAX_SEMMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->semName, semName) == 0) {
				semId = smp->semId;
				break;
			}
		}
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set '%s'.\n",
				__FILE__, __LINE__, semName);
		return -1;
	}

	ts.tv_sec = timeout;
	ts.tv_nsec = 0;
	if (semtimedop(semId, &sb, 1, &ts) == -1) {
		fprintf(stderr, "%s (%d): Could not lock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}

/*
 * This function semTimedUnlock will try to unlock a semaphore.
 * This function only allows you to unlock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 *
 * This will block until operation can be done or timeout in seconds has been reached.
 *
 *   semNum = Master semaphore array offset.
 *   semIdx = number of the semaphore in the semaphore set to unlock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semTimedUnlock(const char *semName, int semIdx, int timeout) {
	struct sembuf sb;
	struct timespec ts;

	sb.sem_num = semIdx;
	sb.sem_op  = 1;
	sb.sem_flg = SEM_UNDO;

	int semId = -1;
	SemMaster *smp = semMaster;
	for (int i = 0; i < MAX_SEMMASTERS; i++, smp++) {
		if (smp->inUse) {
			if (strcmp(smp->semName, semName) == 0) {
				semId = smp->semId;
				break;
			}
		}
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set '%s'.\n",
				__FILE__, __LINE__, semName);
		return -1;
	}

	ts.tv_sec = timeout;
	ts.tv_nsec = 0;
	if (semtimedop(semId, &sb, 1, &ts) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}

/*
 * This function semFastTimedLock will try to lock a semaphore.
 * This function only allows you to lock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 *
 * This will block until operation can be done or timeout in seconds has been reached.
 *
 *   semNum = Master semaphore array offset.
 *   semIdx = number of the semaphore in the semaphore set to lock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semFastTimedLock(int semNum, int semIdx, int timeout) {
	struct sembuf sb;
	struct timespec ts;

	sb.sem_num = semIdx;
	sb.sem_op  = -1;
	sb.sem_flg = SEM_UNDO;

	if (semNum < 0 || semNum >= MAX_SEMMASTERS ) {
		fprintf(stderr, "%s (%d): semNum out of range %d.\n", __FILE__, __LINE__, semNum);
		return -1;
	}

	int semId = -1;
	SemMaster *smp = &semMaster[semNum];
	if (smp->inUse) {
		semId = smp->semId;
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set.\n",	__FILE__, __LINE__);
		return -1;
	}

	ts.tv_sec = timeout;
	ts.tv_nsec = 0;
	if (semtimedop(semId, &sb, 1, &ts) == -1) {
		fprintf(stderr, "%s (%d): Could not lock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}

/*
 * This function semFastTimedUnlock will try to unlock a semaphore.
 * This function only allows you to unlock one semaphore in the set at a time.
 * You will have to use semGetId and handle multiple operations on multiple semaphores at the same time.
 *
 * This will block until operation can be done or timeout in seconds has been reached.
 *
 *   semNum = Master semaphore array offset.
 *   semIdx = number of the semaphore in the semaphore set to unlock.
 *
 *   returns -1 on failure.
 *           0 on success
 */
int semFastTimedUnlock(int semNum, int semIdx, int timeout) {
	struct sembuf sb;
	struct timespec ts;

	sb.sem_num = semIdx;
	sb.sem_op  = 1;
	sb.sem_flg = SEM_UNDO;

	if (semNum < 0 || semNum >= MAX_SEMMASTERS ) {
		fprintf(stderr, "%s (%d): semNum out of range %d.\n", __FILE__, __LINE__, semNum);
		return -1;
	}

	int semId = -1;
	SemMaster *smp = &semMaster[semNum];
	if (smp->inUse) {
		semId = smp->semId;
	}

	if (semId == -1) {
		fprintf(stderr, "%s (%d): Could not find semaphore set.\n",	__FILE__, __LINE__);
		return -1;
	}

	ts.tv_sec = timeout;
	ts.tv_nsec = 0;
	if (semtimedop(semId, &sb, 1, &ts) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number (%d).\n",
				__FILE__, __LINE__, semIdx);
		return -1;
	}

	return 0;
}
#endif		// _GNU_SOURCE
