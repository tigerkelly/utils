
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

#include "strutils.h"

/*
 * This function qparse tokenizes a string based on the characters passed in.
 * If a string has the following pattern.
 *     The,end,,,is,"very near".
 * The above string will return a token count of 4 and the following in the args array.
 *   args[0] = "The";
 *   args[1] = "end";
 *   args[2] = "is";
 *   args[3] = "very near.";		// removes the quotes from the string.
 *   args[4] = NULL;
 *
 * The quote marks can be either single or double pairs but can not mix them on a token.
 *
 *   str = string to be tokenized.  This string is modified.
 *   chrs = characters to parse string by.
 *   argz = A pointer to an array of char * pointers. This array will contain the tokens found up to (max_args - 1).
 *   max_args = max number of pointers in the pointer array argz.
 *
 *   returns number of tokens parsed which would be less than max_args.
 *           The return pointer array is null terminated.
 */
int qparse(char *str, const char *chrs, char **argz, int max_argz) {
	char *pt1, *pt = str;
	int count = 0;

	max_argz--;						/* save room for null pointer to mark */
									/* end of the list. */

	argz[count] = (char *)0;		/* set first pointer to null just in */
									/* case no parsed strings are found. */

	/* loop until we have tokenized the string based on the pattern */

	while( (pt1 = strqtok(pt, chrs)) != (char *)0 ) {
		argz[count++] = pt1;		/* found a string, save pointer */
		pt = (char *)0;				/* set to null so that strqtok */
									/* will parse the remainded of the */
									/* string. */

		argz[count] = (char *)0;	/* set next pointer to null. */

		if( count >= max_argz )		/* break when we reach max count */
			break;
	}

	return(count);					/* return count */
}
