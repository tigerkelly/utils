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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strutils.h"
#include "miscutils.h"
#include "jsmn.h"

void jsonReset(char *jb) {
    if (jb != NULL)
        strcpy(jb, "{}");
}

char *jsonAdd(char *jb, const char *name, const char *value) {
	if (jb == NULL)
		return NULL;

    int len = strlen(jb) - 1;

    if (strcmp(jb, "{}") == 0) {
    	char *jbp = &jb[len];
    	*jbp++ = '\"';
    	for (const char *p = name; *p != '\0'; p++) {
    		if (*p == '\"') {
    			*jbp++ = '\\';
    			*jbp++ = '\"';
    		} else {
    			*jbp++ = *p;
    		}
    	}
    	*jbp++ = '\"';
    	*jbp++ = ':';

    	*jbp++ = '\"';
		for (const char *p = value; *p != '\0'; p++) {
			if (*p == '\"') {
				*jbp++ = '\\';
				*jbp++ = '\"';
			} else {
				*jbp++ = *p;
			}
		}
		*jbp++ = '\"';
		*jbp++ = '}';
		*jbp = '\0';
    } else {
    	char *jbp = &jb[len];
    	*jbp++ = ',';
		*jbp++ = '\"';
		for (const char *p = name; *p != '\0'; p++) {
			if (*p == '\"') {
				*jbp++ = '\\';
				*jbp++ = '\"';
			} else {
				*jbp++ = *p;
			}
		}
		*jbp++ = '\"';
		*jbp++ = ':';

		*jbp++ = '\"';
		for (const char *p = value; *p != '\0'; p++) {
			if (*p == '\"') {
				*jbp++ = '\\';
				*jbp++ = '\"';
			} else {
				*jbp++ = *p;
			}
		}
		*jbp++ = '\"';
		*jbp++ = '}';
		*jbp = '\0';
    }

    return jb;
}

char *jsonAddChar(char *jb, const char *name, char charValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%c", charValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddUChar(char *jb, const char *name, unsigned char charValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%c", charValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddShort(char *jb, const char *name, short shortValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%d", shortValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddUShort(char *jb, const char *name, unsigned short shortValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%u", shortValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddInt(char *jb, const char *name, int intValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%d", intValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddUInt(char *jb, const char *name, unsigned int intValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%u", intValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddIp(char *jb, const char *name, uint32_t intValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%d.%d.%d.%d",
    		((intValue >> 24) & 0xff),
			((intValue >> 16) & 0xff),
			((intValue >> 8) & 0xff),
			(intValue & 0xff));

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddLong(char *jb, const char *name, long longValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%ld", longValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddULong(char *jb, const char *name, unsigned long longValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%lu", longValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddFloat(char *jb, const char *name, float floatValue, int percision) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%.*f", percision, floatValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

char *jsonAddLongLong(char *jb, const char *name, unsigned long long longLongValue) {

    int len = strlen(jb) - 1;

    char tmp[64];
    sprintf(tmp, "%lld", longLongValue);

    if (strcmp(jb, "{}") == 0)
        sprintf(&jb[len], "\"%s\":\"%s\"}", name, tmp);
    else
        sprintf(&jb[len], ",\"%s\":\"%s\"}", name, tmp);

    return jb;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

/*
 * This function jsonGet while fill the buffer with the value part
 * of the key/value pair if the name/key is found.
 *
 *   jb = JSON string.
 *   name = name/key to look for.
 *   buf = buffer to place the value part of the key/value pair.
 *
 *   return -1 on error
 *          else number of byte copied to buf
 */
int jsonGet(const char *jb, const char *name, char *buf) {
	int ret = -1;
	jsmntok_t t[1024];
	jsmn_parser p;

	jsmn_init(&p);
	int r = jsmn_parse(&p, jb, strlen(jb), t, sizeof(t)/sizeof(t[0]));

	if (r < 1 || t[0].type != JSMN_OBJECT) {
//		pOut("JSON Object expected\n");
		return -1;
	}

	if (buf == NULL)
		return -2;

	*buf = '\0';

	/* Loop over all keys of the root object */
	for (int i = 1; i < r; i++) {
		if (jsoneq(jb, &t[i], name) == 0) {
			ret = t[i+1].end-t[i+1].start;
			strncpy(buf, jb + t[i+1].start, ret);
			buf[ret] = '\0';
			break;
		}
	}

	return ret;
}

int jsonFind(const char *jb, const char *name) {
	int ret = 0;
	jsmntok_t t[128];
	jsmn_parser p;

	int r = jsmn_parse(&p, jb, strlen(jb), t, sizeof(t)/sizeof(t[0]));

	if (r < 1 || t[0].type != JSMN_OBJECT) {
//		pOut("JSON Object expected\n");
		return ret;
	}

	/* Loop over all keys of the root object */
	for (int i = 1; i < r; i++) {
		if (jsoneq(jb, &t[i], name) == 0) {
			ret = 1;
			break;
		}
	}

	return ret;
}

int jsonGetInt(const char *jb, const char *name, int *intValue) {
	int ret = -1;
	jsmntok_t t[128];
	jsmn_parser p;

	int r = jsmn_parse(&p, jb, strlen(jb), t, sizeof(t)/sizeof(t[0]));

	if (r < 1 || t[0].type != JSMN_OBJECT) {
//		pOut("JSON Object expected\n");
		return ret;
	}

	char tmp[32];
	if (jsonGet(jb, name, tmp) > 0) {
		*intValue = atoi(tmp);
		ret = 0;
	}

	return ret;
}

int jsonGetLong(const char *jb, const char *name, long *longValue) {
	int ret = -1;
	jsmntok_t t[128];
	jsmn_parser p;

	int r = jsmn_parse(&p, jb, strlen(jb), t, sizeof(t)/sizeof(t[0]));

	if (r < 1 || t[0].type != JSMN_OBJECT) {
//		pOut("JSON Object expected\n");
		return ret;
	}

	char tmp[32];
	if (jsonGet(jb, name, tmp) > 0) {
		*longValue = strtol(tmp, NULL, 10);
		ret = 0;
	}

	return ret;
}

int jsonGetLongLong(const char *jb, const char *name, unsigned long long *longLongValue) {
	int ret = -1;
	jsmntok_t t[128];
	jsmn_parser p;

	int r = jsmn_parse(&p, jb, strlen(jb), t, sizeof(t)/sizeof(t[0]));

	if (r < 1 || t[0].type != JSMN_OBJECT) {
//		pOut("JSON Object expected\n");
		return ret;
	}

	char tmp[32];
	if (jsonGet(jb, name, tmp) > 0) {
		*longLongValue = strtoll(tmp, NULL, 10);
		ret = 0;
	}

	return ret;
}


static int jsonobjeq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int jsonObjGet(const char *jb, const char *name, char *buf) {
	int ret = -1;
	jsmntok_t t[1024];
	jsmn_parser p;

	jsmn_init(&p);
	int r = jsmn_parse(&p, jb, strlen(jb), t, sizeof(t)/sizeof(t[0]));

	if (r < 1 || t[0].type != JSMN_OBJECT) {
//		pOut("JSON Object expected\n");
		return -1;
	}

	if (buf == NULL)
		return -2;

	*buf = '\0';

	/* Loop over all keys of the root object */
	for (int i = 1; i < r; i++) {
		if (jsonobjeq(jb, &t[i], name) == 0) {
			ret = t[i+1].end-t[i+1].start;
			strncpy(buf, jb + t[i+1].start, ret);
			buf[ret] = '\0';
			break;
		}
	}

	return ret;
}
