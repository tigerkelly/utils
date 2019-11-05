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

/* Originally written in 2004, modified to bring it up to coding and format standards. */

#include <stdio.h>
#include <string.h>

#include "strutils.h"

/*
 * This function strEndsWith compares the end of str with endsWith.
 *
 *   str = String to compare end with.
 *   endsWith = String to compare to the end of str
 *
 *   return 0 if it does NOT match end.
 *          1 if it does match end.
 */
int strEndsWith(const char *str, const char *endsWith) {
	int ret = 1;

	int strLen = strlen(str);
	int endsWithLen = strlen(endsWith);

	if (strLen < endsWithLen)
		return 0;

	if (strcmp((str + (strLen - endsWithLen)), endsWith) != 0)
		ret = 0;

	return ret;
}

/*
 * This function strnEndsWith compares the end of str with endsWith.
 *
 *   str = String to compare end with.
 *   endsWith = String to compare to the end of str
 *   endsWithLen = length of endsWith to use.
 *
 *   return 0 if it does NOT match end.
 *          1 if it does match end.
 */
int strnEndsWith(const char *str, const char *endsWith, int endsWithLen) {
	int ret = 1;

	int strLen = strlen(str);

	if (strLen < endsWithLen)
		return 0;

	if (strncmp((str + (strLen - endsWithLen)), endsWith, endsWithLen) != 0)
		ret = 0;

	return ret;
}

/*
 * This function strStartsWith compares the end of str with startsWith.
 *
 *   str = String to compare beginning with.
 *   strLen = Length of str
 *   startsWith = String to compare to the beginning of str
 *   startsWithLen = Length of startsWith
 *   return 0 if it does NOT match end.
 *          1 if it does match end.
 */
int strStartsWith(const char *str, int strLen, const char *startsWith, int startsWithLen) {
	int ret = 1;

	if (startsWithLen > strLen)
		return 0;

	if (strncmp(str, startsWith, startsWithLen) != 0)
		ret = 0;

	return ret;
}

/*
 * This function strnStartsWith compares the end of str with startsWith.
 *
 *   str = String to compare beginning with.
 *   startsWith = String to compare to the beginning of str
 *   startsWithLen = length of startsWith to use.
 *
 *   return 0 if it does NOT match end.
 *          1 if it does match end.
 */
int strnStartsWith(const char *str, const char *startsWith, int startsWithLen) {
	int ret = 1;

	int strLen = strlen(str);

	if (startsWithLen > strLen)
		return 0;

	if (strncmp(str, startsWith, startsWithLen) != 0)
		ret = 0;

	return ret;
}
