/*
 * bittrietree.c
 *
 * Non-compressed Trie Tree for bit storage and lookup.
 *
 * To be used to storage data using bits as lookup.
 *
 *  Created on: Apr 27, 2017
 *      Author: Kelly Wiles
 */

#include "bittrietree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "logutils.h"
#include "nkxutils.h"
#include "strutils.h"

/*
 * NOTE: Use this code as a template because it is not really
 * good as a library call.
 * I will probably move it some were else in the future. KW
 */

#define BITIDX(c)	((int)c - (int)'0')

int _bitTrieTreeInit = 0;

BitTrieTree *bttInit() {

	BitTrieTree *bttRoot = (BitTrieTree *) calloc(1, sizeof(BitTrieTree));
	if (bttRoot == NULL)
		return NULL;
	bttRoot->root = NULL;

	_bitTrieTreeInit = 1;

	return bttRoot;
}

static char *_binary(unsigned int n, char *b, int bLen) {
	if (bLen <= 0)
		bLen = (sizeof(n) * CHAR_BIT);

	for (int i = bLen; i >= 0; i--) {
		b[i] = (n & 1)? '1' : '0';

		n >>= 1;
	}

	b[bLen] = '\0';

	return b;
}

/*
 * Function to find end of tree for a given IP address.
 *
 *   bitLength = number of bit used of the unsigned int bits
 */
BitTrieTreeNode *bttFindEnd(BitTrieTree *trie, unsigned int bits, int bLen) {
	BitTrieTreeNode *node;
	char *p;

	if (_bitTrieTreeInit == 0) {
		pErr("Must call bttInit() first.\n");
		return NULL;
	}

	// Search down the trie until the end of string is reached

	node = trie->root;

	if (bLen <= 0)
		bLen = (sizeof(bits) * CHAR_BIT);

	char bStr[bLen + 1];

	_binary(bits, bStr, bLen);

	for (p = bStr; *p != '\0'; ++p) {

		if (node == NULL) {
			// Not found in the tree. Return.
			return NULL;
		}

		// Jump to the next node
		node = node->next[BITIDX(*p)];
	}

	// This IP is present if the value at the last node is not NULL

	return node;
}

/*
 * Function _bttInsertRollback is private to this file.
 */
static void _bttInsertRollback(BitTrieTree *trie, unsigned int bits, int bLen) {
	BitTrieTreeNode *node;
	BitTrieTreeNode **prev_ptr;
	BitTrieTreeNode *next_node;
	BitTrieTreeNode **next_prev_ptr;
	char *p;

	if (bLen <= 0)
		bLen = (sizeof(bits) * CHAR_BIT);

	// Follow the chain along.  We know that we will never reach the
	// end of the string because bttInsert never got that far.  As a
	// result, it is not necessary to check for the end of string
	// delimiter (NUL)

	node = trie->root;
	prev_ptr = &trie->root;

	char bStr[bLen + 1];

	_binary(bits, bStr, bLen);
	p = bStr;

	while (node != NULL) {

		/* Find the next node now. We might free this node. */

		next_prev_ptr = &node->next[BITIDX(*p)];
		next_node = *next_prev_ptr;
		++p;

		// Decrease the use count and free the node if it
		// reaches zero.

		AtomicSub(&node->useCount, 1);

		if (node->useCount == 0) {
			free(node);

			if (prev_ptr != NULL) {
				*prev_ptr = NULL;
			}

			next_prev_ptr = NULL;
		}

		/* Update pointers */

		node = next_node;
		prev_ptr = next_prev_ptr;
	}
}

/*
 * Function bttInsert is used to insert data into trie tree.
 */
int bttInsert(BitTrieTree *trie, unsigned int bits, int bLen, TheBitData *value) {
	BitTrieTreeNode **rover;
	BitTrieTreeNode *node;
	char *p;

	if (_bitTrieTreeInit == 0) {
		pErr("Must call bttInit() first.\n");
		return 0;
	}

	/* Cannot insert NULL values */

	if (value == TRIE_NULL) {
		return 0;
	}

	// Search down the trie until we reach the end of string,
	// creating nodes as necessary

	rover = &trie->root;

	if (bLen <= 0)
		bLen = (sizeof(bits) * CHAR_BIT);

	char bStr[bLen + 1];
	_binary(bits, bStr, bLen);

	p = bStr;

	BitTrieTreeNode *tmp = NULL;

	for (;;) {

		node = *rover;

		if (tmp == NULL) {
			// tmp will be freed if it is unused at end of loop.
			tmp = (BitTrieTreeNode *) calloc(1, sizeof(BitTrieTreeNode));
			if (tmp != NULL)
				tmp->inUse = 1;
		}

		if (tmp == NULL) {
			// Allocation failed.  Go back and undo
			// what we have done so far.
			_bttInsertRollback(trie, bits, bLen);

			return 0;
		}

		BitTrieTreeNode *expect = NULL;

		// Trying to avoid locks here.
		if (AtomicExchange(&node, &expect, &tmp) == 1) {
			*rover = tmp;
			tmp = NULL;		// Set tmp so another will be allocated.
		} else {
			// Another thread beat us in adding node.
			// Do not free tmp here.
		}

		// Increase the node useCount
		AtomicAdd(&node->useCount, 1);

		// Reached the end of string?  If so, we're finished.
		if (*p == '\0') {
			TheBitData *td = (TheBitData *)calloc(1, sizeof(TheBitData));

			TheBitData *expData = NULL;
			if (AtomicExchange(&node->data, &expData, &td) == 0)
				free(td);	// node->data already allocated

			// Set the data at the node we have reached
			AtomicAdd(&node->data->e1, value->e1);
			AtomicAdd(&node->data->e2, value->e2);
			AtomicAdd(&node->data->e3, value->e3);
			AtomicAdd(&node->data->e4, value->e4);

			break;
		}

		// Advance to the next node in the chain
		rover = &node->next[BITIDX(*p)];
		++p;
	}

	if (tmp != NULL)
		free(tmp);

	return 1;
}

//int bttRemove(BitTrieTree *trie, unsigned int bits, int bLen) {
//	BitTrieTreeNode *node;
//	BitTrieTreeNode *next;
//	BitTrieTreeNode **last_next_ptr;
//	char *p;
//	int c;
//
//	// Find the end node and remove the value
//
//	node = bttFindEnd(trie, bits, blen);
//
//	if (node != NULL && node->data != TRIE_NULL) {
//		node->data = TRIE_NULL;
//	} else {
//		return 0;
//	}
//
//	// Now traverse the tree again as before, decrementing the use
//	// count of each node.  Free back nodes as necessary.
//
//	node = trie->root;
//	last_next_ptr = &trie->root;

//  char str[32];
//	if (_removeDots(ip, str) == 1)
//      return 0;
//	if (bLern <= 0)
//		bLen = (sizeof(buts) * CHAR_BIT);
//
//	char bStr[bLen + 1];
//	_binary(bits, bStr, int bLen);
//
//	p = bStr;
//
//	for (;;) {
//
//		// Find the next node
//
//		c = *p;
//		next = node->next[BITIDX(c)];
//
//		// Free this node if necessary
//
//		AtomicSub(&node->useCount, 1);
//
//		if (node->useCount <= 0) {
//			free(node);
//			AtomicSet(&node->inUse, 0);
//
//			// Set the "next" pointer on the previous node to NULL,
//			// to unlink the freed node from the tree.  This only
//			// needs to be done once in a remove.  After the first
//			// unlink, all further nodes are also going to be free'd.
//
//			if (last_next_ptr != NULL) {
//				*last_next_ptr = NULL;
//				last_next_ptr = NULL;
//			}
//		}
//
//		// Go to the next character or finish
//
//		if (c == '\0') {
//			break;
//		} else {
//			++p;
//		}
//
//		// If necessary, save the location of the "next" pointer
//		// so that it may be set to NULL on the next iteration if
//		// the next node visited is freed.
//
//		if (last_next_ptr != NULL) {
//			last_next_ptr = &node->next[BITIDX(c)];
//		}
//
//		// Jump to the next node
//		node = next;
//	}
//
//	// Removed successfully
//
//	return 1;
//}

void *bttLookup(BitTrieTree *trie, unsigned int bits, int bLen) {
	BitTrieTreeNode *node;

	if (_bitTrieTreeInit == 0) {
		pErr("Must call bttInit() first.\n");
		return NULL;
	}

	node = bttFindEnd(trie, bits, bLen);

	if (node != NULL) {
		return node->data;
	} else {
		return TRIE_NULL;
	}
}

int bttNumEntries(BitTrieTree *trie) {
	// To find the number of entries, simply look at the use count
	// of the root node.

	if (_bitTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return 0;
	}

	if (trie->root == NULL) {
		return 0;
	} else {
		return AtomicGet(&trie->root->useCount);
	}
}
