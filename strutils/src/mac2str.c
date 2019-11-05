/*
 * ip2str.c
 *
 * Description:
 *  Created on: Nov 9, 2017
 *      Author: Kelly Wiles
 *   Copyright: Network Kinetix
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

char *mac2Str(char *s, uint8_t *mac) {
	*s = '\0';

	sprintf(s, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0],	mac[1],	mac[2],	mac[3],	mac[4],	mac[5]);

	return s;
}

char *uoi2Str(char *s, uint8_t *mac) {
	*s = '\0';

	sprintf(s, "%02x:%02x:%02x", mac[0], mac[1], mac[2]);

	return s;
}

char *uoi2HexStr(char *s, uint8_t *mac) {
	*s = '\0';

	sprintf(s, "%02x%02x%02x", mac[0], mac[1], mac[2]);

	return s;
}

int uoi2Int(uint8_t *mac) {
	char s[16];

	sprintf(s, "%02x%02x%02x", mac[0], mac[1], mac[2]);

	return (int)strtol(s, NULL, 16);
}
