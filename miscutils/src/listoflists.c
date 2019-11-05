#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "miscutils.h"

#ifndef DEBUG
#define DEBUG 1
#endif

MList *mList = NULL;
int initDone = 0;
int mainListCount = 0;

pthread_mutex_t lolLock = PTHREAD_MUTEX_INITIALIZER;

int lolInit() {
	if (pthread_mutex_init(&lolLock, NULL) != 0) {
		pErr("Mutex init lock failed.\n");
		return -1;
	}
    initDone = 1;

	return 0;
}

MList *lolGetList() {
	return mList;
}

void keyCreate(char *aNum, char *callId, char *key) {
    strncpy(key, aNum, MAX_NUM);  // Remember that strncpy() pads bytes beyond the end of the string with zeroes.
    strncpy(key + MAX_NUM, callId,  MAX_CALLID);
}

int lolAdd(char *aNum, char *bNum, char *callId, char callType) {

	if (initDone == 0) {
		pErr("Must call lolInit() first.\n");
		return -1;
	}

	pthread_mutex_lock(&lolLock);

	if (mList == NULL) {
		mList = (MList *)calloc(1, sizeof(MList));
		Lol *lol = (Lol *)calloc(1, sizeof(Lol));

		keyCreate(aNum, callId, mList->key);
		mList->timestamp = time(0);

		if (bNum != NULL)
			strcpy(lol->bNum, bNum);
		lol->callType = callType;
		lol->timestamp = time(0);
		mList->lol = lol;
		mainListCount++;
	} else {
		MList *mp = mList;

		char key[MAX_KEY + 1];
		keyCreate(aNum, callId, key);

		int found = 0;
		while (mp != NULL) {
			if (memcmp(mp->key, key, MAX_KEY) == 0) {
				found = 1;
				break;
			}
			mp = mp->mNext;
		}

		if (found == 0) {
			MList *saveList = mList;
			MList *newList = (MList *)calloc(1, sizeof(MList));
			Lol *lol = (Lol *)calloc(1, sizeof(Lol));

			memcpy(newList->key, key, MAX_KEY);
			newList->timestamp = time(0);

			if (bNum != NULL)
				strcpy(lol->bNum, bNum);
			lol->callType = callType;
			lol->timestamp = time(0);
			newList->lol = lol;

			saveList->mPrev = newList;

			// place a head of list.
			mList = newList;
			mList->mNext = saveList;
			mainListCount++;
		} else {
			mp->timestamp = time(0);		// update time stamp.
			Lol *saveLol = mp->lol;
			Lol *newLol = (Lol *)calloc(1, sizeof(Lol));

			if (bNum != NULL)
				strcpy(newLol->bNum, bNum);
			newLol->callType = callType;

			// push on to head of list, newest ones are always near top.

			newLol->lolNext = saveLol;

			mp->lol = newLol;
		}
	}

	pthread_mutex_unlock(&lolLock);

	return 0;
}

void lolRemoveExpired(int seconds) {
	MList *mp = mList;
	time_t curTime = time(0);

	if (initDone == 0) {
		pErr("Must call lolInit() first.\n");
		return;
	}

	pthread_mutex_lock(&lolLock);

	while (mp != NULL) {
		MList *nextList = mp->mNext;
		if ((curTime - mp->timestamp) >= seconds) {
			if (mp->mPrev == NULL) {
				mList = mp->mNext;

				Lol *lp = mp->lol;
				while (lp != NULL) {

					lp = lp->lolNext;
					free(lp);
				}
				free(mp);
			} else {
				MList *savePrev = mp->mPrev;

				Lol *lp = mp->lol;
				while (lp != NULL) {

					lp = lp->lolNext;
					free(lp);
				}
				free(mp);
				savePrev->mNext = nextList;
				if (nextList != NULL)
					nextList->mPrev = savePrev;
			}
			mainListCount--;
		}
		mp = nextList;
	}
	pthread_mutex_unlock(&lolLock);
}

int lolMainListCount() {
	return mainListCount;
}

void lolPrint() {

	if (initDone == 0) {
		pErr("Must call lolInit() first.\n");
		return;
	}

	pthread_mutex_lock(&lolLock);

	char aNum[MAX_NUM + 1];
	char callId[MAX_CALLID + 1];

	MList *mp = mList;
	while (mp != NULL) {
		Lol *lp = mp->lol;
		memcpy(aNum, mp->key, MAX_NUM);
		memcpy(callId, &mp->key[MAX_NUM], MAX_CALLID);
		printf("%s %s %ld\n", aNum, callId, mp->timestamp);
		while (lp != NULL) {
			printf("  %s %s %c\n", lp->bNum, callId, lp->callType);
			lp = lp->lolNext;
		}
		mp = mp->mNext;
	}
	pthread_mutex_unlock(&lolLock);
}
