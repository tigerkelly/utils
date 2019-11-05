
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

#define DQUOTE	'"'
#define SQUOTE	'\''

/*
 * This function kstrqtok will parse a string and keep quoted string together.
 *
 *   s1 = string to tokenize.
 *   s2 = pattern of separator characters.
 */
char *kstrqtok (char *s1, const char *s2) {
	static	char	*sp = NULL;	/* keep track of position */
	char	*begin;
	char	q;

	if( s1 == NULL )	// if passed in a NULL then use previous string.
		s1 = sp;

	/* skip leading separator chars */

	while( *s1 && strchr( s2, *s1 ) != NULL )
		s1++;

	/* if the token begins with nul, end of string reached so no */
	/* more tokens, return NULL.  */

	if( *s1 == '\0' )
		return (NULL);

	/* if the token begins with single or double quotes, search that */

	if( *s1 == SQUOTE || *s1 == DQUOTE ) {
		begin = s1;					/* save begining quote mark */
		q = *s1++;					/* save quote type. */
		while( *s1 && *s1 != q )	/* find matching quote mark. */
			s1++;
		s1++;						/* move past ending quote mark */

		/* while we have not reached a separator character.  We could */
		/* have a string like    abc 'def'ghi */

		while( *s1 && strchr( s2, *s1 ) == NULL )
			s1++;
	} else {							/* else look for next separator char */
		begin = s1;
		while( *s1 && strchr( s2, *s1 ) == NULL )
			s1++;
	}

	/* terminate token */

	if( *s1 )
		*s1++ = '\0';

	/* save position for next call */

	sp = s1;

	/* return pointer to beginning of token */

	return (begin);
}
