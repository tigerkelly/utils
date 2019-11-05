
#include <stdio.h>
#include <string.h>

void replaceChar(char *str, char chr, char replace) {
    for (int z = 0; z < strlen(str); z++) {
        if (str[z] == chr)
            str[z] = replace;
    }
}

void replacenChar(char *str, char chr, char replace, int len) {
    for (int z = 0; z < len; z++) {
        if (str[z] == chr)
            str[z] = replace;
    }
}

// toStr must be long enough to hold result
// escape a single quote mark (') for postgresql inserts.
char *escapeQuote(char *str, char *toStr) {
	int x = 0;

	toStr[0] = '\0';
	for (int z = 0; z < strlen(str); z++) {
		if (str[z] == '\'')
			toStr[x++] = '\'';

		toStr[x++] = str[z];
		toStr[x] = '\0';		// null terminate as you go.
	}

	return toStr;
}
