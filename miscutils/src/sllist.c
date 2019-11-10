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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "miscutils.h"

int maxSortedLists;
SLList *sortedLists = NULL;

pthread_mutex_t createSortLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * This function initializes the Linked List array.
 *
 *   listCnt = max number of named llist you can have.
 *
 *   return -1 on error
 *          0 on success
 */
int sllInit(int listCnt) {
    if (listCnt == 1)
        maxSortedLists = 2;
    else if (listCnt <= 0)
		maxSortedLists = MAX_LLISTS;
	else
		maxSortedLists = listCnt;

	sortedLists = (SLList *)calloc(maxSortedLists, sizeof (SLList));
	if (sortedLists == NULL) {
		pErr("Can not allocate Linked List memory.\n");
		return -1;
	}

	SLList *sp = sortedLists;
	for (int i = 0; i < maxSortedLists; i++, sp++) {

		if (pthread_mutex_init(&(sp->sortLock), NULL) != 0) {
			pErr("Mutex init sortLock failed.\n");
			return -1;
		}

		if (pthread_cond_init(&(sp->sortCond), NULL) != 0) {
			pErr("Mutex condition init failed.\n");
			return -1;
		}
	}

	return 0;
}

char *sllGetName(int sllNum) {
	char *name = NULL;

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return name;
	}

	SLList *lp = &sortedLists[sllNum];
	if (lp->inUse == 1) {
		name = lp->llName;
	}

	return name;
}

/*
 * This function sllCreate creates a named Linked List LList
 *
 *   sllName = Linked List name to create.
 *
 *   returns -1 on error
 *           0 on success
 */
int sllCreate(const char *sllName) {
	int ret = -1;

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return ret;
	}

	pthread_mutex_lock(&createSortLock);

	// Look for empty slot or an exiting entry.
	int foundIt = -1;
	int freeSpot = -1;
	SLList *foundSpot = NULL;
	SLList *sp = sortedLists;
	for (int i = 0; i < maxSortedLists; i++, sp++) {
		if (sp->inUse == 0 && foundSpot == NULL) {
			foundSpot = sp;
			freeSpot = i;
		} else if (sp->inUse == 1 && strcmp(sp->llName, sllName) == 0) {
			// already been created.
			ret = i;
			foundIt = i;
		}
	}

	if (foundIt == -1) {
		if (freeSpot == -1) {
			pErr("No empty slots in LList array.\n");
			pthread_mutex_unlock(&createSortLock);
			return ret;
		}

		foundIt = freeSpot;

		foundSpot->inUse = 1;
        foundSpot->itemCount = 0;
		strcpy(foundSpot->llName, sllName);

		ret = foundIt;
	}

	pthread_mutex_unlock(&createSortLock);

	return ret;
}

/*
 * This function addSLdata is private to this file.
 *
 *   lst = LList entry to add data to
 *   lData = Qdata structure to add to queue.
 *
 *   returns void
 */
void addSLdata(SLList *lst, Ldata *lData) {

	if (lst->head == NULL || strcmp(lst->head->data, lData->data) > 0) {
        lData->next = lst->head;
        lst->head = lData;
		lst->tail = lData;
	} else {
        Ldata *head = lst->head;
        while (head->next != NULL) {
            printf("strcmp: %s, %s\n", head->next->data, lData->data);
            int c = strcmp(head->next->data, lData->data);
            if (c < 0) {
                printf("breaking\n");
            }
            head = head->next;
        }

        lData->next = head->next;
        head->next = lData;
	}
}

/*
 * This function sllAdd adds data to the named queue
 *
 *   sllName = LList name
 *   data = data to add to queue
 *   length = Length of data
 *
 *   return -1 on error
 *          0 on success
 */
int sllAdd(int sllNum, const char *data, int length) {
	int ret = -1;

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return ret;
	}

	SLList *sp = &sortedLists[sllNum];
	if (sp->inUse == 1) {
		pthread_mutex_lock(&(sp->sortLock));

		Ldata *lData = (Ldata *)calloc(1, sizeof(Ldata));
		lData->length = length;
		lData->data = (char *)calloc(1, length + 1);
		memcpy(lData->data, data, length);

		addSLdata(sp, lData);
		sp->itemCount++;
		pthread_cond_signal(&(sp->sortCond));
		ret = 0;

		pthread_mutex_unlock(&(sp->sortLock));
	}

	return ret;
}

/*
 * This function sllAddUnique only will add the data if is unique to the linked list.
 *
 *   sllName = LLIst name
 *   data = data to be added is it does not already exists
 *   length = lengtg of data in bytes.
 *
 *   returns -1 if it already existed
 *           0 if added data to linked list.
 */
int sllAddUnique(int sllNum, const char *data, int length) {
    int ret = -1;

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return ret;
    }

    if (llFind(sllNum, data) == 0) {
        sllAdd(sllNum, data, length);
        ret = 0;
    }

    return ret;
}


/*
 * This function sllAddString is the same as sllAdd but assumes data is null terminated.
 * See sllAdd() for details.
 */
int sllAddString(int sllNum, const char *data) {
    return sllAdd(sllNum, data, strlen(data));
}

/*
 * This function sllAddUuniqueString is the same as sllAddUnique but assumes data is null terminated.
 * See sllAddUnique() for details.
 */
int sllAddUniqueString(int sllNum, const char *data) {
    return sllAddUnique(sllNum, data, strlen(data));
}

/*
 * This function sllFind finds the string if present in the linked list.
 *
 *   sllName = LList name
 *   findStr = string to search for in linked list.
 *
 *   returns 1 if string is found
 *           else 0 if not found.
 */
int sllFind(int sllNum, const char *findStr) {
    int ret = 0;

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return 0;
    }

	SLList *sp = &sortedLists[sllNum];
	if (sp->inUse == 1) {
		pthread_mutex_lock(&(sp->sortLock));

		if (sp->itemCount <= 0) {
			pthread_mutex_unlock(&(sp->sortLock));
			return ret;
		}

		Ldata *ld = sp->head;
		for (int x = 0; x < sp->itemCount; x++, ld = ld->next) {
			if (strcmp((char *)ld->data, (char *)findStr) == 0) {
				ret = 1;
				break;
		   }
		}

		pthread_mutex_unlock(&(sp->sortLock));
	}

	return ret;
}

/*
 * This function sllRemove removes the first element in the queue and
 * returns the data to you.
 * NOTE: The data returned MUST be freed by the caller.
 *
 *   sllName = LList name
 *   length = Pointer to int to place length into.
 *   bsortLock = if true then bsortLock waiting on an item else return null if queue empty
 *
 *   returns NULL on error or empty queue
 *           else pointer to data
 */
char *sllRemove(int sllNum, int *length, int bsortLock) {
    char *data = NULL;
	Ldata *h = NULL;
	Ldata *t = NULL;

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return NULL;
	}

	SLList *sp = &sortedLists[sllNum];
	if (sp->inUse == 1) {
		pthread_mutex_lock(&(sp->sortLock));

		if (sp->itemCount <= 0 && bsortLock == 0) {
			pthread_mutex_unlock(&(sp->sortLock));
			return NULL;
		}

		while (sp->itemCount == 0) {
			pthread_cond_wait(&(sp->sortCond), &(sp->sortLock));
		}

		h = sp->head;
		t = h->next;
		data = (char *)calloc(1, h->length);
		memcpy(data, h->data, h->length);
		free(h->data);
		if (length != NULL)
			*length = h->length;
		free(h);			// this does NOT free the data part.
		sp->head = t;
		if (sp->itemCount > 0)
			sp->itemCount--;
		else
			sp->itemCount = 0;

		pthread_mutex_unlock(&(sp->sortLock));
	}

	return data;
}

/* This function sllDestroy frees up the meeory used by the given named list.
 *
 *   sllName = list name
 *
 *   return -1 on error
 *          else 0 on success
 */
int sllDestroy(int sllNum) {

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return -1;
	}

	SLList *sp = &sortedLists[sllNum];
	if (sp->inUse == 1) {
		pthread_mutex_lock(&(sp->sortLock));

		if (sp->listStr != NULL)
			free(sp->listStr);

		Ldata *ld = sp->head;
		while (ld != NULL) {
			Ldata *t = ld->next;
			free(ld->data);
			free(ld);
			ld = t;
		}

		sp->inUse = 0;

		pthread_mutex_unlock(&(sp->sortLock));
	}

	return 0;
}

/*
 * This function sllCount return the number of item in the named queue.
 *
 *   sllName = LList name
 *
 *   returns number items in queue.
 */
int sllCount(int sllNum) {
	int count = 0;

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return 0;
	}

	SLList *sp = &sortedLists[sllNum];
	if (sp->inUse == 1) {
		pthread_mutex_lock(&(sp->sortLock));

		count = sp->itemCount;

		pthread_mutex_unlock(&(sp->sortLock));
	}

	return count;
}

/*
 * This function sllList returns a comma separated string of the linked list.
 *
 *   sllName = LList name
 *
 *   returns number items in queue.
 */
char *sllList(int sllNum) {

	if (sortedLists == NULL) {
		pErr("Must call sllInit() first.\n");
		return 0;
	}

	SLList *sp = &sortedLists[sllNum];
	if (sp->inUse == 1) {
		pthread_mutex_lock(&(sp->sortLock));

		if (sp->listStr != NULL)
			free(sp->listStr);

		int totalSize = 0;
		Ldata *ld = sp->head;
		while (ld != NULL) {
			totalSize += ld->length;
			ld = ld->next;
		}

		sp->listStr = (char *)calloc(1, (totalSize + sp->itemCount + 2));
		ld = sp->head;
		while (ld != NULL) {
			if (sp->listStr[0] == '\0') {
				strcpy(sp->listStr, ld->data);
			} else {
				strcat(sp->listStr, ",");
				strcat(sp->listStr, ld->data);
			}
			ld = ld->next;
		}

		pthread_mutex_unlock(&(sp->sortLock));
	}

    return sp->listStr;
}
