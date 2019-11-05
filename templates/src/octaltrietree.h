/*
 * octaltritree.h
 *
 *  Created on: June 3, 2017
 *      Author: Kelly Wiles
 */

#ifndef TEMPLATES_SRC_OCTALTRIETREE_H_
#define TEMPLATES_SRC_OCTALTRIETREE_H_

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

typedef struct _theOctalData {
   uint32_t e1;
   uint32_t e2;
   uint32_t e3;
   uint32_t e4;
} TheOctalData;

// The *next array on a 64bit system is 64 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _octalTrieTreeNode {
	TheOctalData *data;
	uint32_t useCount;
	uint16_t inUse;
	struct _octalTrieTreeNode *next[8];
} OctalTrieTreeNode;

typedef struct _octalTrieTree {
	OctalTrieTreeNode *root;
} OctalTrieTree;

OctalTrieTree *ottInit();
OctalTrieTreeNode *ottFindEnd(OctalTrieTree *trie, unsigned int ip);
int ottInsert(OctalTrieTree *trie, unsigned int ip, TheOctalData *value);
//int ottRemove(OctalTrieTree *trie, unsigned  int ip);
void *ottLookup(OctalTrieTree *trie, unsigned int ip);
int ottNumEntries(OctalTrieTree *trie);



#endif /* TEMPLATES_SRC_OCTALTRIETREE_H_ */
