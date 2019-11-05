
/*
 * Copyright (c) 2015 Richard Kelly Wiles (rkwiles@twc.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Originally written in 05/28/1988, modified to bring it up to coding and format standards. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strutils.h"

/*
 * This function split will split a string based on the character passed in.
 * This can only split things like comma separated values and the like.
 * If a string has the following pattern.
 *     The,end,,,is,near.
 * The above string will return a token count of 6 and the following in the words array.
 * The string pointed to by str is not null terminated, must use len field.
 * This function does NOT modify the string passed in, unlike tokenize(), parse() and the like.
 *   words[0].str = "The";
 *   words[0].len = 3;
 *   words[1].str = "end";
 *   words[1].len = 3;
 *   words[2].str = "";
 *   words[2].len = 0;
 *   words[3].str = "";
 *   words[3].len = 0;
 *   words[4].str = "is";
 *   words[4].len = 2;
 *   words[5].str = "near.";
 *   words[5].len = 5;
 *   words[6].str = NULL;
 *   words[6].len = 0;
 *
 *   str = string to be tokenized.  This string is NOT modified.
 *   chr = character to parse string by.
 *   words = A pointer to an array of Words. This array will contain the tokens found up to (max_args - 1).
 *           Words is a typedef of the struct _Words found in the strutils.h file.
 *   maxLen = max number of pointers in the pointer array argz.
 *
 *   returns number of tokens parsed which would be less than max_args.
 *           The return pointer array is null terminated.
 */
int split(char *str, char chr, StrParts *words, int maxLen) {
	int n = 0;		// number of tokens found.

	char *p1 = str;
	char *p2 = str;
	int len = strlen(str);
	for (int i = 0; i < len; i++, p1++) {
		if (*p1 == chr) {
			words[n].len = (p1 - p2);
			words[n].str = p2;
			n++;
			p2 = (p1 + 1);		// move p2 to start of next token.
		}

		if (n >= (maxLen -2))
			break;
	}

	if (p2 != NULL) {
		words[n].len = (p1 - p2);
		words[n].str = p2;
		n++;
	}

	words[n].str = NULL;

	return n;
}
