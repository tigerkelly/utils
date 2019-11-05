
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

#include <string.h>

/*
 * This function tokenize will tokenizes a string based on the character passed in.
 * This can only tokenize things like comma separated values and the like.
 * If a string has the following pattern.
 *     The,end,,,is,near.
 * The above string will return a token count of 6 and the following in the argz array.
 *   args[0] = "The";
 *   args[1] = "end";
 *   args[2] = "";
 *   args[3] = "";
 *   args[4] = "is";
 *   args[5] = "near.";
 *   args[6] = NULL;
 *
 *   str = string to be tokenized.  This string is modified.
 *   chr = character to parse string by.
 *   argz = A pointer to an array of char * pointers. This array will contain the tokens found up to (max_args - 1).
 *   maxLen = max number of pointers in the pointer array argz.
 *
 *   returns number of tokens parsed which would be less than max_args.
 *           The return pointer array is null terminated.
 */
int tokenize(char *str, char chr, char **argz, int maxLen) {
	int n = 0;		// number of tokens found.

	char *p1 = str;
	char *p2 = str;
	int len = strlen(str);
	for (int i = 0; i < len; i++, p1++) {
		if (*p1 == chr) {
			*p1 = '\0';			// replace chr with nul.
			argz[n++] = p2;		// add start of token to array list.
			p2 = (p1 + 1);		// move p2 to start of next token.
		}

		if (n >= (maxLen -1))
			break;
	}

    if (n < (maxLen - 1))
	    argz[n++] = p2;				// pickup last token.

	argz[n] = (char *)0;		// set next pointer to null

	return n;
}
