/*
 * asciitritree.h
 *
 *  Created on: Apr 27, 2017
 *      Author: nkx
 */

#ifndef INCS_ASCIITRIETREE_H_
#define INCS_ASCIITRIETREE_H_

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

typedef struct _theAsciiData {
   uint32_t e1;
   uint32_t e2;
   uint32_t e3;
   uint32_t e4;
} TheAsciiData;

// The *next array on a 64bit system is 760 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _asciiTrieTreeNode {
	TheAsciiData *data;
	uint32_t useCount;
	uint16_t inUse;
	struct _asciiTrieTreeNode *next[95];
} AsciiTrieTreeNode;

typedef struct _asciiTrieTree {
	AsciiTrieTreeNode *root;
} AsciiTrieTree;

AsciiTrieTree *attInit();
AsciiTrieTreeNode *attFindEnd(AsciiTrieTree *trie, char *ip);
int attInsert(AsciiTrieTree *trie, char *ip, TheAsciiData *value);
//int attRemove(AsciiTrieTree *trie, char *ip);
void *attLookup(AsciiTrieTree *trie, char *ip);
int attNumEntries(AsciiTrieTree *trie);



#endif /* INCS_HEXTRIETREE_H_ */
