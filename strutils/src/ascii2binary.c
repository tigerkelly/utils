/*
 * Copyright (c) 2017 Richard Kelly Wiles (rkwiles@twc.com)
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
 * ascii2binary.c
 *
 *  Created on: May 8, 2017
 *      Author: Kelly Wiles
 */


/*
 * This function converts a string for N characters and returns a string.
 *
 * Only works with base 10 numbers.
 */
int ascii2int(const char *ascii, int asciiLen) {
	int ret = 0;
	int off = 0;
	int negFlag = 0;

	if (*ascii == '-') {
		negFlag = 1;
		off = 1;
	} else if (*ascii == '+') {
		off = 1;
	}

	for(int i = off; i < asciiLen; ++i) {
		if (ascii[i] < '0' || ascii[i] > '9') {
			ret = 0;
			break;
		}
		ret = ret * 10 + (ascii[i] - '0');
	}

	return (negFlag == 1)? (ret * -1): ret;
}

/*
 * This function converts a string for N characters and returns a string.
 *
 * Only works with base 10 numbers.
 */
unsigned int ascii2uint(const char *ascii, int asciiLen) {
	unsigned int ret = 0;
	int off = 0;

	if (*ascii == '-') {
		return 0;
	} else if (*ascii == '+') {
		off = 1;
	}

	for(int i = off; i < asciiLen; ++i) {
		if (ascii[i] < '0' || ascii[i] > '9') {
			ret = 0;
			break;
		}
		ret = ret * 10 + (ascii[i] - '0');
	}

	return ret;
}

/*
 * This function converts a string for N characters and returns a string.
 *
 * Only works with base 10 numbers.
 */
long ascii2long(const char *ascii, int asciiLen) {
	long ret = 0;
	int off = 0;
	int negFlag = 0;

	if (*ascii == '-') {
		negFlag = 1;
		off = 1;
	} else if (*ascii == '+') {
		off = 1;
	}

	for(int i = off; i < asciiLen; ++i) {
		if (ascii[i] < '0' || ascii[i] > '9') {
			ret = 0;
			break;
		}
		ret = ret * 10 + (ascii[i] - '0');
	}

	return (negFlag == 1)? (ret * -1): ret;
}

/*
 * This function converts a string for N characters and returns a string.
 *
 * Only works with base 10 numbers.
 */
unsigned long ascii2ulong(const char *ascii, int asciiLen) {
	unsigned long ret = 0;
	int off = 0;

	if (*ascii == '-') {
		return 0;
	} else if (*ascii == '+') {
		off = 1;
	}

	for(int i = off; i < asciiLen; ++i) {
		if (ascii[i] < '0' || ascii[i] > '9') {
			ret = 0;
			break;
		}
		ret = ret * 10 + (ascii[i] - '0');
	}

	return ret;
}
