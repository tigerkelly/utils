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
