/*
 * hextrietree.c
 *
 * Non-compressed Trie Tree for hex storage and lookup.
 *
 * To be used to storage IP address in hex strings alone with it data.
 * In this case the tree height is a max of 8.
 *
 *  Created on: Apr 27, 2017
 *      Author: Kelly Wiles
 */

#include "hextrietree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logutils.h"
#include "nkxutils.h"
#include "strutils.h"

/*
 * NOTE: Use this code as a template because it is not really
 * good as a library call.
 * I will probably move it some were else in the future. KW
 *
 * To create the hex string that will be passed to this functions.
 *    char hexStr[9];
 *    snprintf(hexStr, "%08x", num);
 */

int _hexTrieTreeInit = 0;

static inline int _toHexIdx(char ch) __attribute__((always_inline));

HexTrieTree *httInit() {

	HexTrieTree *httRoot = (HexTrieTree *) calloc(1, sizeof(HexTrieTree));
	if (httRoot == NULL)
		return NULL;
	httRoot->root = NULL;

	_hexTrieTreeInit = 1;

	return httRoot;
}

/*
 * Function _toHexIdx is private to this file.
 */
static inline int _toHexIdx(char ch) {
	// Only allow hex characters, both upper and lower case.
	if (ch > 47 && ch < 58)
		return ((int)ch - (int)'0');
	else if ((int)ch > 64 && ch < 71)
		return ((int)ch - (int)'A') + 10;
	else if ((int)ch > 96 && ch < 103)
		return ((int)ch - (int)'a') + 10;
	else {
		pErr("ERROR: Not a Hex character.\n");
		return 0;
	}
}

/*
 * Function to find end of tree for a given IP address.
 *
 *   hexIP = A hex string of IPv4 address string like, "192.168.7.100" = "c0a80764"
 */
HexTrieTreeNode *httFindEnd(HexTrieTree *trie, char *hexIP) {
	HexTrieTreeNode *node;
	char *p;

	if (_hexTrieTreeInit == 0) {
		pErr("Must call httInit() first.\n");
		return NULL;
	}

	// Search down the trie until the end of string is reached

	node = trie->root;

	for (p = hexIP; *p != '\0'; ++p) {

		if (node == NULL) {
			// Not found in the tree. Return.
			return NULL;
		}

		// Jump to the next node
		node = node->next[_toHexIdx(*p)];
	}

	// This IP is present if the value at the last node is not NULL

	return node;
}

/*
 * Function _httInsertRollback is private to this file.
 */
static void _httInsertRollback(HexTrieTree *trie, char *ip) {
	HexTrieTreeNode *node;
	HexTrieTreeNode **prev_ptr;
	HexTrieTreeNode *next_node;
	HexTrieTreeNode **next_prev_ptr;
	char *p;

	// Follow the chain along.  We know that we will never reach the
	// end of the string because httInsert never got that far.  As a
	// result, it is not necessary to check for the end of string
	// delimiter (NUL)

	node = trie->root;
	prev_ptr = &trie->root;
	p = ip;

	while (node != NULL) {

		/* Find the next node now. We might free this node. */

		next_prev_ptr = &node->next[_toHexIdx(*p)];
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
 * Function httInsert is used to insert data into trie tree.
 */
int httInsert(HexTrieTree *trie, char *hexIP, TheHexData *value) {
	HexTrieTreeNode **rover;
	HexTrieTreeNode *node;
	char *p;

	if (_hexTrieTreeInit == 0) {
		pErr("Must call httInit() first.\n");
		return 0;
	}

	/* Cannot insert NULL values */

	if (value == TRIE_NULL) {
		return 0;
	}

	// Search down the trie until we reach the end of string,
	// creating nodes as necessary

	rover = &trie->root;

	p = hexIP;

	HexTrieTreeNode *tmp = NULL;

	for (;;) {

		node = *rover;

		if (tmp == NULL) {
			// tmp will be freed if it is unused at end of loop.
			tmp = (HexTrieTreeNode *) calloc(1, sizeof(HexTrieTreeNode));
			if (tmp != NULL)
				tmp->inUse = 1;
		}

		if (tmp == NULL) {
			// Allocation failed.  Go back and undo
			// what we have done so far.
			_httInsertRollback(trie, hexIP);

			return 0;
		}

		HexTrieTreeNode *expect = NULL;

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
			TheHexData *td = (TheHexData *)calloc(1, sizeof(TheHexData));

			TheHexData *expData = NULL;
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
		rover = &node->next[_toHexIdx(*p)];
		++p;
	}

	if (tmp != NULL)
		free(tmp);

	return 1;
}

//int httRemove(HexTrieTree *trie, char *ip) {
//	HexTrieTreeNode *node;
//	HexTrieTreeNode *next;
//	HexTrieTreeNode **last_next_ptr;
//	char *p;
//	int c;
//
//	// Find the end node and remove the value
//
//	node = httFindEnd(trie, ip);
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
//	p = str;
//
//	for (;;) {
//
//		// Find the next node
//
//		c = *p;
//		next = node->next[_toHexIdx(c)];
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
//			last_next_ptr = &node->next[_toHexIdx(c)];
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

void *httLookup(HexTrieTree *trie, char *ip) {
	HexTrieTreeNode *node;

	if (_hexTrieTreeInit == 0) {
		pErr("Must call httInit() first.\n");
		return NULL;
	}

	node = httFindEnd(trie, ip);

	if (node != NULL) {
		return node->data;
	} else {
		return TRIE_NULL;
	}
}

int httNumEntries(HexTrieTree *trie) {
	// To find the number of entries, simply look at the use count
	// of the root node.

	if (_hexTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return 0;
	}

	if (trie->root == NULL) {
		return 0;
	} else {
		return AtomicGet(&trie->root->useCount);
	}
}
