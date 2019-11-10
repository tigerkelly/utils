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
 * ip2str.c
 *
 * Description:
 *  Created on: Nov 9, 2017
 *      Author: Kelly Wiles
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
