
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

#ifndef _INI_H
#define _INI_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INI_FILES		16
#define MAX_SECTION_NAME	64		// max section name size
#define MAX_KEY_NAME		64		// max key name size
#define MAX_VALUE_SIZE		256		// max value size
#define MAX_COMMENT_SIZE	128		// max section comment size
#define MAX_COMMENTS		1024	// number of comments to start with.
#define MAX_SAVED_COMMENTS	2048	// number of saved comments.
#define MAX_SEC_COMMENTS	1024	// number of section comments.
#define MAX_KV_COMMENTS		2048
#define MAX_LINE_SIZE		1024	// max size of read line.

#define MAX_SECTIONS		128		// max number of sections
#define MAX_KEYS			256		// max number of key/value pairs

typedef struct _KV {
	int inUse;
	char key[MAX_KEY_NAME];
	char value[MAX_VALUE_SIZE];
	char kvComment[MAX_COMMENT_SIZE];
} KV;

typedef struct _Section {
	int inUse;
	char secName[MAX_SECTION_NAME];
	char secComment[MAX_COMMENT_SIZE];
	KV *kv;
} Section;

typedef struct _globalComment {
	int inUse;
	char secName[MAX_SECTION_NAME];
	char comment[MAX_COMMENT_SIZE];
} Comment;

typedef struct _secComment {
	int inUse;
	char secName[MAX_SECTION_NAME];
	char comment[MAX_COMMENT_SIZE];
} SecComment;

typedef struct _kvComment {
	int inUse;
	int pos;
	char secName[MAX_SECTION_NAME];
	char comment[MAX_COMMENT_SIZE];
} KvComment;

typedef struct _savedComment {
	int inUse;
	char comment[MAX_COMMENT_SIZE];
} SavedComment;

typedef struct _IniFile {
	int inUse;
	int maxSections;
	int maxKeys;
	int maxComments;
	int maxSecComments;
	char *fileName;
	Section *section;
	Comment *comments;
	SecComment *secComments;
	SavedComment *_savedComments;
	KvComment *_kvComments;
} IniFile;

IniFile *iniCreate(const char *filename);
IniFile *iniCreateBuf(const char *filename, char *buf, int maxLines);

int iniAddSection(IniFile *ini, const char *secName);
int iniAddSectionComment(IniFile *ini, const char *secName, const char *comment);
int iniAddKeyValue(IniFile *ini, const char *secName, const char *key, const char *value);
int iniAddKeyValueComment(IniFile *ini, const char *secName, const char *key, const char *comment);

int iniWalkSections(IniFile *ini, void (*funcPtr)(IniFile *ini, const char *secName));
int iniWalkSectionKeys(IniFile *ini, const char *secName, void (*funcPtr)(IniFile *ini, const char *secName, const char *key));

int iniRemoveSection(IniFile *ini, const char *secName);
int iniRemoveKey(IniFile *ini, const char *secName, const char *key);

int iniSectionExists(IniFile *ini, const char *secName);
int iniKeyExists(IniFile *ini, const char *secName, const char *keyName);
int iniGetKeyCount(IniFile *ini, const char *secName);

Section *iniGetSectionNames(IniFile *ini);
KV *iniGetSectionKeys(IniFile *ini, const char *secName);
char *iniGetSectionComment(IniFile *ini, const char *secName);

int iniGetSectionMax(IniFile *ini);
int iniGetKeyMax(IniFile *ini);
int iniGetCommentMax(IniFile *ini);
int iniGetSecCommentMax(IniFile *ini);

int iniGetBooleanValue(IniFile *ini, const char *secName, const char *key);
char *iniGetString(IniFile *ini, const char *secName, const char *key);
char *iniGetValue(IniFile *ini, const char *secName, const char *key);
int iniGetIntValue(IniFile *ini, const char *secName, const char *key);
unsigned short iniGetShortValue(IniFile *ini, const char *secName, const char *key);
char *iniGetValueComment(IniFile *ini, const char *secName, const char *key);

int iniPrint(IniFile *ini);
int iniPrintSection(IniFile *ini, const char *secName);
int iniPrintKeyValue(IniFile *ini, const char *secName, const char *key);

int iniWrite(IniFile *ini, const char *filename);
int iniWriteBuf(IniFile *ini, char *buf);
int iniSecWriteBuf(IniFile *ini, const char *secName, char *buf);

int iniIncreaseSectionSize(IniFile *ini, int maxSec);
int iniIncreaseCommentSize(IniFile *ini, int maxComment);
int iniIncreaseSecCommentSize(IniFile *ini, int maxSavedComment);
int iniIncreaseKeySize(IniFile *ini, int maxKeySize);

int iniFree(IniFile *ini);

#ifdef __cplusplus
} // closing brace for extern "C"

#endif

#endif
