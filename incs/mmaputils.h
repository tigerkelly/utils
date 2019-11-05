/*
 * mmaputils.h
 *
 * Description: Utilities for mmap
 *  Created on: July 4, 2019
 *      Author: Kelly Wiles
 *   Copyright: Kelly Wiles
 */

#ifndef INCS_MMAPUTILS_H_
#define INCS_MMAPUTILS_H_

// On a 64bit system pointers are 8 bytes in length.
typedef enum {
	CHAR_FIELD,			// uint8_t type field
	BOOL_FIELD,			// uint8_t type field
	SHORT_FIELD,		// uint16_t type field
	INT_FIELD,			// uint32_t type field
	LONG_FIELD,			// uint64_t type field
	ARRAY_FIELD,		// uint8_t array type field
	POINTER_FIELD,		// void pointer type field
} fieldType_t;

typedef struct _mmapField {
	char *fieldName;
	fieldType_t fldType;
	uint32_t fldLength;
} MmapField;

void *mmapInit(char *fileName, unsigned int sizeInPages);
int mmapLock();
int mmapUnlock();


#endif /* INCS_MMAPUTILS_H_ */
