/*
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
