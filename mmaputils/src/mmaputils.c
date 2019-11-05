/*
 * mmaputils.c
 *
 * Description:
 *  Created on: Sep 4, 2017
 *      Author: Kelly Wiles
 *   Copyright: Network Kinetix
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "mmaputils.h"
#include "ipcutils.h"
#include "logutils.h"
#include "nkxutils.h"

typedef struct _imdbSchema {
	char schemaName[32];
	int fieldLen;
	MmapField *fields;
} ImdbSchema;

ImdbSchema _schema;

int _mmapFd = -1;
int _semId = -1;
int _semNum = -1;

// NOTE: Total mapped file size must be less then physical memory minus OS over head.

void *mmapInit(char *fileName, unsigned int sizeInPages) {

	void *vp = NULL;
	struct stat st;

	int pageSize = getpagesize();

	int r = access(fileName, F_OK);

#ifdef __ARM_ARCH
	printf("pageSize: %d, %d\n", pageSize, r);
#else
	printf("pageSize: %d, %d\n", pageSize, r);
#endif

	if (r == -1) {
		printf("Creating file.\n");
		_mmapFd = open(fileName, O_RDWR | O_CREAT, 0644);

		char *blk = (char *)calloc(1, pageSize);

		// Increase file size.
//		lseek(_mmapFd, (sizeInPages * pageSize), SEEK_SET);
//		ftruncate(_mmapFd, (sizeInPages * pageSize));
//		if( lseek(_mmapFd, (sizeInPages * pageSize), SEEK_SET) == -1) {
//			printf("Failed to increase file size. %d, %s\n", errno, strerror(errno));
//		}
		for (int i = 0; i < sizeInPages; i++) {
			r = write(_mmapFd, blk, pageSize);
			if (r != pageSize) {
				Err("Create of file failed. %d, %s\n", errno, strerror(r));
				free(blk);
				return NULL;
			}
		}

		lseek(_mmapFd, 0, SEEK_SET);		// seek back to start of file.

		free(blk);
	} else {
		_mmapFd = open(fileName, O_RDWR);
	}

	int mmapFlags = PROT_WRITE | PROT_READ;

	r = stat(fileName, &st);
	if (r != -1) {
#ifdef __ARM_ARCH
		printf("File size: %lld\n", st.st_size);
#else
		printf("File size: %ld\n", st.st_size);
#endif
	}

	semInit();

	semCreate("imdbSem", 2);

	_semId = semGetId("imdbSem");
	_semNum = semGetNum("imdbSem");

	unsigned long size = (sizeInPages * pageSize);

#ifdef __ARM_ARCH
	printf("unsigned long size: %u\n", sizeof(unsigned long));
	printf("size: %lu, %d, %d, %u\n", size, sizeInPages, pageSize, (sizeInPages * pageSize));
#else
	printf("unsigned long size: %lu\n", sizeof(unsigned long));
	printf("size: %lu, %d, %d, %u\n", size, sizeInPages, pageSize, (sizeInPages * pageSize));
#endif

	vp = mmap(0, size, mmapFlags, MAP_SHARED, _mmapFd, 0);
	if (vp == MAP_FAILED) {
		printf("Failed to mmap file. %d, %s\n", errno, strerror(errno));
		return NULL;
	}

	return vp;
}

int mmapLock() {
	return semFastLock(_semNum, _semId);
}

int mmapUnlock() {
	return semFastUnlock(_semNum, _semId);
}

