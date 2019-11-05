/*
 * octaltrietree.c
 *
 * Non-compressed Trie Tree for octal storage and lookup.
 *
 * To be used to storage IP address in unsigned integer alone with it data.
 * In this case the tree height is a max of 8.
 *
 *  Created on: June 3, 2017
 *      Author: Kelly Wiles
 */

#include "octaltrietree.h"

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

int _octalTrieTreeInit = 0;

OctalTrieTree *ottInit() {

	OctalTrieTree *ottRoot = (OctalTrieTree *) calloc(1, sizeof(OctalTrieTree));
	if (ottRoot == NULL)
		return NULL;
	ottRoot->root = NULL;

	_octalTrieTreeInit = 1;

	return ottRoot;
}

/*
 * Function to find end of tree for a given IP address.
 *
 *   octalIP = A octal string of IPv4 address string like, "192.168.7.100" = "c0a80764"
 */
OctalTrieTreeNode *ottFindEnd(OctalTrieTree *trie, unsigned int octalIP) {
	OctalTrieTreeNode *node;

	if (_octalTrieTreeInit == 0) {
		pErr("Must call ottInit() first.\n");
		return NULL;
	}

	// Search down the trie until the end of string is reached

	node = trie->root;

	int len = sizeof(unsigned int) * 2;

	for (int i = 0; i < len; i++) {
		if (node == NULL) {
			// Not found in the tree. Return.
			return NULL;
		}

		unsigned int octal = (octalIP >> (i * 4) & 0x0000000f);

		// Jump to the next node
		node = node->next[octal];
	}

	// This IP is present if the value at the last node is not NULL

	return node;
}

/*
 * Function _ottInsertRollback is private to this file.
 */
static void _ottInsertRollback(OctalTrieTree *trie, unsigned int ip) {
	OctalTrieTreeNode *node;
	OctalTrieTreeNode **prev_ptr;
	OctalTrieTreeNode *next_node;
	OctalTrieTreeNode **next_prev_ptr;
	int n = 0;

	// Follow the chain along.  We know that we will never reach the
	// end of the string because ottInsert never got that far.  As a
	// result, it is not necessary to check for the end of string
	// delimiter (NUL)

	node = trie->root;
	prev_ptr = &trie->root;

	int len = sizeof(unsigned int) * 2;

	while (node != NULL && n < len) {

		/* Find the next node now. We might free this node. */

		unsigned int octal = (ip >> (n * 4) & 0x0000000f);

		next_prev_ptr = &node->next[octal];
		next_node = *next_prev_ptr;
		n++;

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
 * Function ottInsert is used to insert data into trie tree.
 */
int ottInsert(OctalTrieTree *trie, unsigned int octalIP, TheOctalData *value) {
	OctalTrieTreeNode **rover;
	OctalTrieTreeNode *node;

	if (_octalTrieTreeInit == 0) {
		pErr("Must call ottInit() first.\n");
		return 0;
	}

	/* Cannot insert NULL values */

	if (value == TRIE_NULL) {
		return 0;
	}

	// Search down the trie until we reach the end of unsigned int,
	// creating nodes as necessary

	rover = &trie->root;

	OctalTrieTreeNode *tmp = NULL;

	int len = sizeof(unsigned int) * 2;

	for (int i = 0; i < len; i++) {

		node = *rover;

		if (tmp == NULL) {
			// tmp will be freed if it is unused at end of loop.
			tmp = (OctalTrieTreeNode *) calloc(1, sizeof(OctalTrieTreeNode));
			if (tmp != NULL)
				tmp->inUse = 1;
		}

		if (tmp == NULL) {
			// Allocation failed.  Go back and undo
			// what we have done so far.
			_ottInsertRollback(trie, octalIP);

			return 0;
		}

		OctalTrieTreeNode *expect = NULL;

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
			TheOctalData *td = (TheOctalData *)calloc(1, sizeof(TheOctalData));

			TheOctalData *expData = NULL;
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
		unsigned int octal = (octalIP >> (i * 4) & 0x0000000f);
		rover = &node->next[octal];
	}

	if (tmp != NULL)
		free(tmp);

	return 1;
}

//int ottRemove(OctalTrieTree *trie, char *ip) {
//	OctalTrieTreeNode *node;
//	OctalTrieTreeNode *next;
//	OctalTrieTreeNode **last_next_ptr;
//	char *p;
//	int c;
//
//	// Find the end node and remove the value
//
//	node = ottFindEnd(trie, ip);
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
//		next = node->next[_toOctalIdx(c)];
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
//			last_next_ptr = &node->next[_toOctalIdx(c)];
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

void *ottLookup(OctalTrieTree *trie, unsigned int ip) {
	OctalTrieTreeNode *node;

	if (_octalTrieTreeInit == 0) {
		pErr("Must call ottInit() first.\n");
		return NULL;
	}

	node = ottFindEnd(trie, ip);

	if (node != NULL) {
		return node->data;
	} else {
		return TRIE_NULL;
	}
}

int ottNumEntries(OctalTrieTree *trie) {
	// To find the number of entries, simply look at the use count
	// of the root node.

	if (_octalTrieTreeInit == 0) {
		pErr("Must call ttInit() first.\n");
		return 0;
	}

	if (trie->root == NULL) {
		return 0;
	} else {
		return AtomicGet(&trie->root->useCount);
	}
}
