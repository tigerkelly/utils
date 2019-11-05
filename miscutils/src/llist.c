
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "miscutils.h"

int maxLists;
LList *lists = NULL;

pthread_mutex_t llCreateLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * This function initializes the Linked List array.
 *
 *   listCnt = max number of named llist you can have.
 *
 *   return -1 on error
 *          0 on success
 */
int llInit(int listCnt) {
    if (listCnt == 1)
        maxLists = 2;
    else if (listCnt <= 0)
		maxLists = MAX_LLISTS;
	else
		maxLists = listCnt;

	lists = (LList *)calloc(maxLists, sizeof (LList));
	if (lists == NULL) {
		pErr("Can not allocate Linked List memory.\n");
		return -1;
	}

	LList *lp = lists;
	for (int i = 0; i < maxLists; i++, lp++) {
		// lp->lcond = PTHREAD_COND_INITIALIZER;
		// lp->llock = PTHREAD_MUTEX_INITIALIZER;

		if (pthread_mutex_init(&(lp->llock), NULL) != 0) {
			pErr("Mutex init lock failed.\n");
			return -1;
		}

		if (pthread_cond_init(&(lp->lcond), NULL) != 0) {
			pErr("Mutex condition init failed.\n");
			return -1;
		}
	}

	return 0;
}

char *llGetName(int llNum) {
	char *name = NULL;

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return name;
	}

	LList *lp = &lists[llNum];
	if (lp->inUse == 1) {
		name = lp->llName;
	}

	return name;
}

/*
 * This function llCreate creates a named Linked List LList
 *
 *   llName = Linked List name to create.
 *
 *   returns -1 on error
 *           0 on success
 */
int llCreate(const char *llName) {
	int ret = -1;

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return ret;
	}

	pthread_mutex_lock(&llCreateLock);

	// Look for empty slot or an exiting entry.
	int foundIt = -1;
	int freeSpot = -1;
	LList *foundSpot = NULL;
	LList *lp = lists;
	for (int i = 0; i < maxLists; i++, lp++) {
		if (lp->inUse == 0 && foundSpot == NULL) {
			foundSpot = lp;
			freeSpot = i;
		} else if (lp->inUse == 1 && strcmp(lp->llName, llName) == 0) {
			// already been created.
			ret = i;
			foundIt = i;
		}
	}

	if (foundIt == -1) {
		if (freeSpot == -1) {
			pErr("No empty slots in LList array.\n");
			pthread_mutex_unlock(&llCreateLock);
			return ret;
		}

		foundIt = freeSpot;

		foundSpot->inUse = 1;
        foundSpot->itemCount = 0;
		strcpy(foundSpot->llName, llName);

		ret = foundIt;
	}

	pthread_mutex_unlock(&llCreateLock);

	return ret;
}

/*
 * This function addQdata is private to this file.
 *
 *   que = LList entry to add data to
 *   qData = Qdata structure to add to queue.
 *
 *   returns void
 */
void addLdata(LList *lst, Ldata *lData) {

	if (lst->head == NULL) {
		lst->head = lst->tail = lData;
	} else {
		lst->tail->next = lData;
		lst->tail = lData;
	}
}

/*
 * This function llAdd adds data to the named queue
 *
 *   llName = LList name
 *   data = data to add to queue
 *   length = Length of data
 *
 *   return -1 on error
 *          0 on success
 */
int llAdd(int llNum, const char *data, int length) {
	int ret = -1;

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return ret;
	}

	LList *lp = &lists[llNum];
	if (lp->inUse == 1) {
		pthread_mutex_lock(&(lp->llock));
		Ldata *lData = (Ldata *)calloc(1, sizeof(Ldata));
		lData->length = length;
		lData->data = (char *)calloc(1, length + 1);
		memcpy(lData->data, data, length);

		addLdata(lp, lData);
		lp->itemCount++;
		pthread_cond_signal(&(lp->lcond));
		ret = 0;

		pthread_mutex_unlock(&(lp->llock));
	}

	return ret;
}

/*
 * This function llAddUnique only will add the data if is unique to the linked list.
 *
 *   llName = LLIst name
 *   data = data to be added is it does not already exists
 *   length = lengtg of data in bytes.
 *
 *   returns -1 if it already existed
 *           0 if added data to linked list.
 */
int llAddUnique(int llNum, const char *data, int length) {
    int ret = -1;

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return ret;
    }

    if (llFind(llNum, data) == 0) {
        llAdd(llNum, data, length);
        ret = 0;
    }

    return ret;
}


/*
 * This function llAddString is the same as llAdd but assumes data is null terminated.
 * See llAdd() for details.
 */
int llAddString(int llNum, const char *data) {
    return llAdd(llNum, data, strlen(data));
}

/*
 * This function llAddUuniqueString is the same as llAddUnique but assumes data is null terminated.
 * See llAddUnique() for details.
 */
int llAddUniqueString(int llNum, const char *data) {
    return llAddUnique(llNum, data, strlen(data));
}

/*
 * This function llFind finds the string if present in the linked list.
 *
 *   llName = LList name
 *   findStr = string to search for in linked list.
 *
 *   returns 1 if string is found
 *           else 0 if not found.
 */
int llFind(int llNum, const char *findStr) {
    int ret = 0;

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return 0;
    }

	LList *lp = &lists[llNum];
	if (lp->inUse == 1) {
		pthread_mutex_lock(&(lp->llock));

		if (lp->itemCount <= 0) {
			pthread_mutex_unlock(&(lp->llock));
			return ret;
		}

		Ldata *ld = lp->head;
		for (int x = 0; x < lp->itemCount; x++, ld = ld->next) {
			if (strcmp((char *)ld->data, (char *)findStr) == 0) {
				ret = 1;
				break;
		   }
		}

		pthread_mutex_unlock(&(lp->llock));
	}

	return ret;
}

/*
 * This function llRemove removes the first element in the queue and
 * returns the data to you.
 * NOTE: The data returned MUST be freed by the caller.
 *
 *   llName = LList name
 *   length = Pointer to int to place length into.
 *   block = if true then block waiting on an item else return null if queue empty
 *
 *   returns NULL on error or empty queue
 *           else pointer to data
 */
char *llRemove(int llNum, int *length, int block) {
    char *data = NULL;
	Ldata *h = NULL;
	Ldata *t = NULL;

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return NULL;
	}

	LList *lp = &lists[llNum];
	if (lp->inUse == 1) {
		pthread_mutex_lock(&(lp->llock));

		if (lp->itemCount <= 0 && block == 0) {
			pthread_mutex_unlock(&(lp->llock));
			return data;
		}

		while (lp->itemCount == 0) {
			pthread_cond_wait(&(lp->lcond), &(lp->llock));
		}

		h = lp->head;
		t = h->next;
		data = (char *)calloc(1, h->length);
		memcpy(data, h->data, h->length);
		free(h->data);
		if (length != NULL)
			*length = h->length;
		free(h);			// this does NOT free the data part.
		lp->head = t;
		if (lp->itemCount > 0)
			lp->itemCount--;
		else
			lp->itemCount = 0;

		pthread_mutex_unlock(&(lp->llock));
	}

	return data;
}

/* This function sllDestroy frees up the memory used by the given named list.
 *
 *  sllName = list name
 *
 *  return -1 on error
 *         else 0 on success
 */
int llDestroy(int llNum) {

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return -1;
	}

	LList *lp = &lists[llNum];
	if (lp->inUse == 1) {
		pthread_mutex_lock(&(lp->llock));

		if (lp->listStr != NULL)
			free(lp->listStr);

		Ldata *ld = lp->head;
		while (ld != NULL) {
			Ldata *t = ld->next;
			free(ld->data);
			free(ld);
			ld = t;
		}

		lp->inUse = 0;

		pthread_mutex_unlock(&(lp->llock));
	}

	return 0;
}

/*
 * This function llCount returns the number of item in the named queue.
 *
 *   llName = LList name
 *
 *   returns number items in queue.
 */
int llCount(int llNum) {
	int count = 0;

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return 0;
	}

	LList *lp = &lists[llNum];
	if (lp->inUse == 1) {
		pthread_mutex_lock(&(lp->llock));

		count = lp->itemCount;

		pthread_mutex_unlock(&(lp->llock));
	}

	return count;
}

/*
 * This function llList returns a comma separated string of the linked list.
 *
 *  llName = LList name
 *
 *  returns number items in queue.
 */
char *llList(int llNum) {

	if (lists == NULL) {
		pErr("Must call llInit() first.\n");
		return 0;
	}

	LList *lp = &lists[llNum];
	if (lp->inUse == 1) {
		pthread_mutex_lock(&(lp->llock));

		if (lp->listStr != NULL)
			free(lp->listStr);

		int totalSize = 0;
		Ldata *ld = lp->head;
		while (ld != NULL) {
			totalSize += ld->length;
			ld = ld->next;
		}

		lp->listStr = (char *)calloc(1, (totalSize + lp->itemCount + 2));
		ld = lp->head;
		while (ld != NULL) {
			if (lp->listStr[0] == '\0') {
				strcpy(lp->listStr, ld->data);
			} else {
				strcat(lp->listStr, ",");
				strcat(lp->listStr, ld->data);
			}
			ld = ld->next;
		}

		pthread_mutex_unlock(&(lp->llock));
	}

    return lp->listStr;
}
