/*
 * hextritree.h
 *
 *  Created on: Apr 27, 2017
 *      Author: nkx
 */

#ifndef INCS_HEXTRIETREE_H_
#define INCS_HEXTRIETREE_H_

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

typedef struct _theHexData {
   uint32_t e1;
   uint32_t e2;
   uint32_t e3;
   uint32_t e4;
} TheHexData;

// The *next array on a 64bit system is 128 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _hexTrieTreeNode {
	TheHexData *data;
	uint32_t useCount;
	uint16_t inUse;
	struct _hexTrieTreeNode *next[16];
} HexTrieTreeNode;

typedef struct _hexTrieTree {
	HexTrieTreeNode *root;
} HexTrieTree;

HexTrieTree *httInit();
HexTrieTreeNode *httFindEnd(HexTrieTree *trie, char *ip);
int httInsert(HexTrieTree *trie, char *ip, TheHexData *value);
//int httRemove(HexTrieTree *trie, char *ip);
void *httLookup(HexTrieTree *trie, char *ip);
int httNumEntries(HexTrieTree *trie);



#endif /* INCS_HEXTRIETREE_H_ */
