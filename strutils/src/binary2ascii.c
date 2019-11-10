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
 * binary2ascii.c
 *
 * Used to convert integer to ascii.
 *
 *  Created on: May 4, 2017
 *      Author: Kelly Wiles
 */

// Convert a signed integer to a ASCII string.
int int2ascii(int value, char *sp, int radix) {
    char tmp[32];
    char *tp = tmp;
    int i;
    unsigned int v;

    int sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned int)value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
            *tp++ = i + '0';
        else
            *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    if (sign) {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp)
        *sp++ = *--tp;

    *sp = '\0';

    return len;
}

// Convert a unsigned integer to a ASCII string.
int uint2ascii(unsigned int value, char *sp, int radix) {
    char tmp[32];
    char *tp = tmp;
    unsigned int i;
    unsigned int v;

    v = value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
            *tp++ = i + '0';
        else
            *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    while (tp > tmp)
        *sp++ = *--tp;

    *sp = '\0';

    return len;
}

// Convert a signed long to a ASCII string.
int long2ascii(long value, char *sp, int radix) {
    char tmp[32];
    char *tp = tmp;
    long i;
    unsigned long v;

    int sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned int)value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
            *tp++ = i + '0';
        else
            *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    if (sign) {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp)
        *sp++ = *--tp;

    *sp = '\0';

    return len;
}

// Convert a unsigned long to a ASCII string.
int ulong2ascii(unsigned long value, char *sp, int radix) {
    char tmp[32];
    char *tp = tmp;
    unsigned long i;
    unsigned long v;

    v = value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
            *tp++ = i + '0';
        else
            *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    while (tp > tmp)
        *sp++ = *--tp;

    *sp = '\0';

    return len;
}
