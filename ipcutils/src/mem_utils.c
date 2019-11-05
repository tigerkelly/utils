
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
#include <sys/shm.h>
#include <sys/sem.h>

#include "ipcutils.h"

/* Pointer to Actual shared memory that hold the Master Segment*/
MemMaster *sharedMemMaster = NULL;
/* Copy of shared memory Master Segment - allocated for each process */
MemMaster *memMaster = NULL;

key_t memMasterId;

key_t masterSemId;

/*
 * This function memInit attaches or creates the master memory segment.
 * This segment keeps track of all segments created through this library.
 *
 * returns  0 on success.
 *         -1 on failure.
 */
int memInit() {
	// Attach or create the master memory segment.
	memMasterId = shmget(MEMMASTER_ID,
			(MAX_MEMMASTERS * sizeof(MemMaster)),
			0666);

	if (memMasterId == -1) {
		if (errno == ENOENT) {
			// Memory master not allocated yet.

			memMasterId = shmget(MEMMASTER_ID,
					(MAX_MEMMASTERS * sizeof(MemMaster)),
					(IPC_CREAT | IPC_EXCL | 0666));
		} else {
			fprintf(stderr, "%s (%d): Could create master shared memory segment.\n", __FILE__, __LINE__);
			return -1;
		}
	}

	 sharedMemMaster = (MemMaster *)shmat(memMasterId, NULL, 0);
	if (sharedMemMaster == (void*)-1) {
		fprintf(stderr, "%s (%d): failed to attach to master shared memory.", __FILE__, __LINE__);
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
			fprintf(stderr, "%s (%d): Could not create master semaphore.  errno: %d\n",
					__FILE__, __LINE__, errno);
			perror("memInit:");
			return -1;
		}
	}
        memMaster = calloc(1, MAX_MEMMASTERS * sizeof(MemMaster));
        memcpy(memMaster, sharedMemMaster, MAX_MEMMASTERS * sizeof(MemMaster)); 
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

	sb[0].sem_num = SHM_INDEX;
	sb[0].sem_op = 0;			// wait for semaphore to become zero.
	sb[0].sem_flg = 0;

	sb[1].sem_num = SHM_INDEX;
	sb[1].sem_op = 1;			// lock semaphore
	sb[1].sem_flg = SEM_UNDO;

	if (semop(masterSemId, sb, 2) == -1) {
		fprintf(stderr, "%s (%d): Could not lock master semaphore number [%d] errno: %d.\n",
				__FILE__, __LINE__, SHM_INDEX, errno);
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
static int unlockMasterSem() {
	struct sembuf sb;

	sb.sem_num = SHM_INDEX;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;

	if (semop(masterSemId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number [%d] errno: %d.\n",
				__FILE__, __LINE__, SHM_INDEX, errno);
		perror("unlockMasterSem:");
		return -1;
	}

	return 0;
}

/*
 * This function is used to lock the maemory semaphore.
 * This single threads access to the master memory array.
 *
 * returns -1 on error
 *         0 on success
 */
int memLockMemory() {
	struct sembuf sb[2];

	sb[0].sem_num = MEM_LOCK_INDEX;
	sb[0].sem_op = 0;			// wait for semaphore to become zero.
	sb[0].sem_flg = 0;

	sb[1].sem_num = MEM_LOCK_INDEX;
	sb[1].sem_op = 1;			// lock semaphore
	sb[1].sem_flg = SEM_UNDO;

	if (semop(masterSemId, sb, 2) == -1) {
		fprintf(stderr, "%s (%d): Could not lock master semaphore number [%d]. errno: %d\n",
				__FILE__, __LINE__, MEM_LOCK_INDEX, errno);
		perror("memLockMemory:");
		return -1;
	}

	return 0;
}

/*
 * This function unlockMemory is used to unlock the memory semaphore.
 *
 * returns -1 on error
 *         0 on success
 */
int memUnlockMemory() {
	struct sembuf sb;

	sb.sem_num = MEM_LOCK_INDEX;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;

	if (semop(masterSemId, &sb, 1) == -1) {
		fprintf(stderr, "%s (%d): Could not unlock semaphore number [%d] errno: %d.\n",
				__FILE__, __LINE__, MEM_LOCK_INDEX, errno);
		perror("memUnlockMemory:");
		return -1;
	}

	return 0;
}

/*
 * This function allocateShm updates the MemMasters segment and creates
 * the user shared memory segment if needed.
 * NOTE: This function is private to this library and should not be called by user.
 *
 *   mmp = Pointer to MemMaster entry for this user segment.
 *   memSize = size of shared memory to allocate for segment.
 *
 *   returns -1 on failure
 *           0 on success
 */
int allocateShm(MemMaster *mmp, long memSize) {
	// Attach or create the user memory segment.
	mmp->shmId = shmget(mmp->id, memSize, 0666);

	if (mmp->shmId == -1) {
		if (errno == ENOENT) {
			// Memory segment not allocated yet.

			mmp->shmId = shmget(mmp->id, memSize,
					(IPC_CREAT | IPC_EXCL | 0666));
		} else {
			fprintf(stderr, "%s (%d): Could not create shared memory segment '%s'.\n",
					__FILE__, __LINE__, mmp->memName);
			return -1;
		}
	} else {
		struct shmid_ds ds;
		shmctl(mmp->shmId, IPC_STAT, &ds);
#if (__SIZEOF_LONG__ == 4)
		fprintf(stdout, "%s (%d): Size of shared memory segment: %u\n",
				__FILE__, __LINE__, ds.shm_segsz);
#else
		fprintf(stdout, "%s (%d): Size of shared memory segment: %lu\n",
				__FILE__, __LINE__, ds.shm_segsz);
#endif
	}

	return 0;
}

/*
 * This function memGetNum returns a shared memory memId.
 * Use this function to get the memId.
 *
 *   msgName = Message queue name to get.
 *
 *   returns -1 on error or not found
 *           else master memory array offset.
 */
int memGetNum(const char *memName) {
	int memNum = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return memNum;
	}

	MemMaster *mp = memMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mp++) {
		if (mp->inUse) {
			if (strcmp(mp->memName, memName) == 0) {
				memNum = i;
				break;
			}
		}
	}

	return memNum;
}

/*
 * This function memCreate creates or attaches to existing segment.
 *
 *   memName = Segment name to attach to or create.
 *   memSize = Size of segment to create if is does not exist.
 *
 *   returns -1 on failure
 *   		 -2 memSize does not match found segment.
 *           0 if segment already exists
 *           1 if segment was created.
 */
int memCreate(const char *memName, long memSize) {
	int ret = -1;
        int index = 0;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	if (strlen(memName) > (MAX_SEGNAME -1)) {
		fprintf(stderr, "%s (%d): Segment name is too long, %d max.\n", __FILE__, __LINE__, MAX_SEGNAME);
		return ret;
	}

	lockMasterSem();

	int foundIt = 0;
	MemMaster *foundSpot = NULL;
	MemMaster *mp = sharedMemMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mp++) {
		if (mp->inUse == 0 && foundSpot == NULL) {
			foundSpot = mp;
			foundSpot->id = i + 100;
                        index = i;
		} else if (mp->inUse) {
			if (strcmp(memName, mp->memName) == 0) {
				struct shmid_ds buf;

				shmctl(mp->shmId, IPC_STAT, &buf);
				if (buf.shm_segsz != memSize) {
					unlockMasterSem();
					return -2;
				}
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
			foundSpot->memSize = memSize;
			strcpy(foundSpot->memName, memName);

			allocateShm(foundSpot, memSize);
            memcpy(&memMaster[index], foundSpot, sizeof(MemMaster));
			ret = 1;
		}
	}

	unlockMasterSem();

	return ret;
}

/*
 * This function memAttach returns a memory pointer to segment.
 *
 *   memName = Segment name to get pointer to.
 *
 *   returns NULL on error
 *           else pointer to shared memory segment.
 */
void *memAttach(const char *memName) {
	void *ptr = NULL;

	if (sharedMemMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return NULL;
	}

	MemMaster *mmp = sharedMemMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mmp++) {
		if (mmp->inUse) {
			if (strcmp(mmp->memName, memName) == 0) {

				memMaster[i].shmPtr = (unsigned char *)shmat(mmp->shmId, NULL, 0);
                ptr = memMaster[i].shmPtr;
				if (memMaster[i].shmPtr == (void*)-1) {
					fprintf(stderr, "%s (%d): failed to attach to shared memory segment '%s'.\n",
							__FILE__, __LINE__, mmp->memName);
                                        perror("shm error:\n");
					return NULL;
				}
                memMaster[i].id = sharedMemMaster[i].id;
                memMaster[i].inUse = sharedMemMaster[i].inUse;
                memMaster[i].usingCnt = sharedMemMaster[i].usingCnt;
                strcpy(memMaster[i].memName, sharedMemMaster[i].memName);
                memMaster[i].memSize = sharedMemMaster[i].memSize;
                memMaster[i].shmId = sharedMemMaster[i].shmId;
				break;
			}
		}
	}

	return ptr;
}

/*
 * This function memWrite is used to update the shared memory.
 * This uses a semaphore to keep processes from updating at the same time.
 *
 *   memName = Shared memory name
 *   data = Data to write to shared memory
 *   offset = Offset within the shared memory to write the data.
 *   length = Length of the data to write.
 *
 *   return -1 on error
 *          0 on success
 */
int memWrite(const char *memName, const char *data, int offset, int length) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	MemMaster *mmp = memMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mmp++) {
		if (mmp->inUse) {
			if (strcmp(mmp->memName, memName) == 0) {
				if (mmp->memSize < (long)(offset + length)) {
					fprintf(stderr, "%s (%d): Offset plus length is greater than shared memory size.\n",
							__FILE__, __LINE__);
					return -1;
				}

				memLockMemory();
				memcpy((mmp->shmPtr + offset), data, length);
				memUnlockMemory();
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

/*
 * This function memRead is used to read data from shared memory.
 * This uses a semaphore to keep processes from reading at the same time.
 *
 *   memName = Shared memory name
 *   data = place to return data
 *   offset = Offset within the shared memory to read the data.
 *   length = Length of the data to read.
 *
 *   return -1 on error
 *          0 on success
 */
int memRead(const char *memName, char *data, int offset, int length) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	MemMaster *mmp = memMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mmp++) {
		if (mmp->inUse) {
			if (strcmp(mmp->memName, memName) == 0) {
				if (mmp->memSize < (long)(offset + length)) {
					fprintf(stderr, "%s (%d): Offset plus length is greater than shared memory size.\n",
							__FILE__, __LINE__);
					return -1;
				}

				memLockMemory();
				memcpy(data, (mmp->shmPtr + offset), length);
				memUnlockMemory();
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

/**
 * This function reads data from SHM without lock
 */
int memReadWL(const char *memName, char *data, int offset, int length) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	MemMaster *mmp = memMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mmp++) {
		if (mmp->inUse) {
			if (strcmp(mmp->memName, memName) == 0) {
				if (mmp->memSize < (long)(offset + length)) {
					fprintf(stderr, "%s (%d): Offset plus length is greater than shared memory size.\n",
							__FILE__, __LINE__);
					return -1;
				}

				memcpy(data, (mmp->shmPtr + offset), length);
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

/*
 * This function memWrite is used to update the shared memory.
 * This uses a semaphore to keep processes from updating at the same time.
 * Use the memGetNum() to retrieve the msgNum
 *
 *   memNum = Master shared memory array offset.
 *   data = Data to write to shared memory
 *   offset = Offset within the shared memory to write the data.
 *   length = Length of the data to write.
 *
 *   return -1 on error
 *          0 on success
 */
int memFastWrite(int memNum, const char *data, int offset, int length) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	if (memNum < 0 || memNum >= MAX_MEMMASTERS ) {
		fprintf(stderr, "%s (%d): memNum out of range %d.\n", __FILE__, __LINE__, memNum);
		return ret;
	}

	MemMaster *mmp = &memMaster[memNum];
	if (mmp->inUse) {
		if (mmp->memSize < (long)(offset + length)) {
			fprintf(stderr, "%s (%d): Offset plus length is greater than shared memory size.\n",
					__FILE__, __LINE__);
			return -1;
		}

		memLockMemory();
		memcpy((mmp->shmPtr + offset), data, length);
		memUnlockMemory();
		ret = 0;
	} else {
		fprintf(stderr, "%s (%d): memNum not in use %d.\n", __FILE__, __LINE__, memNum);
	}

	return ret;
}

/*
 * This function memFastRead is used to read data from shared memory.
 * This uses a semaphore to keep processes from reading at the same time.
 * Use the memGetNum() function to get memNum.
 *
 *   memName = Shared memory name
 *   data = place to return data
 *   offset = Offset within the shared memory to read the data.
 *   length = Length of the data to read.
 *
 *   return -1 on error
 *          0 on success
 */
int memFastRead(int memNum, char *data, int offset, int length) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	if (memNum < 0 || memNum >= MAX_MEMMASTERS ) {
		fprintf(stderr, "%s (%d): memNum out of range %d.\n", __FILE__, __LINE__, memNum);
		return ret;
	}

	MemMaster *mmp = &memMaster[memNum];
	if (mmp->inUse) {
		if (mmp->memSize < (long)(offset + length)) {
			fprintf(stderr, "%s (%d): Offset plus length is greater than shared memory size.\n",
					__FILE__, __LINE__);
			return -1;
		}

		memLockMemory();
		memcpy(data, (mmp->shmPtr + offset), length);
		memUnlockMemory();
		ret = 0;
	} else {
		fprintf(stderr, "%s (%d): memNum not in use %d.\n", __FILE__, __LINE__, memNum);
	}

	return ret;
}

/*
 * This function memAddrWrite is used to update the shared memory.
 * This uses a semaphore to keep processes from updating at the same time.
 *
 *   shmAddr = Valid address in the range of memAttach to memAttach + memSize
 *   data = Data to write to shared memory
 *   length = Length of the data to write.
 *
 *   return -1 on error
 *          0 on success
 */
int memAddrWrite(char *shmAddr, const char *data, int length) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	memLockMemory();
	memcpy(shmAddr, data, length);
	memUnlockMemory();

	return ret;
}

/*
 * This function memRead is used to read data from shared memory.
 * This uses a semaphore to keep processes from reading at the same time.
 *
 *   shmAddr = Valid address in the range of memAttach() to memAttach() + memSize
 *   data = place to return data
 *   length = Length of the data to read.
 *
 *   return -1 on error
 *          0 on success
 */
int memAddrRead(char *shmAddr, char *data, int length) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	memLockMemory();
	memcpy(data, shmAddr, length);
	memUnlockMemory();

	return ret;
}

/*
 * This function memDetach detaches for shared memory segment.
 * Once detached you can not longer use the shared memory segment.
 *
 *   memName = Segment name to detach from.
 *
 *   returns void
 */
void memDetach(const char *memName) {
	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return;
	}

	lockMasterSem();

	MemMaster *mmp = sharedMemMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mmp++) {
		if (mmp->inUse) {
			if (strcmp(mmp->memName, memName) == 0) {
				shmdt((void *)memMaster[i].shmPtr);
				mmp->usingCnt--;
				if (mmp->usingCnt < 0) {
					mmp->usingCnt = 0;
				}
				break;
			}
		}
	}

	unlockMasterSem();
}

/*
 * Place a formated string containing the name and size of each memory segment.
 * Buffer must be large enough to hold result.
 */
int memList(char *buffer) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	lockMasterSem();

	buffer[0] = '\0';

	int r = 0;
	MemMaster *mp = memMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mp++) {
		if (mp->inUse == 0)
			continue;

		r += sprintf(buffer + r, "Mem Seg Name: %-16s  Size: %ld\n", mp->memName, mp->memSize);
	}

	unlockMasterSem();

	return ret;
}

int memCheckAndSet(const char *memName, int offset, int length, char *checkVal, char *setVal) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return -1;
	}

	MemMaster *mmp = memMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mmp++) {
		if (mmp->inUse) {
			if (strcmp(mmp->memName, memName) == 0) {
				if (mmp->memSize < (long)(offset + length)) {
					fprintf(stderr, "%s (%d): Offset plus length is greater than shared memory size.\n",
							__FILE__, __LINE__);
					return -1;
				}

				memLockMemory();
				/* do byte by byte comparison and if it equals to checkVal set it to setVal atomically*/
				if (0 == memcmp((mmp->shmPtr + offset), checkVal, length)) {
					memcpy((mmp->shmPtr + offset), setVal, length);
					ret = 0;
					memUnlockMemory();
					break;
				}
				memUnlockMemory();
			}
		}
	}

	return ret;
}
/*
 * This function memGetSize returns a shared memory size.
 * Use this function to get the mem size.
 *
 *   msgName = Message queue name to get.
 *
 *   returns -1 on error or not found
 *           else master memory size.
 */
int memGetSize(const char *memName) {
	int memSize = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return memSize;
	}

	MemMaster *mp = memMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mp++) {
		if (mp->inUse) {
			if (strcmp(mp->memName, memName) == 0) {
				return mp->memSize;
				break;
			}
		}
	}

	return memSize;
}

int memDestroy(const char *memName) {
	int ret = -1;

	if (memMaster == NULL) {
		fprintf(stderr, "%s (%d): Need to call memInit() first.\n", __FILE__, __LINE__);
		return ret;
	}

	if (strlen(memName) > (MAX_SEGNAME -1)) {
		fprintf(stderr, "%s (%d): Segment name is too long, %d max.\n", __FILE__, __LINE__, MAX_SEGNAME);
		return ret;
	}

	lockMasterSem();

	MemMaster *mp = sharedMemMaster;
	for (int i = 0; i < MAX_MEMMASTERS; i++, mp++) {
	    if (strcmp(memName, mp->memName) == 0) {
            if(-1 == shmctl(mp->shmId, IPC_RMID, NULL)) {
                            memset(mp, 0, sizeof(MemMaster));
			    fprintf(stderr, "%s (%d): Shared Memory destroy failed\n", __FILE__, __LINE__);
	                    unlockMasterSem();
                return -1;
            } else {
                // clean shared mem 
                memset(mp, 0, sizeof(MemMaster));
                // clean copy shared mem 
                memset(&memMaster[i], 0, sizeof(MemMaster)); 
                break;
            }
	    }
	}

	unlockMasterSem();

	return 0;
}


