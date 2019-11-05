
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "miscutils.h"

void strDump(const char *data, int len) {

	pOut("");
	for (int i = 0; i < len; i++) {
		if (isalnum(data[i])) {
			printf("%c ", data[i]);
		} else if (ispunct(data[i])) {
			printf("%c ", data[i]);
		} else if (isspace(data[i])) {
			printf("%c ", data[i]);
		} else {
			printf("%02x ", (data[i] & 0xff));
		}
	}
	printf("\n");
}

void strHexDump(const char *data, int len) {

	pOut("");
	for (int i = 0; i < len; i++) {
		printf("%02x ", (data[i] & 0xff));
	}
	printf("\n");
}
