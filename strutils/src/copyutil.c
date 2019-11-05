
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
#include <string.h>

/*
 * This function copyuntil copies from the source string to the destination string
 * until one of the characters in stopChrs is found or NULL.
 *
 *   dest = destination string.
 *   src = source string.
 *   stopChrs = list of characters to stop on.
 *
 *   returns pointer to dest.
 */
char *copyuntil(char *dest, const char *src, const char *stopChrs) {
	int found = 0;
    char *sp = dest;			// save pointer of string dest.

    while( *src ) {				// copy while not null.
		if (strchr(stopChrs, *src) != NULL) {
			found++;
			break;
		}

        *dest++ = *src++;		// copy from src to dest.
	}

    *dest = '\0';				// null terminate string dest.

    return( sp );				// return pointer to dest.
}
