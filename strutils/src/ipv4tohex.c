/*
 * ipv4tohex.c
 *
 * Description: Convert an IPv4 (uint32_t) to hex ASCII string.
 *  Created on: Sep 5, 2017
 *      Author: Kelly Wiles
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

char _asciiHex[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

char *ipv4ToHex(uint32_t ipv4, char *b) {
	int i = 0;

	int x = (ipv4 >> 28) & 0x0F;
	b[i++] = _asciiHex[x];

	x = (ipv4 >> 24) & 0x0F;
	b[i++] = _asciiHex[x];

	x = (ipv4 >> 20) & 0x0F;
	b[i++] = _asciiHex[x];

	x = (ipv4 >> 16) & 0x0F;
	b[i++] = _asciiHex[x];

	x = (ipv4 >> 12) & 0x0F;
	b[i++] = _asciiHex[x];

	x = (ipv4 >> 8) & 0x0F;
	b[i++] = _asciiHex[x];

	x = (ipv4 >> 4) & 0x0F;
	b[i++] = _asciiHex[x];

	x = ipv4 & 0x0F;
	b[i++] = _asciiHex[x];

	b[i] = '\0';

	return b;
}

uint32_t hexTpIpv4(char *ipv4Hex) {

	return (uint32_t)(strtol(ipv4Hex, NULL, 16) & 0xffffffff);
}
