/*
 * digitaltrietree.c
 *
 * Non-compressed Trie Tree for digital storage and lookup.
 *
 * To be used to storage IP address strings alone with it data.
 * In this case the tree height is a max of 12.
 *
 *  Created on: Apr 11, 2017
 *      Author: Kelly Wiles
 */

#include "digitaltrietree.h"

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
 */

#define IDX(c)	((int)c - (int)'0')

int _digitalTrieTreeInit = 0;

DigitalTrieTree *dttInit() {

	DigitalTrieTree *dttRoot = (DigitalTrieTree *) calloc(1, sizeof(DigitalTrieTree));
	if (dttRoot == NULL)
		return NULL;
	dttRoot->root = NULL;

	_digitalTrieTreeInit = 1;

	return dttRoot;
}

static int _removeDots(char *ip, char *str) {

	char *args[5];
	char tmp[32];
	strcpy(tmp, ip);

	tokenize(tmp, '.', args, 5);

	for (int i = 0; i < 4; i++) {
		int len = strlen(args[i]);
		for(int x = 0; x < len; x++) {
			if (isdigit(args[i][x]) == 0) {
				str[0] = '\0';
				return 1;		// invalid digit found.
			}
		}
		if (len < 3) {
			int n = atoi(args[i]);
			sprintf(&str[i*3], "%03d", n);
		} else {
			strcpy(&str[i*3], args[i]);
		}
	}

	return 0;
}

/*
 * Function to find end of tree for a given IP address.
 *
 *   ip = A dotted IP address string like, "192.168.0.1"
 */
DigitalTrieTreeNode *dttFindEnd(DigitalTrieTree *trie, char *ip) {
	DigitalTrieTreeNode *node;
	char *p;

	if (_digitalTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return NULL;
	}

	// Search down the trie until the end of string is reached

	node = trie->root;

	char str[32];
	if (_removeDots(ip, str) == 1)
		return NULL;

	for (p = str; *p != '\0'; ++p) {

		if (node == NULL) {
			// Not found in the tree. Return.
			return NULL;
		}

		// Jump to the next node
		node = node->next[IDX(*p)];
	}

	// This IP is present if the value at the last node is not NULL

	return node;
}

/*
 * Function _dttInsertRollback is private to this file.
 */
static void _dttInsertRollback(DigitalTrieTree *trie, char *ip) {
	DigitalTrieTreeNode *node;
	DigitalTrieTreeNode **prev_ptr;
	DigitalTrieTreeNode *next_node;
	DigitalTrieTreeNode **next_prev_ptr;
	char *p;

	// Follow the chain along.  We know that we will never reach the
	// end of the string because dttInsert never got that far.  As a
	// result, it is not necessary to check for the end of string
	// delimiter (NUL)

	node = trie->root;
	prev_ptr = &trie->root;
	p = ip;

	while (node != NULL) {

		/* Find the next node now. We might free this node. */

		next_prev_ptr = &node->next[IDX(*p)];
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
 * Function dttInsert is used to insert data into trie tree.
 */
int dttInsert(DigitalTrieTree *trie, char *ip, TheData *value) {
	DigitalTrieTreeNode **rover;
	DigitalTrieTreeNode *node;
	char *p;

	if (_digitalTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return 0;
	}

	/* Cannot insert NULL values */

	if (value == TRIE_NULL) {
		return 0;
	}

	// Search to see if this is already in the tree

//	node = dttFindEnd(trie, ip);

	// Already in the tree? If so, replace the existing value and
	// return success.

//	if (node != NULL && node->data != NULL) {
//		// need to selectively update data.
////		printf("already in tree.\n");
//		AtomicAdd(&node->data->e1, value->e1);
//		AtomicAdd(&node->data->e2, value->e2);
//		AtomicAdd(&node->data->e3, value->e3);
//		AtomicAdd(&node->data->e4, value->e4);
//		return 1;
//	}

	// Search down the trie until we reach the end of string,
	// creating nodes as necessary

	rover = &trie->root;

	char str[32];
	if (_removeDots(ip, str) == 1)
		return 0;
	p = str;

	DigitalTrieTreeNode *tmp = NULL;

	for (;;) {

		node = *rover;

		if (tmp == NULL) {
			// tmp will be freed if it is unused at end of loop.
			tmp = (DigitalTrieTreeNode *) calloc(1, sizeof(DigitalTrieTreeNode));
			if (tmp != NULL)
				tmp->inUse = 1;
		}

		if (tmp == NULL) {
			// Allocation failed.  Go back and undo
			// what we have done so far.
			_dttInsertRollback(trie, str);

			return 0;
		}

		DigitalTrieTreeNode *expect = NULL;

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
			TheData *td = (TheData *)calloc(1, sizeof(TheData));

			TheData *expData = NULL;
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
		rover = &node->next[IDX(*p)];
		++p;
	}

	if (tmp != NULL)
		free(tmp);

	return 1;
}

//int dttRemove(DigitalTrieTree *trie, char *ip) {
//	DigitalTrieTreeNode *node;
//	DigitalTrieTreeNode *next;
//	DigitalTrieTreeNode **last_next_ptr;
//	char *p;
//	int c;
//
//	// Find the end node and remove the value
//
//	node = dttFindEnd(trie, ip);
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
//		next = node->next[IDX(c)];
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
//			last_next_ptr = &node->next[IDX(c)];
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

void *dttLookup(DigitalTrieTree *trie, char *ip) {
	DigitalTrieTreeNode *node;

	if (_digitalTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return NULL;
	}

	node = dttFindEnd(trie, ip);

	if (node != NULL) {
		return node->data;
	} else {
		return TRIE_NULL;
	}
}

int dttNumEntries(DigitalTrieTree *trie) {
	// To find the number of entries, simply look at the use count
	// of the root node.

	if (_digitalTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return 0;
	}

	if (trie->root == NULL) {
		return 0;
	} else {
		return AtomicGet(&trie->root->useCount);
	}
}
