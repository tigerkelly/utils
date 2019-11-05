
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
 *
 * This library kind of duplicates the IniFile library I wrote in Java.
 * An ini type file contains sections with key/value pairs like:
 *   [Sec1]
 *      key1 = value1
 *      key2 = value2
 *   [end]
 *   [Sec2]
 *      key1 = value1
 *      key2 = value2
 *   [end]
 *   [Sec3]
 *      ...
 *   [end]
 *
 * This gives you a unique key of Section/Key for each value stored.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/stat.h>

#include "ini.h"
#include "strutils.h"
#include "logutils.h"
#include "miscutils.h"

#define GLOBAL_NAME		"$Global"

SavedComment *_savedComments;
KvComment *_kvComments;

pthread_mutex_t lock;

int _iniLoad(IniFile *ini, const char *filename);
int _iniLoadBuf(IniFile *ini, char *buf, int maxLines);

int _iniAddSectionWithComment(IniFile *ini, const char *secName, const char *comment);
int _iniAddKeyValueWithComment(IniFile *ini, const char *secName, const char *key, const char *value, const char *comment);

int _iniAddGlobalComment(IniFile *ini, const char *comment, const char *secName);
int _iniAddSecComment(IniFile *ini, const char *comment, const char *secName);
int _iniAddKvComment(IniFile *ini, const char *comment, const char *secName, int idx);
int _iniAddSavedComment(IniFile *ini, const char *comment);
void _iniClearSavedComment(IniFile *ini);
void _iniClearKvComment(IniFile *ini);

/* NOTE: Added comment support.
 * Comments in the ini file have the following format.
 *
 * A global comment starts with a '#' and end with a newline and must be on a line by them self.
 * These are comments that do not belong to a section name and if found any were
 * else within the file they are moved to the top of the file if iniWrite is called.
 *
 * All other comments start with a ';' and end with a newline.  This can be
 * at the end of a section name or key/value pair.
 * To have multiple comments for a section, see example below.
 *
 *    # Global comment start with a '#' and must
 *    # not contain section name or key/value stuff.
 *
 *    ; This a section comment block
 *    ; used to describe a section name.
 *    [section name]		; section name comment.
 *    		key = value		; key/value comment.
 *    [end]
 *
 */

/*
 * Initialize the sizes.
 * This function allocates the memory needed based on the values passed in.
 * This MUST be called before all other functions.
 *
 *   maxCount = max number of ini that can be created, if <= 0 then use MAX_INI_FILES(16).
 *   maxSect = max number of sections per ini, if <= 0 the use MAX_SECTIONS(32).
 *   maxKey = max number of keys per section, if <= 0 then use MAX_KEYS(16).
 *
 *   returns pointer to an IniFile structure else NULL of failure.
 */
IniFile *iniInit() {

	/* I could have allocated the memory on a as needed bases but I like the idea that the memory
	 * is allocated in blocks and not fragmented.
	 * Much friendlier in an embedded environment.
	 * Plus the resizing is very simple in the iniIncreaseSectionSize() and iniIncreaseKeySize() functions.
	 */

	IniFile *ini = (IniFile *)calloc(1, sizeof(IniFile));
	ini->maxSections = MAX_SECTIONS;
	ini->maxKeys = MAX_KEYS;
	ini->maxComments = MAX_COMMENTS;
	ini->maxSecComments = MAX_SEC_COMMENTS;
	ini->fileName = (char *)calloc(1, 1024);
	ini->section = (Section *)calloc(ini->maxSections, sizeof(Section));	// allocate section array per IniFile
	ini->comments = (Comment *)calloc(ini->maxComments, sizeof(Comment));	// allocate comment array per IniFile
	ini->secComments = (SecComment *)calloc(ini->maxSecComments, sizeof(SecComment));
	ini->_kvComments = (KvComment *)calloc(MAX_KV_COMMENTS, sizeof(KvComment));
	Section *p = ini->section;
	for (int x = 0; x < ini->maxSections; x++, p++) {
		p->kv = (KV *)calloc(ini->maxKeys, sizeof(KV));	// allocate key/value array per section.
	}

	ini->_savedComments = (SavedComment *)calloc(MAX_SAVED_COMMENTS, sizeof(SavedComment));

	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("%s (%d): Mutex init lock failed.\n", __FILE__, __LINE__);
		return NULL;
	}

	return ini;
}

/*
 * Create a new IniFile.
 * This function finds an empty iniFiles slot.
 *   filename = Path to ini type file. If NULL then creates an empty IniFile else fills IniFile from file.
 *
 *   returns NULL on error
 *           else pointer to IniFile structure.
 */
IniFile *iniCreate(const char *filename) {
	IniFile *ini = NULL;
	if (filename != NULL) {
		if (access(filename, R_OK | W_OK) == -1) {
			fprintf(stderr, "File %s, %s\n", filename, strerror(errno));
			return NULL;
		}
		ini = iniInit();
//		ini->filename = (char *)calloc(1, (strlen(filename) + 1));
		strcpy(ini->fileName, filename);
		_iniLoad(ini, filename);
	} else {
		ini = iniInit();
	}

	return ini;
}

/*
 * Create a new IniFile.
 * This function finds an empty iniFiles slot.
 *   filename = Path to ini type file. If NULL then creates an empty IniFile else fills IniFile from file.
 *
 *   returns NULL on error
 *           else pointer to IniFile structure.
 */
IniFile *iniCreateBuf(const char *filename, char *buf, int maxLines) {
	IniFile *ini = NULL;
	if (buf == NULL || filename == NULL)
		return NULL;
	ini = iniInit();
	strcpy(ini->fileName, filename);
	_iniLoadBuf(ini, buf, maxLines);

	return ini;
}

/*
 * Loads the IniFile from the buffer given.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   buf = buffer to parse.
 *   maxLines = Max number of lines in buffer.
 *
 *   returns 0;
 */
int _iniLoadBuf(IniFile *ini, char *buf, int maxLines) {

	char *comment;
	char secName[MAX_SECTION_NAME];
	char *args[maxLines+1];
	int nargs = tokenize(buf, '\n', args, maxLines);

	secName[0] = '\0';
	int kvLineCnt = 0;

	for (int i = 0; i < nargs; i++) {
		int len = trim(args[i]);		// trim whitespace from head and tail.

		if (len <= 0)
			continue;					// line has no length

		comment = NULL;
		char *p = strchr(args[i], ';');	// look for comment start
		if (p != NULL) {
			comment = (p + 1);
			*p = '\0';			// replaces ';' with null
			// should be a section comment.
			if (secName[0] == '\0' && args[i][0] != '[') {
				_iniAddSavedComment(ini, comment);
			}
		} else {
			p = strchr(args[i], '#');	// look for global comment start
			if (p != NULL) {
				comment = (p + 1);
				*p = '\0';			// replaces '#' with null
				if (comment != NULL && secName[0] != '\0') {
					_iniAddGlobalComment(ini, comment, GLOBAL_NAME);
				}
				continue;
			}
		}

		len = trim(args[i]);		// trim whitespace from head and tail.

		if (len <= 0 && comment == NULL)
			continue;

		if (strncasecmp(args[i], "[end]", 5) == 0) {
			SavedComment *sc = ini->_savedComments;
			for (int k = 0; k < MAX_SAVED_COMMENTS; k++, sc++) {
				if (sc->inUse == 0)
					continue;

				_iniAddSecComment(ini, sc->comment, secName);
			}
			_iniClearSavedComment(ini);

			secName[0] = '\0';

			continue;
		}

		if (args[i][0] == '[') {
			trim_head_chr(args[i], '[');		// trim '[' from head of string.
			trim_tail_chr(args[i], ']');		// trim '[' from tail of string.

			if (iniSectionExists(ini, args[i]) == 0) {
				// add section if it does not exist.
				_iniAddSectionWithComment(ini, args[i], comment);
				strcpy(secName, args[i]);			// save section name

				SavedComment *sc = ini->_savedComments;
				for (int k = 0; k < MAX_SAVED_COMMENTS; k++, sc++) {
					if (sc->inUse == 0)
						continue;

					_iniAddSecComment(ini, sc->comment, secName);
				}

				_iniClearSavedComment(ini);
			}
			kvLineCnt = 0;
		} else {
			char *p = strchr(args[i], '=');		// find first '='
			if (p != NULL) {
				char keyStr[MAX_KEY_NAME];
				char value[MAX_VALUE_SIZE];

				memset(keyStr, 0, sizeof(keyStr));
				memset(value, 0, sizeof(value));

				strncpy(keyStr, args[i], (p - args[i]));
				trim(keyStr);

				strcpy(value, ++p);
				trim(value);

				// add key/value pair to section.
				_iniAddKeyValueWithComment(ini, secName, keyStr, value, comment);

				kvLineCnt++;
			} else {
				// If a comment appears in the key/value list area.
				if (secName[0] != '\0') {
					_iniAddKvComment(ini, comment, secName, kvLineCnt);
					kvLineCnt++;
				}
			}
		}
	}

	return 0;
}

/*
 * Loads the IniFile with the filename given.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   filename = file to read.
 *
 *   returns 0;
 */
int _iniLoad(IniFile *ini, const char *filename) {
	FILE *fp = NULL;
	char line[MAX_LINE_SIZE];
	char *comment;
	char secName[MAX_SECTION_NAME];
	int kvLineCnt = 0;

	fp = fopen(filename, "r");			// open file for reading.
	if (fp != NULL) {

		secName[0]= '\0';

		int lineCnt = 0;
		while (fgets(line, sizeof(line), fp) != NULL) {
			int len = trim(line);			// trim whitespace from head and tail.

			if (len <= 0)
				continue;					// line has no length

			comment = NULL;
			char *p = strchr(line, ';');	// look for comment start
			if (p != NULL) {
				comment = (p + 1);
				*p = '\0';					// replaces ';' with null
				// should be a section comment.
				if (secName[0] == '\0' && line[0] != '[') {
					_iniAddSavedComment(ini, comment);
				}
			} else {
				p = strchr(line, '#');		// look for global comment start
				if (p != NULL) {
					comment = (p + 1);
					*p = '\0';				// replaces '#' with null
					if (comment != NULL) {
						_iniAddGlobalComment(ini, comment, GLOBAL_NAME);
					}
					continue;
				}
			}

			len = trim(line);		// trim whitespace from head and tail.

			if (len <= 0 && comment == NULL)
				continue;

			if (strncasecmp(line, "[end]", 5) == 0) {
				SavedComment *sc = ini->_savedComments;
				for (int k = 0; k < MAX_SAVED_COMMENTS; k++, sc++) {
					if (sc->inUse == 0)
						continue;

					_iniAddSecComment(ini, sc->comment, secName);
				}
				_iniClearSavedComment(ini);

				secName[0] = '\0';

				continue;
			}

			if (line[0] == '[') {
				// It is a section name.
				trim_head_chr(line, '[');		// trim '[' from head of string.
				trim_tail_chr(line, ']');		// trim '[' from tail of string.

				if (iniSectionExists(ini, line) == 0) {
					// add section if it does not exist.
					_iniAddSectionWithComment(ini, line, comment);
					strcpy(secName, line);			// save section name
				}
				kvLineCnt = 0;
			} else {
				// It is a key/value pair, instead of a section name.
				char *p = strchr(line, '=');		// find first '='
				if (p != NULL) {
					char keyStr[MAX_KEY_NAME];
					char value[MAX_VALUE_SIZE];

					memset(keyStr, 0, sizeof(keyStr));
					memset(value, 0, sizeof(value));

					strncpy(keyStr, line, (p - line));
					trim(keyStr);

					strcpy(value, ++p);
					trim(value);

					// add key/value pair to section.
					_iniAddKeyValueWithComment(ini, secName, keyStr, value, comment);

					kvLineCnt++;
				} else {
					// If a comment appears in the key/value list area.
					if (secName[0] != '\0') {
						_iniAddKvComment(ini, comment, secName, kvLineCnt);
						kvLineCnt++;
					}
				}
			}

			lineCnt++;
		}
	}

//	printf("+++++++++++++++++++++++++++++==\n");
//	KvComment *kvc = _kvComments;
//	for (int x = 0; x < MAX_KV_COMMENTS; x++, kvc++) {
//		if (kvc->inUse == 0)
//			continue;
//
//		printf("%s, %d, %d\n", kvc->comment, kvc->inUse, kvc->pos);
//	}
//
//	SecComment *scp = ini->secComments;
//	for (int x = 0; x < ini->maxSecComments; x++, scp++) {
//		if (scp->inUse == 0)
//			continue;
//
//			printf("%s, %s, %d, %d\n", scp->secName, scp->comment, scp->inUse, x);
//	}
//	printf("+++++++++++++++++++++++++++++==\n");

//	exit(0);

	return 0;
}

/*
 * Adds a global comment to the comment list.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   comment = Comment to add.
 *   secName = Section name comment applies to.
 *
 *   returns -1 if ini is NULL
 *           -2 if comment is too large.
 *           0 on success.
 */
int _iniAddGlobalComment(IniFile *ini, const char *comment, const char *secName) {
	if (ini == NULL)
		return -1;
	if (strlen(comment) > MAX_COMMENT_SIZE)
		return -2;

	pthread_mutex_lock(&lock);

	Comment *c = ini->comments;

	int foundSlot = -1;
	for (int i = 0; i < ini->maxComments; i++, c++) {
		if (c->inUse == 1) {
			continue;
		} else {
			foundSlot = i;
			c->inUse = 1;
			strcpy(c->secName, secName);
			strcpy(c->comment, comment);
			break;
		}
	}

	if (foundSlot == -1) {		// no empty slots left.
		iniIncreaseCommentSize(ini, iniGetCommentMax(ini) + MAX_COMMENT_SIZE);
		return _iniAddGlobalComment(ini, comment, secName);
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Adds a section comment to the comment list.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   comment = Comment to add.
 *   secName = Section name comment applies to.
 *
 *   returns -1 if ini is NULL
 *           -2 if comment is too large.
 *           0 on success.
 */
int _iniAddSecComment(IniFile *ini, const char *comment, const char *secName) {
	if (ini == NULL)
		return -1;
	if (strlen(comment) > MAX_COMMENT_SIZE)
		return -2;

	pthread_mutex_lock(&lock);

	SecComment *sc = ini->secComments;

	int foundSlot = -1;
	for (int i = 0; i < ini->maxSecComments; i++, sc++) {
		if (sc->inUse == 1) {
			continue;
		} else {
			foundSlot = i;
			sc->inUse = 1;
			strcpy(sc->secName, secName);
			strcpy(sc->comment, comment);
			break;
		}
	}

	if (foundSlot == -1) {		// no empty slots left.
		iniIncreaseSecCommentSize(ini, iniGetSecCommentMax(ini) + MAX_COMMENT_SIZE);
		return _iniAddSecComment(ini, comment, secName);
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Adds a saved section comment to the comment list.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   comment = Comment to add.
 *   secName = Section name comment applies to.
 *
 *   returns -1 if ini is NULL
 *           -2 if comment is too large.
 *           0 on success.
 */
int _iniAddSavedComment(IniFile *ini, const char *comment) {
	if (ini == NULL)
		return -1;
	if (strlen(comment) > MAX_COMMENT_SIZE)
		return -2;

	pthread_mutex_lock(&lock);

	SavedComment *sc = ini->_savedComments;

	int foundSlot = -1;
	for (int i = 0; i < MAX_SAVED_COMMENTS; i++, sc++) {
		if (sc->inUse == 1) {
			continue;
		} else {
			foundSlot = i;
			sc->inUse = 1;
			strcpy(sc->comment, comment);
			break;
		}
	}

	if (foundSlot == -1) {		// no empty slots left.
		printf("No empty slots for saved comments.  Dropping comment.\n");
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Adds a key/value comment to the comment list.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   comment = Comment to add.
 *   secName = Section name comment applies to.
 *
 *   returns -1 if ini is NULL
 *           -2 if comment is too large.
 *           0 on success.
 */
int _iniAddKvComment(IniFile *ini, const char *comment, const char *secName, int idx) {
	if (ini == NULL)
		return -1;
	if (strlen(comment) > MAX_COMMENT_SIZE)
		return -2;

	pthread_mutex_lock(&lock);

	KvComment *kvc = ini->_kvComments;

	int foundSlot = -1;
	for (int i = 0; i < MAX_KV_COMMENTS; i++, kvc++) {
		if (kvc->inUse == 1) {
			continue;
		} else {
			foundSlot = i;
			kvc->inUse = 1;
			kvc->pos = idx;
			strcpy(kvc->secName, secName);
			strcpy(kvc->comment, comment);
			break;
		}
	}

	if (foundSlot == -1) {		// no empty slots left.
		printf("No empty slots for key/value comments.  Dropping comment.\n");
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

void _iniClearSavedComment(IniFile *ini) {
	SavedComment *sc = ini->_savedComments;

	for (int i = 0; i < MAX_SAVED_COMMENTS; i++, sc++) {
		sc->inUse = 0;
	}
}

void _iniClearKvComment(IniFile *ini) {
	KvComment *kvc = ini->_kvComments;

	for (int i = 0; i < MAX_KV_COMMENTS; i++, kvc++) {
		kvc->inUse = 0;
	}
}

/*
 * Adds a new Section if their is a free slot.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = Name of section to create if it does not already exist.
 *
 *   returns -1 if iniIdx is out of range.
 *           -2 if section name is to long.
 *           0 if successful.
 */
int iniAddSection(IniFile *ini, const char *secName) {
	return _iniAddSectionWithComment(ini, secName, NULL);
}

/*
 * Get the section comment.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = Name of section to get comment for.
 *
 *   returns comment or NULL
 */
char *iniGetSectionComment(IniFile *ini, const char *secName) {
	char *sc = NULL;

	if (ini == NULL)
		return sc;

	pthread_mutex_lock(&lock);

	Section *sec = ini->section;

	for (int i = 0; i < ini->maxSections; i++, sec++) {
		if (sec->inUse == 1 && strcasecmp(sec->secName, secName) == 0) {
			// found matching section name
			sc = sec->secComment;
			break;
		}
	}

	pthread_mutex_unlock(&lock);

	return sc;
}

/*
 * Add or replace section comment.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = Name of section to add comment for.
 *   comment = Comment to add or replace.
 *
 *   return -1 if ini is NULL
 *          -2 secName is too large.
 *          0 on success.
 */
int iniAddSectionComment(IniFile *ini, const char *secName, const char *comment) {
	if (ini == NULL)
		return -1;
	if (strlen(secName) > MAX_SECTION_NAME)
		return -2;

	pthread_mutex_lock(&lock);

	Section *sec = ini->section;

	Section *foundSec = NULL;
	for (int i = 0; i < ini->maxSections; i++, sec++) {
		if (sec->inUse == 1 && strcasecmp(sec->secName, secName) == 0) {
			// found matching section name
			foundSec = sec;
			break;
		}
	}

	if (foundSec != NULL) {
		strcpy(sec->secComment, comment);
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Adds a new Section if their is a free slot.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = Name of section to create if it does not already exist.
 *
 *   returns -1 if iniIdx is out of range.
 *           -2 if section name is to long.
 *           0 if successful.
 */
int _iniAddSectionWithComment(IniFile *ini, const char *secName, const char *comment) {
	if (ini == NULL)
		return -1;
	if (strlen(secName) > MAX_SECTION_NAME)
		return -2;

	pthread_mutex_lock(&lock);

	int secFound = 0;
	int foundSlot = -1;
	Section *sec = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sec++) {		// loop thur sections
		if (sec->inUse == 0 && foundSlot == -1)			// find first free slot
			foundSlot = i;

		if (sec->inUse == 1 && strcasecmp(sec->secName, secName) == 0) {
			secFound = 1;		// section already exists.
			break;
		}
	}

	if (foundSlot != -1) {
		sec = ini->section;
		sec += foundSlot;
		strcpy(sec->secName, secName);
		if (comment != NULL)
			strcpy(sec->secComment, comment);
		sec->inUse = 1;
	} else if (secFound == 0){
		pthread_mutex_unlock(&lock);
		iniIncreaseSectionSize(ini, iniGetSectionMax(ini) + MAX_SECTIONS);
		return iniAddSection(ini, secName);
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Check to see if section exists.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = Section name to look for.
 *
 *   returns -1 if iniIdx is out of range.
 *           1 if section already exists.
 *           0 if section does not exist.
 */
int iniSectionExists(IniFile *ini, const char *secName) {
	if (ini == NULL)
		return -1;

	Section *sec = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sec++) {
		if (sec->inUse == 1 && strcasecmp(sec->secName, secName) == 0) {
			return 1;
		}
	}

	return 0;
}

/*
 * Check to see if section and key exists.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = Section name to look for.
 *   keyName = Key name to look for.
 *
 *   returns -1 if iniIdx is out of range.
 *           1 if section and key already exists.
 *           0 if section and key does not exist.
 */
int iniKeyExists(IniFile *ini, const char *secName, const char *keyName) {
	if (ini == NULL)
		return -1;

	Section *sec = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sec++) {
		if (sec->inUse == 1 && strcasecmp(sec->secName, secName) == 0) {
			char *p = iniGetValue(ini, secName, keyName);
			if (p == NULL)
				return 0;
			else
				return 1;
		}
	}

	return 0;
}

/*
 * Get pointer to Section array.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *
 *   returns Section *
 */
Section *iniGetSectionNames(IniFile *ini) {
	if (ini == NULL)
		return NULL;
	return ini->section;
}

/*
 * Get pointer to KV array in the section found.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name to look for.
 *
 *   returns KV *
 */
KV *iniGetSectionKeys(IniFile *ini, const char *secName) {
	if (ini == NULL)
		return NULL;

	KV *kp = NULL;
	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (strcasecmp(sp->secName, secName) == 0) {
			kp = sp->kv;
			break;
		}
	}
	return kp;
}

/*
 * Return number of keys in section.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name to look for.
 *
 *   returns int
 */
int iniGetKeyCount(IniFile *ini, const char *secName) {
	if (ini == NULL)
		return 0;

	int num = 0;
	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (strcasecmp(sp->secName, secName) == 0) {
			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				if (kvp->inUse == 0)
					continue;
				num++;
			}
			break;
		}
	}
	return num;
}

/*
 * Returns number of max comments currently allowed.  Use iniIncreaseCommentSize() to change.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *
 *   returns max comments.
 */
int iniGetCommentMax(IniFile *ini) {
	return ini->maxComments;
}

/*
 * Returns number of max saved comments currently allowed.  Use iniIncreateSecCommentSize() to change.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *
 *   returns max comments.
 */
int iniGetSecCommentMax(IniFile *ini) {
	return ini->maxSecComments;
}

/*
 * Returns number of max sections currently allowed.  Use iniIncreateSetionSize() to change.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *
 *   returns max sections.
 */
int iniGetSectionMax(IniFile *ini) {
	return ini->maxSections;
}

/*
 * Returns number of max keys currently allowed.  Use iniIncreateKeySize() to change.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *
 *   returns max keys.
 */
int iniGetKeyMax(IniFile *ini) {
	return ini->maxKeys;
}

/*
 * Print to stdout the IniFile structure.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *
 *   returns -1 if iniIdx is out of range.
 *           0 on success.
 */
int iniPrint(IniFile *ini) {
	if (ini == NULL)
		return -1;

	Comment *cp = ini->comments;
	for (int x = 0; x < ini->maxComments; x++, cp++) {
		if (cp->inUse == 0)
			continue;
		if (strcmp(cp->secName, GLOBAL_NAME) == 0)
			printf("#%s\n", cp->comment);
	}
	printf("\n");

	Section *sp = ini->section;

	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;

		int cnt = 0;
		SecComment *scp = ini->secComments;
		for (int x = 0; x < ini->maxSecComments; x++, scp++) {
			if (scp->inUse == 0)
				continue;

			if (cnt == 0)
				printf("\n");

			cnt++;
			if (strcmp(scp->secName, sp->secName) == 0)
				printf(";%s\n", scp->comment);
		}

		if (sp->secComment[0] == '\0')
			printf("Section: %s\n", sp->secName);
		else
			printf("Section: %s\t\t;%s\n", sp->secName, sp->secComment);

		KV *kvp = sp->kv;
		for (int x = 0; x < ini->maxKeys; x++, kvp++) {
			if (kvp->inUse == 0)
				continue;
			if (kvp->kvComment[0] == '\0')
				printf("   Key: %s Value: %s\n", kvp->key, kvp->value);
			else
				printf("   Key: %s Value: %s\t\t;%s\n", kvp->key, kvp->value, kvp->kvComment);
		}

		KvComment *kvc = ini->_kvComments;
		for (int i = 0; i < MAX_KV_COMMENTS; i++, kvc++) {
			if (kvc->inUse == 0)
				continue;

			if (strcmp(kvc->secName, sp->secName) == 0)
				printf("\t;%s\n", kvc->comment);
		}

		printf("[end]\n");

//		Comment *cp = ini->comments;
//		for (int x = 0; x < ini->maxComments; x++, cp++) {
//			if (cp->inUse == 0)
//				continue;
//			if (strcmp(cp->secName, sp->secName) == 0)
//				printf(";%s\n", cp->comment);
//		}
	}

	return 0;
}

/*
 * Print to stdout a single section.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = Name of section to print.
 *
 *   returns -1 if iniIdx is out of range.
 *           0 on success.
 */
int iniPrintSection(IniFile *ini, const char *secName) {
	if (ini == NULL)
		return -1;

	Section *sp = ini->section;

	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;

		Comment *cp = ini->comments;
		for (int x = 0; x < ini->maxComments; x++, cp++) {
			if (cp->inUse == 0)
				continue;

			if (strcmp(cp->secName, sp->secName) == 0)
				printf(";%s\n", cp->comment);
		}
		printf("\n");

		if (strcasecmp(sp->secName, secName) == 0) {
			if (sp->secComment[0] == '\0')
				printf("Section: %s\n", sp->secName);
			else
				printf("Section: %s\t\t;%s\n", sp->secName, sp->secComment);

			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				if (kvp->inUse == 0)
					continue;

				if (kvp->kvComment[0] == '\0')
					printf("   Key: %s Value: %s\n", kvp->key, kvp->value);
				else
					printf("   Key: %s Value: %s\t\t;%s\n", kvp->key, kvp->value, kvp->kvComment);
			}

			KvComment *kvc = ini->_kvComments;
			for (int i = 0; i < MAX_KV_COMMENTS; i++, kvc++) {
				if (kvc->inUse == 0)
					continue;

				if (strcmp(kvc->secName, sp->secName) == 0)
					printf("\t;%s\n", kvc->comment);
			}

			printf("[end]\n");
		}
	}
	return 0;
}

/*
 * Print to stdout a single key/value pair.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name.
 *   key = key name to print.
 *
 *   returns -1 if iniIdx is out of range.
 *           0 on success.
 */
int iniPrintKeyValue(IniFile *ini, const char *secName, const char *key) {
	if (ini == NULL)
		return -1;

	Section *sp = ini->section;

	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;
		if (strcasecmp(sp->secName, secName) == 0) {
			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				if (kvp->inUse == 0)
					continue;
				if (strcasecmp(kvp->key, key) == 0) {
					if (kvp->kvComment[0] == '\0')
						printf("   Key: %s Value: %s\n", kvp->key, kvp->value);
					else
						printf("   Key: %s Value: %s\t\t;%s\n", kvp->key, kvp->value, kvp->kvComment);
				}
			}

			KvComment *kvc = ini->_kvComments;
			for (int i = 0; i < MAX_KV_COMMENTS; i++, kvc++) {
				if (kvc->inUse == 0)
					continue;

				if (strcmp(kvc->secName, sp->secName) == 0)
					printf("\t;%s\n", kvc->comment);
			}
		}
	}
	return 0;
}

/*
 * Returns a pointer to the comment of a key or NULL if not found.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name.
 *   key = key name to print.
 *
 *   returns null if iniIdx is out of range or key is not found.
 *           pointer to comment on success.
 */
char *iniGetValueComment(IniFile *ini, const char *secName, const char *key) {
	char *cp = NULL;

	if (ini == NULL)
		return NULL;

	pthread_mutex_lock(&lock);

	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;

		if (strcasecmp(sp->secName, secName) == 0) {
			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				if (kvp->inUse == 0)
					continue;
				if (strcasecmp(kvp->key, key) == 0) {
					cp = kvp->kvComment;
					break;
				}
			}
			if (cp != NULL)
				break;
		}
	}

	pthread_mutex_unlock(&lock);

	return cp;
}

/*
 * Returns a pointer to the value of a key/value pair or NULL if not found.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name.
 *   key = key name to print.
 *
 *   returns null if iniIdx is out of range or key is not found.
 *           pointer to value on success.
 */
char *iniGetValue(IniFile *ini, const char *secName, const char *key) {
	char *vp = NULL;

	if (ini == NULL)
		return NULL;

	pthread_mutex_lock(&lock);

	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;

		if (strcasecmp(sp->secName, secName) == 0) {
			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				if (kvp->inUse == 0)
					continue;
				if (strcasecmp(kvp->key, key) == 0) {
					vp = kvp->value;
					break;
				}
			}
			if (vp != NULL)
				break;
		}
	}

	pthread_mutex_unlock(&lock);

	return vp;
}

char *iniGetString(IniFile *ini, const char *secName, const char *key) {
	return iniGetValue(ini, secName, key);
}

int iniGetIntValue(IniFile *ini, const char *secName, const char *key) {
	char *s = iniGetValue(ini, secName, key);

	if (s == NULL)
		return 0;
	
	int len = strlen(s);

	for (int i = 0; i < len; i++) {
		if (isdigit(s[i]) == 0)
			return -1;
	}
	
	return strtol(s, NULL, 10);

}

int iniGetBooleanValue(IniFile *ini, const char *secName, const char *key) {
	int flag = 0;

	char *s = iniGetString(ini, secName, key);
	if (s == NULL)
		return flag;
	
	if (strcasecmp(s, "true") == 0 || strcasecmp(s, "yes") == 0 ||
			*s == 'Y' || *s == 'y' || *s == 'T' || *s == 't') {
		return 1;
	}

	return flag;
}

unsigned short iniGetShortValue(IniFile *ini, const char *secName, const char *key) {
	return (unsigned short)iniGetIntValue(ini, secName, key);
}

/*
 * Remove a section from the IniFile
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name to remove.
 *
 *   returns -1 if iniIdx is out of range.
 *           0 on success.
 */
int iniRemoveSection(IniFile *ini, const char *secName) {
	if (ini == NULL)
		return -1;

	pthread_mutex_lock(&lock);

	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;
		if (strcasecmp(sp->secName, secName) == 0) {
			sp->inUse = 0;
			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				kvp->inUse = 0;
			}
		}
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Remove key/value pair from a section
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name to remove.
 *   key = key name to remove.
 *
 *   returns -1 if iniIdx is out of range.
 *           0 on success.
 */
int iniRemoveKey(IniFile *ini, const char *secName, const char *key) {
	if (ini == NULL)
		return -1;

	pthread_mutex_lock(&lock);

	int found = 0;
	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;
		if (strcasecmp(sp->secName, secName) == 0) {
			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				if (strcasecmp(kvp->key, key) == 0) {
					kvp->inUse = 0;
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Free up a INiFile slot.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *
 *   returns -1 if iniIdx is out of range.
 *           0 on success
 */
int iniFree(IniFile *ini) {
	if (ini == NULL)
		return -1;

	Section *sp = ini->section;

	pthread_mutex_lock(&lock);

	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 1) {
			sp->inUse = 0;
			KV *kp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kp++) {
				if (kp->inUse == 1) {
					kp->inUse = 0;
				}
			}
		}
        if (sp->kv) {
            free(sp->kv);
        }
	}

    ini->inUse = 0;
    free(ini->section);
    free(ini->comments);
    free(ini->secComments);
    free(_savedComments);
    free(_kvComments);
	free(ini->fileName);
    free(ini);

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Walks the section array calling the function provided.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   funcPtr = function pointer to be called for each section in use.
 *
 *   returns -1 if iniIdx is out of range.
 *            count, number of sections in use.
 */
int iniWalkSections(IniFile *ini, void (*funcPtr)(IniFile *iniFile, const char *secName)) {
	if (ini == NULL)
		return -1;

	int count = 0;
	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;
		funcPtr(ini, sp->secName);
		count++;
	}

	return count;
}


/*
 * Walks the keys array for the section name given, calling the function provided.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   funcPtr = function pointer to be called for each section in use.
 *             Calls this function pointer with iniIdx, secName and keyName
 *
 *   returns -1 if iniIdx is out of range.
 *            count, number of keys in use.
 */
int iniWalkSectionKeys(IniFile *ini, const char *secName, void (*funcPtr)(IniFile *iniFile, const char *secName, const char *key)) {
	if (ini == NULL)
		return -1;

	int count = 0;
	Section *sp = ini->section;
	for (int i = 0; i < ini->maxSections; i++, sp++) {
		if (sp->inUse == 0)
			continue;
		if (strcasecmp(sp->secName, secName) == 0) {
			KV *kvp = sp->kv;
			for (int x = 0; x < ini->maxKeys; x++, kvp++) {
				if (kvp->inUse == 1) {
					funcPtr(ini, sp->secName, kvp->key);
					count++;
				}
			}
		}
	}

	return count;
}

/*
 * Add a key/value comment
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = name of section.
 *   key = Name of key.
 *   comment = Comment to add.
 *
 *   return -1 if ini is NULL
 *          -2 if key is too large
 *          0 on success
 */
int iniAddKeyValueComment(IniFile *ini, const char *secName, const char *key, const char *comment) {
	if (ini == NULL)
		return -1;
	if (strlen(key) > MAX_KEY_NAME)
		return -2;

	pthread_mutex_lock(&lock);

//	Section *sec = ini->section;
	Section *ps = ini->section;

	KV *foundKv = NULL;
	for (int i = 0; i < ini->maxSections; i++, ps++) {
		if (ps->inUse == 0)
			continue;
		if (ps->inUse == 1 && strcasecmp(ps->secName, secName) == 0) {
			// found matching section name
			KV *kp = ps->kv;
			for (int x = 0; x < ini->maxKeys; x++, kp++) {
				if (kp->inUse == 0)
					continue;
				if (kp->inUse == 1 && strcasecmp(kp->key, key) == 0) {
					// found matching key anem
					foundKv = kp;
					break;
				}
			}
			break;
		}
	}

	if (foundKv != NULL) {
		if (comment != NULL)
			strcpy(foundKv->kvComment, comment);
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Adds a key/value pair to a section if a key slot is available.  Section must already exist.
 * If the key already exists then the value is replaced.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name to look for.
 *   key = key name to be added.
 *   value = value to assign to key.
 *
 *   returns -1 if iniIdx is out of range.
 *           -2 if key name length too long.
 *           -3 if value length is too long.
 *           0 on success
 */
int iniAddKeyValue(IniFile *ini, const char *secName, const char *key, const char *value) {
	return _iniAddKeyValueWithComment(ini, secName, key, value, NULL);
}

/*
 * Adds a key/value pair to a section if a key slot is available.  Section must already exist.
 * If the key already exists then the value is replaced.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   secName = section name to look for.
 *   key = key name to be added.
 *   value = value to assign to key.
 *   comment = comment for key/value pair.
 *
 *   returns -1 if iniIdx is out of range.
 *           -2 if key name length too long.
 *           -3 if value length is too long.
 *           -4 if section does not exist.
 *           0 on success
 */
int _iniAddKeyValueWithComment(IniFile *ini, const char *secName, const char *key, const char *value, const char *comment) {
	if (ini == NULL)
		return -1;
	if (strlen(key) > MAX_KEY_NAME)
		return -2;
	if (strlen(value) > MAX_VALUE_SIZE)
		return -3;

	pthread_mutex_lock(&lock);

	Section *ps = ini->section;

	KV *foundKv = NULL;
	KV *newKv = NULL;
	int foundSec = 0;

	for (int i = 0; i < ini->maxSections; i++, ps++) {
		if (ps->inUse == 0)
			continue;

		if (ps->inUse == 1 && strcasecmp(ps->secName, secName) == 0) {
			// found matching section name
			foundSec = 1;
			KV *kp = ps->kv;
			for (int x = 0; x < ini->maxKeys; x++, kp++) {
				if (kp->inUse == 1 && strcasecmp(kp->key, key) == 0) {
					// found matching key name
					foundKv = kp;
					break;
				} else if (kp->inUse == 0 && newKv == NULL) {
					newKv = kp;		// find first free key slot.
				}
			}
			break;
		}
	}

	if (foundSec == 0) {
		pthread_mutex_unlock(&lock);
		return -4;
	} else if (foundKv != NULL) {
		strcpy(foundKv->key, key);
		strcpy(foundKv->value, value);
		if (comment != NULL)
			strcpy(foundKv->kvComment, comment);
	} else if (newKv != NULL) {
		newKv->inUse = 1;
		strcpy(newKv->key, key);
		strcpy(newKv->value, value);
		if (comment != NULL)
			strcpy(newKv->kvComment, comment);
	} else {
		pthread_mutex_unlock(&lock);
		// Out of key slots increase the size and try again.
		iniIncreaseKeySize(ini, iniGetKeyMax(ini) + MAX_KEYS);
		return iniAddKeyValue(ini, secName, key, value);
	}

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Write the IniFile to the buf given.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   buf = buffer to write to
 *
 *   returns -1 if iniIdx is out of range.
 *           -2 if both filename and ini->filename are NULL
 *           or number of bytes written to buf on success
 */
int iniWriteBuf(IniFile *ini, char *buf) {
	if (ini == NULL)
		return -1;
	if (buf == NULL)
		return -2;

    int wBytes = 0;

	pthread_mutex_lock(&lock);

	char tsStr[80];
	struct tm ts;
	time_t t = time(NULL);
	ts = *localtime(&t);
	strftime(tsStr, sizeof(tsStr), "# Last modified by program: %Y-%m-%d %H:%M:%S", &ts);

	wBytes += sprintf(&buf[wBytes], "%s\n", tsStr);

	Comment *cp = ini->comments;
	for (int x = 0; x < ini->maxComments; x++, cp++) {
		if (cp->inUse == 0)
			continue;

		if (strcmp(cp->secName, GLOBAL_NAME) == 0) {
			if (strstr(cp->comment, "Last modified") == NULL)
				wBytes += sprintf(&buf[wBytes], "#%s\n", cp->comment);
		}
	}
	wBytes += sprintf(&buf[wBytes], "\n");

	Section *sec = ini->section;

	for (int i = 0; i < ini->maxSections; i++, sec++) {
		if (sec->inUse == 0)
			continue;

		int cnt = 0;
		SecComment *scp = ini->secComments;
		for (int x = 0; x < ini->maxComments; x++, scp++) {
			if (scp->inUse == 0)
				continue;

			if (cnt == 0)
				wBytes += sprintf(&buf[wBytes], "\n");

			cnt++;
			if (strcmp(scp->secName, sec->secName) == 0)
				wBytes += sprintf(&buf[wBytes], ";%s\n", scp->comment);
		}
//		wBytes += sprintf(&buf[wBytes], "\n");

		if (sec->secComment[0] == '\0')
			wBytes += sprintf(&buf[wBytes], "[%s]\n", sec->secName);
		else
			wBytes += sprintf(&buf[wBytes], "[%s]\t\t;%s\n", sec->secName, sec->secComment);

		KV *kp = sec->kv;
		for (int x = 0; x < ini->maxKeys; x++, kp++) {
			if (kp->inUse == 0)
				continue;
			if (kp->kvComment[0] == '\0')
				wBytes += sprintf(&buf[wBytes], "\t%s = %s\n", kp->key, kp->value);
			else
				wBytes += sprintf(&buf[wBytes], "\t%s = %s\t\t;%s\n", kp->key, kp->value, kp->kvComment);
        }

		KvComment *kvp = ini->_kvComments;
		for (int i = 0; i < MAX_KV_COMMENTS; i++, kvp++) {
			if (kvp->inUse == 0)
				continue;

			if (strcmp(kvp->secName, sec->secName) == 0)
				wBytes += sprintf(&buf[wBytes], "\t;%s\n", kvp->comment);
		}

		wBytes += sprintf(&buf[wBytes], "[end]\n");
	}

//	printf("ini: '%s'\n", buf);

	pthread_mutex_unlock(&lock);

	return wBytes;
}

/*
 * Write the IniFile to the filename given.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   filename = If NULL then use ini->filename, which overwrites existing file.
 *              If not NULL then create a new file using past in filename.
 *              if both filename and ini->filename are NULL the do nothing.
 *
 *   returns -1 if iniIdx is out of range.
 *           -2 if both filename and ini->filename are NULL
 *           -3 if could not fopen file.
 *           0 on success
 */
int iniWrite(IniFile *ini, const char *filename) {
	if (ini == NULL)
		return -1;

	Section *sec = ini->section;

	if (filename == NULL && ini->fileName[0] == '\0')
		return -2;

	pthread_mutex_lock(&lock);

//	if (ini->fileName != NULL)
//		free(ini->fileName);

//	ini->fileName = (char *)calloc(1, (strlen(filename) + 1));
	strcpy(ini->fileName, filename);

	struct stat buf;
	errno = 0;

	if (stat(ini->fileName, &buf) == 0) {
		if (access(ini->fileName, F_OK | W_OK) == -1) {
			return -4;
		}
	}

	char tsStr[80];
	struct tm ts;
	time_t t = time(NULL);
	ts = *localtime(&t);
	strftime(tsStr, sizeof(tsStr), "# Last modified by program: %Y-%m-%d %H:%M:%S", &ts);

	FILE *fp = fopen(ini->fileName, "w");

	if (fp == NULL)
		return -3;

	fprintf(fp, "%s\n", tsStr);

	Comment *cp = ini->comments;
	for (int x = 0; x < ini->maxComments; x++, cp++) {
		if (cp->inUse == 0)
			continue;
		if (strcmp(cp->secName, GLOBAL_NAME) == 0) {
			if (strstr(cp->comment, "Last modified") == NULL)
				fprintf(fp, "#%s\n", cp->comment);
		}
	}
	fprintf(fp, "\n");

	for (int i = 0; i < ini->maxSections; i++, sec++) {
		if (sec->inUse == 0)
			continue;

		int cnt = 0;
		SecComment *scp = ini->secComments;
		for (int x = 0; x < ini->maxSecComments; x++, scp++) {
			if (scp->inUse == 0)
				continue;

			if (cnt == 0)
				fprintf(fp, "\n");

			cnt++;
			if (strcmp(scp->secName, sec->secName) == 0)
				fprintf(fp, ";%s\n", scp->comment);
		}

		if (sec->secComment[0] == '\0')
			fprintf(fp, "[%s]\n", sec->secName);
		else
			fprintf(fp, "[%s]\t\t;%s\n", sec->secName, sec->secComment);

		KV *kp = sec->kv;
		for (int x = 0; x < ini->maxKeys; x++, kp++) {;

			if (kp->inUse == 0)
				continue;

			if (kp->kvComment[0] == '\0')
				fprintf(fp, "\t%s = %s\n", kp->key, kp->value);
			else
				fprintf(fp, "\t%s = %s\t\t;%s\n", kp->key, kp->value, kp->kvComment);
		}

		KvComment *kvp = ini->_kvComments;
		for (int i = 0; i < MAX_KV_COMMENTS; i++, kvp++) {
			if (kvp->inUse == 0)
				continue;

			if (strcmp(kvp->secName, sec->secName) == 0)
				fprintf(fp, "\t;%s\n", kvp->comment);
		}

		fprintf(fp, "[end]\n");
	}

	fclose(fp);

	pthread_mutex_unlock(&lock);

	return 0;
}

/*
 * Increase the number of global comments allowed.
 * This effects all IniFiles.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   maxSec = new max section size.
 *
 *   return -1 if new section size is less than or equal to current size;
 *          0 on success
 */
int iniIncreaseCommentSize(IniFile *ini, int maxComment) {
	printf("Enter iniIcreaseCommentSize.\n");
	if (ini->maxComments < maxComment) {
		pthread_mutex_lock(&lock);

		Comment *cp = ini->comments;
		Comment *newComment = (Comment *)calloc(maxComment, sizeof(Comment));	// allocate new size.
		memcpy(newComment, cp, (sizeof(Comment) * ini->maxComments));		// copy old to new

		free(ini->comments);		// free old section array.
		ini->comments = newComment;	// set new section array.

		ini->maxComments = maxComment;

		pthread_mutex_unlock(&lock);

		return 0;
	}

	return -1;
}

/*
 * Increase the number of saved comments allowed.
 * This effects all IniFiles.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   maxSec = new max section size.
 *
 *   return -1 if new section size is less than or equal to current size;
 *          0 on success
 */
int iniIncreaseSecCommentSize(IniFile *ini, int maxSecComment) {
	printf("Enter iniIcreaseSecCommentSize.\n");
	if (ini->maxSecComments < maxSecComment) {
		pthread_mutex_lock(&lock);

		SecComment *scp = ini->secComments;
		SecComment *newComment = (SecComment *)calloc(maxSecComment, sizeof(SecComment));	// allocate new size.
		memcpy(newComment, scp, (sizeof(SecComment) * ini->maxSecComments));		// copy old to new

		free(ini->secComments);		// free old section array.
		ini->secComments = newComment;	// set new section array.

		ini->maxSecComments = maxSecComment;

		pthread_mutex_unlock(&lock);

		return 0;
	}

	return -1;
}

/*
 * Increase the number of sections allowed.
 * This effects all IniFiles.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   maxSec = new max section size.
 *
 *   return -1 if new section size is less than or equal to current size;
 *          0 on success
 */
int iniIncreaseSectionSize(IniFile *ini, int maxSec) {
	printf("Enter iniIcreaseSectionSize.\n");
	if (ini->maxSections < maxSec) {
		pthread_mutex_lock(&lock);

		Section *sp = ini->section;
		Section *newSec = (Section *)calloc(maxSec, sizeof(Section));	// allocate new size.
		memcpy(newSec, sp, (sizeof(Section) * ini->maxSections));		// copy old to new

		sp = (newSec + ini->maxSections);							// move past old size.
		for (int x = ini->maxSections; x < maxSec; x++, sp++) {
			sp->kv = (KV *)calloc(ini->maxKeys, sizeof(KV));			// allocate key/value array for new sections.
		}

		free(ini->section);		// free old section array.
		ini->section = newSec;	// set new section array.

		ini->maxSections = maxSec;

		pthread_mutex_unlock(&lock);

		return 0;
	}

	return -1;
}

/*
 * Increase the number of keys per section allowed.
 * This effects all IniFiles and all sections.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   maxKeySize = new max key size.
 *
 *   return -1 if new key size is less than or equal to current size;
 *          0 on success
 */
int iniIncreaseKeySize(IniFile *ini, int maxKeySize) {
//	printf("ini->maxKeys=%d, maxKeySize=%d\n", ini->maxKeys, maxKeySize);
	if (ini->maxKeys < maxKeySize) {
		pthread_mutex_lock(&lock);

		Section *sp = ini->section;

		for (int k = 0; k < ini->maxSections; k++, sp++) {
			KV *kp = sp->kv;									// save old memory address
			KV *newKp = (KV *)calloc(maxKeySize, sizeof(KV));		// allocate new size
			memcpy(newKp, kp, (sizeof(KV) * ini->maxKeys));			// copy old to new

			free(sp->kv);		// free old memory.
			sp->kv = newKp;		// set new memory.
		}

		ini->maxKeys = maxKeySize;

		pthread_mutex_unlock(&lock);

		return 0;
	}

	return -1;
}
/*
 * Write the IniFile with the given section to the buf given.
 *
 *   ini = pointer to IniFile returned from the iniCreate().
 *   buf = buffer to write to
 *
 *   returns -1 if iniIdx is out of range.
 *           -2 if both filename and ini->filename are NULL
 *           or number of bytes written to buf on success
 */
int iniSecWriteBuf(IniFile *ini, const char *secName, char *buf) {
	if (ini == NULL)
		return -1;
	if (buf == NULL)
		return -2;

	Section *sec = ini->section;
	char line[MAX_LINE_SIZE];
    int wBytes = 0;

	pthread_mutex_lock(&lock);

	for (int i = 0; i < ini->maxSections; i++, sec++) {
		if (sec->inUse == 0)
			continue;
        if (strcmp(secName, sec->secName) == 0) {
		    wBytes += sprintf(line, "[%s]\n", sec->secName);
		    if (sec->secComment[0] == '\0')
		    	wBytes += sprintf(line, "[%s]\n", sec->secName);
			else
		    	wBytes += sprintf(line, "[%s]\t\t;%s\n", sec->secName, sec->secComment);
		    strcat(buf, line);
            wBytes += 1;
		    KV *kp = sec->kv;
		    for (int x = 0; x < ini->maxKeys; x++, kp++) {
			    if (kp->inUse == 0)
				    continue;
			    wBytes += sprintf(line, "\t%s = %s\n", kp->key, kp->value);
			    strcat(buf, line);
                wBytes += 1;
            }
		    strcat(buf, "\n");
            wBytes += 2;
       }
	}

//	printf("ini: '%s'\n", buf);

	pthread_mutex_unlock(&lock);

	return wBytes;
}


