/*
 * binarytrietree.c
 *
 * Non-compressed Trie Tree for binary storage and lookup.
 *
 * To be used to storage data using 2-binary as lookup.
 * Every 2 bits of an unsigned int is used as nodes.
 *
 *  Created on: Apr 27, 2017
 *      Author: Kelly Wiles
 */

#include "binarytrietree.h"

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

int _binaryTrieTreeInit = 0;

BinaryTrieTree *byttInit() {

	BinaryTrieTree *bttRoot = (BinaryTrieTree *) calloc(1, sizeof(BinaryTrieTree));
	if (bttRoot == NULL)
		return NULL;
	bttRoot->root = NULL;

	_binaryTrieTreeInit = 1;

	return bttRoot;
}

/*
 * Function to find end of tree for a given IP address.
 *
 *   binaryLength = number of binary used of the unsigned int binarys
 */
BinaryTrieTreeNode *byttFindEnd(BinaryTrieTree *trie, unsigned int binarys) {
	BinaryTrieTreeNode *node;

	if (_binaryTrieTreeInit == 0) {
		pErr("Must call bttInit() first.\n");
		return NULL;
	}

	// Search down the trie until the end of string is reached

	node = trie->root;

	// hard coded to to 4 byte unsigned integers.
	for (int i = 0; i < 16; i++) {
		if (node == NULL) {
			// Not found in the tree. Return.
			return NULL;
		}

		int idx = ((binarys >> (i * 2)) & 0x00000007);

		// Jump to the next node
		node = node->next[idx];
	}

	// This IP is present if the value at the last node is not NULL

	return node;
}

/*
 * Function _bttInsertRollback is private to this file.
 */
static void _byttInsertRollback(BinaryTrieTree *trie, unsigned int binarys) {
	BinaryTrieTreeNode *node;
	BinaryTrieTreeNode **prev_ptr;
	BinaryTrieTreeNode *next_node;
	BinaryTrieTreeNode **next_prev_ptr;

	// Follow the chain along.  We know that we will never reach the
	// end of the string because bttInsert never got that far.  As a
	// result, it is not necessary to check for the end of string
	// delimiter (NUL)

	node = trie->root;
	prev_ptr = &trie->root;

	for (int i = 0; i < 16; i++) {
		if (node == NULL)
			break;

		int idx = ((binarys >> (2 * i)) & 0x00000003);
		/* Find the next node now. We might free this node. */

		next_prev_ptr = &node->next[idx];
		next_node = *next_prev_ptr;

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
int byttInsert(BinaryTrieTree *trie, unsigned int binarys, TheBinaryData *value) {
	BinaryTrieTreeNode **rover;
	BinaryTrieTreeNode *node;

	if (_binaryTrieTreeInit == 0) {
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
	BinaryTrieTreeNode *tmp = NULL;

	for (int i = 0; i < 16; i++) {

		node = *rover;

		if (tmp == NULL) {
			// tmp will be freed if it is unused at end of loop.
			tmp = (BinaryTrieTreeNode *) calloc(1, sizeof(BinaryTrieTreeNode));
			if (tmp != NULL)
				tmp->inUse = 1;
		}

		if (tmp == NULL) {
			// Allocation failed.  Go back and undo
			// what we have done so far.
			_byttInsertRollback(trie, binarys);

			return 0;
		}

		BinaryTrieTreeNode *expect = NULL;

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
		if (i == 7) {
			TheBinaryData *td = (TheBinaryData *)calloc(1, sizeof(TheBinaryData));

			TheBinaryData *expData = NULL;
			if (AtomicExchange(&node->data, &expData, &td) == 0)
				free(td);	// node->data already allocated

			// Set the data at the node we have reached
			AtomicAdd(&node->data->e1, value->e1);
			AtomicAdd(&node->data->e2, value->e2);
			AtomicAdd(&node->data->e3, value->e3);
			AtomicAdd(&node->data->e4, value->e4);

			break;
		}

		int idx = ((binarys >> (2 * i)) & 0x00000003);

		// Advance to the next node in the chain
		rover = &node->next[idx];
	}

	if (tmp != NULL)
		free(tmp);

	return 1;
}

void *byttLookup(BinaryTrieTree *trie, unsigned int binarys) {
	BinaryTrieTreeNode *node;

	if (_binaryTrieTreeInit == 0) {
		pErr("Must call bttInit() first.\n");
		return NULL;
	}

	node = byttFindEnd(trie, binarys);

	if (node != NULL) {
		return node->data;
	} else {
		return TRIE_NULL;
	}
}

int byttNumEntries(BinaryTrieTree *trie) {
	// To find the number of entries, simply look at the use count
	// of the root node.

	if (_binaryTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return 0;
	}

	if (trie->root == NULL) {
		return 0;
	} else {
		return AtomicGet(&trie->root->useCount);
	}
}
