/*
 * idnode.c
 *
 * Description: used to lookup table indexes for RTD Engine.
 *  Created on: Oct. 2, 2017
 *      Author: Kelly Wiles
 */

#include "rtdutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logutils.h"
#include "miscutils.h"
#include "strutils.h"


int _idTrieTreeInit = 0;

static inline int _char2Idx(char ch) __attribute__((always_inline));

IdTrieTree *ittInit() {

	IdTrieTree *ittRoot = (IdTrieTree *) calloc(1, sizeof(IdTrieTree));
	if (ittRoot == NULL)
		return NULL;
	ittRoot->root = NULL;

	_idTrieTreeInit = 1;

	return ittRoot;
}

static inline int _char2Idx(char ch) {

	// Only allow lower case letters and numbers,
	// this reduces the node size in bytes.
	if (ch > 47 && ch < 58)
		return (int)ch - '0';
	else if (ch > 64 && ch < 91)
		return (int)(ch - 'A') + 10;
	else if (ch > 96 && ch < 123)
		return (int)(ch - 'a') + 10;
	else {
		pErr("ERROR: Not an allowable character (%c).\n", ch);
		return 0;
	}
}

/*
 * Function to find end of tree for a given IP address.
 *
 *   ascii = A ascii string
 */
IdNode *ittFindEnd(IdTrieTree *trie, char *ascii) {
	IdNode *node;
	char *p;

	if (_idTrieTreeInit == 0) {
		pErr("Must call ittInit() first.\n");
		return NULL;
	}

	// Search down the trie until the end of string is reached

	node = trie->root;

	for (p = ascii; *p != '\0'; ++p) {

		if (node == NULL) {
			// Not found in the tree. Return.
			return NULL;
		}

		// Jump to the next node
		node = node->next[_char2Idx(*p)];
	}

	// This IP is present if the value at the last node is not NULL

	return node;
}

/*
 * Function _ittInsertRollback is private to this file.
 */
static void _ittInsertRollback(IdTrieTree *trie, char *ip) {
	IdNode *node;
	IdNode **prev_ptr;
	IdNode *next_node;
	IdNode **next_prev_ptr;
	char *p;

	// Follow the chain along.  We know that we will never reach the
	// end of the string because attInsert never got that far.  As a
	// result, it is not necessary to check for the end of string
	// delimiter (NUL)

	node = trie->root;
	prev_ptr = &trie->root;
	p = ip;

	while (node != NULL) {

		/* Find the next node now. We might free this node. */

		next_prev_ptr = &node->next[_char2Idx(*p)];
		next_node = *next_prev_ptr;
		++p;

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
 * Function ittInsert is used to insert data into trie tree.
 */
int ittInsert(IdTrieTree *trie, char *ascii, int value) {
	IdNode **rover;
	IdNode *node;
	char *p;

	if (_idTrieTreeInit == 0) {
		pErr("Must call attInit() first.\n");
		return 0;
	}

	// Search down the trie until we reach the end of string,
	// creating nodes as necessary

	rover = &trie->root;

	p = ascii;

	IdNode *tmp = NULL;

	for (;;) {

		node = *rover;

		if (tmp == NULL) {
			// tmp will be freed if it is unused at end of loop.
			tmp = (IdNode *) calloc(1, sizeof(IdNode));
			if (tmp != NULL) {
				tmp->inUse = 1;
			}
		}

		if (tmp == NULL) {
			// Allocation failed.  Go back and undo
			// what we have done so far.
			_ittInsertRollback(trie, ascii);

			return 0;
		}

		IdNode *expect = NULL;

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
			// Set the data at the node we have reached
			AtomicAdd(&node->idx, value);
			node->isEnd = 1;

			break;
		} else {
			if (node->isEnd == 0)
				node->idx = -1;
		}

		// Advance to the next node in the chain
		rover = &node->next[_char2Idx(*p)];
		++p;
	}

	if (tmp != NULL)
		free(tmp);

	return 1;
}

int ittLookup(IdTrieTree *trie, char *ip) {
	IdNode *node;

	if (_idTrieTreeInit == 0) {
		pErr("Must call ittInit() first.\n");
		return -1;
	}

	node = ittFindEnd(trie, ip);

	if (node != NULL) {
		return node->idx;
	} else {
		return -1;
	}
}

int ittNumEntries(IdTrieTree *trie) {
	// To find the number of entries, simply look at the use count
	// of the root node.

	if (_idTrieTreeInit == 0) {
		pErr("Must call ittInit() first.\n");
		return 0;
	}

	if (trie->root == NULL) {
		return 0;
	} else {
		return AtomicGet(&trie->root->useCount);
	}
}

