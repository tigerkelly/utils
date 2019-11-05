
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

/*
 * NOTE: This software modifies the string being passed in.  This is so that the caller
 *       does not have to free any memory or pass in another pointer to place the result into..
 *       If you need a version that passes back a pointer to a allocated memory or you need
 *       to pass in a pointer to place the result into then this library is not for you.
 */

/* Originally written in 1988, modified to include it into a libstrutils.a archive and coding/format standards. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "strutils.h"

/*
 * The trim function removes leading and trailing white spaces characters from the string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *
 *   returns 0 if the string is NULL or begins with a '\0' character
 *           else the number of characters in the string after the trim.
 */
int trim( char *str ) {
	int i;

	if( str == (char *)NULL || *str == '\0' )
		return(0);

	trim_head( str );
	i = trim_tail( str );

	return( i );
}

/*
 * The trim_head function only trims leading white space characters from the string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *
 *   returns 0 if the string is NULL or begins with a '\0' character
 *           else the number of characters in the string after the trim.
 */
int trim_head( char *str ) {
	char *pt;

	if( str == NULL || *str == '\0' )
		return(0);

	pt = str;
	while ( isspace(*pt))
		pt++;

	if (pt > str) {
		// shift it down.
		while (*pt != '\0')
			*str++ = *pt++;
		*str = '\0';
	}

	return( strlen( str ) );

}

/*
 * The trim_tail function only trims trailing white space characters from the string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *
 *   returns 0 if the string is NULL or begins with a '\0' character
 *           else the number of characters in the string after the trim.
 */
int trim_tail( char *str ) {

	if( str == (char *)NULL || *str == '\0' )
		return(0);

	int len = strlen(str);

	for( int i = len - 1; i>= 0; i-- ) {
		if( isspace( str[i] ) )
			str[i] = '\0';
		else
			break;
	}

	return( strlen( str ) );
}

int trim_chrs( char *str, char *chrs ) {
	int i;

	if( str == (char *)NULL || *str == '\0' )
		return(0);

	trim_head_chrs( str, chrs );
	i = trim_tail_chrs( str, chrs);

	return( i );
}

int trim_head_chrs( char *str, char *chrs ) {
	char *pt;

	if( str == NULL || *str == '\0' )
		return(0);

	pt = str;
	while ( *pt) {
		int cnt = 0;
		for (char *cp = chrs; *cp != '\0'; cp++) {
			if (*pt == *cp)
				cnt++;
		}
		if (cnt == 0)
			break;
		pt++;
	}

	if (pt > str) {
		// shift it down.
		while (*pt != '\0')
			*str++ = *pt++;
		*str = '\0';
	}

	return( strlen( str ) );

}

int trim_tail_chrs( char *str, char *chrs ) {

	if( str == (char *)NULL || *str == '\0' )
		return(0);

	int len = strlen(str);

	for( int i = len - 1; i>= 0; i-- ) {
		int cnt = 0;
		for (char *cp = chrs; *cp != '\0'; cp++) {
			if (str[i] == *cp) {
				cnt++;
				str[i] = '\0';
			}
		}
		if (cnt == 0)
			break;
	}

	return( strlen( str ) );
}

/*
 * The tidy function removes leading and trailing white spaces characters from the string.
 * This will return a pointer to the first non-whitespace of the string and change all
 * whitespaces at end of string are changed to NULLs.  Does not use strdup.
 *
 *   str = The string to be trimmed and returning tidied string.  This string is modified.
 *
 *   returns pointer to new start of string.
 */
char *tidy( char *str ) {
	char *p = str;
	if( str == (char *)NULL || *str == '\0' )
		return(p);

	p = tidy_head(str);

	tidy_tail(p);

	return p;
}

/*
 * The tidy_head function only trims leading white space characters from the string.
 *
 *   str = The string to be trimmed.  This string is NOT modified.
 *
 *   returns pointer to first non-whitespace character in string.
 */
char *tidy_head( char *str ) {
	char *p = str;

	if( str == (char *)NULL || *str == '\0' )
		return(p);

	while( isspace( *p ) )
		p++;		// move pointer to first non-whitespace

	return p;
}

/*
 * The tidy_tail function only trims trailing white space characters from the string.
 * All whitespaces at end of string are changed to NULLs.
 *
 *   str = The string to be trimmed.  This string is modified.
 *
 *   returns void
 */
void tidy_tail( char *str ) {
	if( str == (char *)NULL || *str == '\0' )
		return;

	int len = strlen(str);
	char *e = (str + (len - 1));
	for( int i = len - 1; i>= 0; i-- ) {
		if( isspace( e[i] ) )
			e[i] = '\0';
		else
			break;
	}
}

/*
 * This function trim_chr trims character (chr) from the leading and trailing string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *   chr = The character to match for trimming.
 *
 *   returns 0 if the string is NULL or begins with a '\0' character
 *           else the number of characters in the string after being trimmed.
 */
int trim_chr(char *str, char chr) {
	int i;

	if( str == (char *)NULL || *str == '\0' )
		return(0);

	trim_head_chr( str, chr );
	i = trim_tail_chr( str, chr );

	return( i );
}

/*
 * This function trim_head_chr trims leading character (chr) from string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *   chr = The character to match for trimming.
 *
 *   returns 0 if the string is NULL or begins with a '\0' character
 *           else the number of characters in the string after being trimmed.
 */
int trim_head_chr( char *str, char chr ) {
//	char *new;
	char *pt;

	if( str == NULL || *str == '\0' )
		return(0);

	pt = str;
	while (*pt == chr)
		pt++;

	if (pt > str) {
		// shift it down.
		while (*pt != '\0')
			*str++ = *pt++;
		*str = '\0';
	}
	return( strlen( str ) );

}

/*
 * This function trim_tail_chr trims trailing character (chr) from string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *   chr = The character to match for trimming.
 *
 *   returns 0 if the string is NULL or begins with a '\0' character
 *           else the number of characters in the string after being trimmed.
 */
int trim_tail_chr( char *str, char chr ) {
	int i;
	int len;

	if( str == (char *)NULL || *str == '\0' )
		return(0);

	len = strlen(str);

	for( i = len - 1; i >= 0; i-- ) {
		if( str[i] == chr )
			str[i] = '\0';
		else
			break;
	}

	return( strlen( str ) );
}

/*
 * This function tidy_chr trims character (chr) from the leading and trailing string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *   chr = The character to match for trimming.
 *
 *   returns pointer to first non-whitespace character
 */
char *tidy_chr(char *str, char chr) {
	char *p = str;

	if( str == (char *)NULL || *str == '\0' )
		return p;

	p = tidy_head_chr(str, chr);
	tidy_tail_chr(p, chr);

	return p;
}

/*
 * This function tidy_head_chr trims leading character (chr) from string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *   chr = The character to match for trimming.
 *
 *   returns pointer to fist non-whitespace character.
 */
char *tidy_head_chr(char *str, char chr) {
	char *p = str;

	if( str == (char *)NULL || *str == '\0' )
		return(p);

	while( *p == chr )
		p++;		// move pointer to first non-whitespace

	return p;
}

/*
 * This function tidy_tail_chr trims trailing character (chr) from string.
 *
 *   str = The string to be trimmed and returning trimmed string.  This string is modified.
 *   chr = The character to match for trimming.
 *
 *   returns void
 */
void tidy_tail_chr(char *str, char chr) {
	int len = strlen(str);

	char *e = (str + (len - 1));
	for( int i = len - 1; i >= 0; i-- ) {
		if( e[i] == chr )
			e[i] = '\0';
		else
			break;
	}
}
