/*
 * hextritree.h
 *
 *  Created on: Apr 27, 2017
 *      Author: Kelly Wiles
 */

#ifndef INCS_BITTRIETREE_H_
#define INCS_BITTRIETREE_H_

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

typedef struct _theBitData {
   uint32_t e1;
   uint32_t e2;
   uint32_t e3;
   uint32_t e4;
} TheBitData;

// The *next array on a 64bit system is 16 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _bitTrieTreeNode {
	TheBitData *data;
	uint32_t useCount;
	uint16_t inUse;
	struct _bitTrieTreeNode *next[2];
} BitTrieTreeNode;

typedef struct _bitTrieTree {
	BitTrieTreeNode *root;
} BitTrieTree;

BitTrieTree *bttInit();
BitTrieTreeNode *bttFindEnd(BitTrieTree *trie, unsigned int bits, int bLen);
int bttInsert(BitTrieTree *trie, unsigned int bits, int bLen, TheBitData *value);
//int bttRemove(BitTrieTree *trie, unsigned int bits, int bLen);
void *bttLookup(BitTrieTree *trie, unsigned int bits, int bLen);
int bttNumEntries(BitTrieTree *trie);



#endif /* INCS_HEXTRIETREE_H_ */
