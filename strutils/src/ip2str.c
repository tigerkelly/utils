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

char *ip2Str(char *s, uint32_t ip) {
	*s = '\0';
	sprintf(s, "%d.%d.%d.%d",
			((ip >> 24) & 0xff),
			((ip >> 16) & 0xff),
			((ip >> 8) & 0xff),
			(ip & 0xff));

	return s;
}
