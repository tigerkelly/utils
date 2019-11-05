
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

/* Originally written in 1993, modified to bring it up to coding and format standards. */

#ifndef _LOGUTILS_H
#define _LOGUTILS_H

void logOpen(const char *prgName, int stderrFlag, int facility);
void logClose();
void logInfo(const char *fmt, ...);
void logErr(const char *fmt, ...);
void logDebug(const char *fmt, ...);
void logNotice(const char *fmt, ...);
void logWarn(const char *fmt, ...);

int nkxLogInit();
int nkxLogCreate(char *logPath, char *logFile);
void nkxLogClose();
void nkxLogInfo(const char *fmt, ...);
void nkxLogErr(const char *fmt, ...);
void nkxLogDebug(const char *fmt, ...);
void nkxLogNotice(const char *fmt, ...);
void nkxLogWarn(const char *fmt, ...);

#endif
