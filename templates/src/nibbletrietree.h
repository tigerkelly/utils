/*
 * nibbletritree.h
 *
 *  Created on: June 3, 2017
 *      Author: Kelly Wiles
 */

#ifndef TEMPLATES_SRC_NIBBLETRIETREE_H_
#define TEMPLATES_SRC_NIBBLETRIETREE_H_

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

typedef struct _theNibbleData {
   uint32_t e1;
   uint32_t e2;
   uint32_t e3;
   uint32_t e4;
} TheNibbleData;

// The *next array on a 64bit system is 128 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _nibbleTrieTreeNode {
	TheNibbleData *data;
	uint32_t useCount;
	uint16_t inUse;
	struct _nibbleTrieTreeNode *next[16];
} NibbleTrieTreeNode;

typedef struct _nibbleTrieTree {
	NibbleTrieTreeNode *root;
} NibbleTrieTree;

NibbleTrieTree *nttInit();
NibbleTrieTreeNode *nttFindEnd(NibbleTrieTree *trie, unsigned int ip);
int nttInsert(NibbleTrieTree *trie, unsigned int ip, TheNibbleData *value);
//int nttRemove(NibbleTrieTree *trie, unsigned int ip);
void *nttLookup(NibbleTrieTree *trie, unsigned int ip);
int nttNumEntries(NibbleTrieTree *trie);



#endif /* TEMPLATES_SRC_NIBBLETRIETREE_H_ */
