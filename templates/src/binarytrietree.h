/*
 * binarytritree.h
 *
 *  Created on: June 3, 2017
 *      Author: Kelly Wiles
 */

#ifndef TEMPLATES_SRC_BINARYTRIETREE_H_
#define TEMPLATES_SRC_BINARYTRIETREE_H_

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

typedef struct _theBinaryData {
   uint32_t e1;
   uint32_t e2;
   uint32_t e3;
   uint32_t e4;
} TheBinaryData;

// The *next array on a 64 bit system is 32 bytes in size,
// cause on 64binary systems pointers are 8 bytes long.
typedef struct _binaryTrieTreeNode {
	TheBinaryData *data;
	uint32_t useCount;
	uint16_t inUse;
	struct _binaryTrieTreeNode *next[4];
} BinaryTrieTreeNode;

typedef struct _binaryTrieTree {
	BinaryTrieTreeNode *root;
} BinaryTrieTree;

BinaryTrieTree *byttInit();
BinaryTrieTreeNode *byttFindEnd(BinaryTrieTree *trie, unsigned int binary);
int byttInsert(BinaryTrieTree *trie, unsigned int binarys, TheBinaryData *value);
//int byttRemove(BinaryTrieTree *trie, unsigned int binary);
void *byttLookup(BinaryTrieTree *trie, unsigned int binary);
int byttNumEntries(BinaryTrieTree *trie);



#endif /* TEMPLATES_SRC_BINARYTRIETREE_H_ */
