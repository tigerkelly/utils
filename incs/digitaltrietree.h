/*
 * digitaltritree.h
 *
 *  Created on: Apr 27, 2017
 *      Author: nkx
 */

#ifndef INCS_DIGITALTRIETREE_H_
#define INCS_DIGITALTRIETREE_H_

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

typedef struct _theData {
   uint32_t e1;
   uint32_t e2;
   uint32_t e3;
   uint32_t e4;
} TheData;

// The *next array on a 64bit system is 80 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _digitalTrieTreeNode {
	TheData *data;
	uint32_t useCount;
	uint16_t inUse;
	struct _digitalTrieTreeNode *next[10];
} DigitalTrieTreeNode;

typedef struct _digitalTrieTree {
	DigitalTrieTreeNode *root;
} DigitalTrieTree;

DigitalTrieTree *dttInit();
DigitalTrieTreeNode *dttFindEnd(DigitalTrieTree *trie, char *ip);
int dttInsert(DigitalTrieTree *trie, char *ip, TheData *value);
//int dttRemove(DigitalTrieTree *trie, char *ip);
void *dttLookup(DigitalTrieTree *trie, char *ip);
int dttNumEntries(DigitalTrieTree *trie);



#endif /* INCS_DIGITALTRIETREE_H_ */
