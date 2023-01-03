
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

#ifndef _STRUTILS_H
#define _STRUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* trim functions use strdup, original string is modified. */
int trim( char *str );
int trim_head( char *str );
int trim_tail( char *str );

/* trim*_chr functions only remove 1 type of character form head and tail. */
int trim_chr(char *str, char chr);
int trim_head_chr( char *str, char chr );
int trim_tail_chr( char *str, char chr );

/* trim*_chrs functions remove a set of characters from head and tail. */
int trim_chrs(char *str, char *chrs);
int trim_head_chrs( char *str, char *chrs );
int trim_tail_chrs( char *str, char *chrs );

/* tidy functions do NOT use strdup, original string is modified. */
char *tidy( char *str );
char *tidy_head( char *str );
void tidy_tail( char *str );

/* tidy*_chr functions only remove 1 type of character from head and tail. */
char *tidy_chr(char *str, char chr);
char *tidy_head_chr( char *str, char chr );
void tidy_tail_chr( char *str, char chr );

int parse(char *str, const char *chrs, char **argz, int max_argz);

int kqparse(char *str, const char *chrs, char **argz, int max_argz);
char *kstrqtok (char *s1, const char *s2);

int qparse(char *str, const char *chrs, char **argz, int max_argz);
char *strqtok (char *s1, const char *s2);

int tokenize(char *str, char chr, char **argz, int maxLen);

typedef struct _strParts {
    char *str;
    int len;
} StrParts;

int split(char *str, char chr, StrParts *words, int maxLen);
int splitByStr(char *str, const char *pat, StrParts *words, int maxLen);
int splitByNStr(char *str, int len, const char *pat, int patLen, StrParts *words, int maxLen);

int strEndsWith(const char *str, const char *endsWith);
int strnEndsWith(const char *str, const char *endsWith, int endsWithLen);
int strStartsWith(const char *str, int strLen, const char *startsWith, int startsWithLen);
int strnStartsWith(const char *str, const char *startsWith, int startsWithLen);

char *getTimestamp();

char *copyuntil(char *dest, const char *src, char *stopChrs);

void strDump(const char *data, int len);
void strHexDump(const char *data, int len);

void replaceChar(char *str, char chr, char replace);
void replacenChar(char *str, char chr, char replace, int len);
char *escapeQuote(char *str, char *toStr);

int int2ascii(int value, char *sp, int radix);
int uint2ascii(unsigned int value, char *sp, int radix);
int long2ascii(long value, char *sp, int radix);
int ulong2ascii(unsigned long value, char *sp, int radix);

int ascii2int(const char *ascii, int asciiLen);
unsigned int ascii2uint(const char *ascii, int asciiLen);
long ascii2long(const char *ascii, int asciiLen);
unsigned long ascii2ulong(const char *ascii, int asciiLen);

char *ip2Str(char *s, unsigned int ip);
char *mac2Str(char *s, unsigned char *mac);
char *uoi2Str(char *s, unsigned char *mac);
char *uoi2HexStr(char *s, unsigned char *mac);
int uoi2Int(unsigned char *mac);

char *sipBitString(unsigned int n, char *t);
char *tcpBitString(unsigned int n, char *t);
char *tcpBitNString(unsigned int n, char *t, int nBits);
char *tlsBitString(unsigned int n, char *t);

char **getSipNames();
char **getTcpNames();
char **getTlsNames();

bool endsWith(char *s1, char *s2);

#ifdef __cplusplus
} // closing brace for extern "C"

#endif

#endif
