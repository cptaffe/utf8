
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utf8.h"

const char UTF8_BOM[] = {0xef, 0xbb, 0xbf};

utf8_Parser *utf8_Parser_init(const char *str) {
	utf8_Parser *parser = malloc(sizeof(utf8_Parser));
	memset(parser, 0, sizeof(utf8_Parser));
	parser->str = str;
	parser->len = strlen(str);
	return parser;
}

void utf8_Parser_free(utf8_Parser *parser) {
	free(parser);
}

// returns the length of the current token.
int utf8_RuneLen(const char *str) {
	// 4 are in utf-8, 6 was the first spec.
	const int max_bytes = 4; // arbitrary maximum, 0 <= max_bytes <= 8

	// build mask
	uint8_t mask = 0;
	int bytelength = 0;

	for (int i = max_bytes - 1; i >= 0; i--) {
		mask += 1 << (i + (8 - max_bytes)); // eight is bits in a byte.
		if ((str[0] & mask) != mask) {
			// find maximum bits set before zero bit is found.
			break;
		}
		bytelength++;
	}

	return bytelength;
}

// checks for bom, if found, increments over it.
bool utf8_BomCheck(const char *str) {
	// length safety
	if (strlen(str) >= sizeof(UTF8_BOM)) {
		if (strncmp(str, UTF8_BOM, sizeof(UTF8_BOM))) {
			return true;
		}
	}
	return false;
}

// returns codepoint value of utf-8 character


// checks byte for validity.
bool utf8_ValidRune(const uint32_t cp) {
	// TODO: implement validation tests.
	return true;
}

int utf8_RuneCount(const char *str) {
	uint32_t i;
	utf8_Parser *parser = utf8_Parser_init(str);
	for (i = 0; !utf8_Parser_Get(parser, NULL); i++) {}
	utf8_Parser_free(parser);
	return i;
}


// return values:
// zero indicates success.
// one indicates eof.
// negative numbers are errors.
int utf8_Parser_Get(utf8_Parser *parser, uint32_t *codepoint) {
	uint32_t *cp = malloc(sizeof(uint32_t));
	const uint8_t *byte = (uint8_t *) &parser->str[parser->index];

	// get byte length of token.
	const int bl = utf8_RuneLen((char *) byte);

	if (bl == 0) {
		// standard ascii value
		if (byte[0] == 0) {
			return 1; // eof, error
		} else {
			memcpy(cp, byte, 1);
			parser->index++;
		}
	} else if (bl == 1) {
		// unsupported character
		return 1;
	} else {
		// multi-byte unicode value
		// check for zeroed 2nd bit..
		uint8_t msk_xor = (1 << 6);
		uint8_t msk_and = (1 << 7);
		for (int i = 1; i < bl; i++) {
			// TODO: testing!
			if ((byte[i] ^ msk_xor) == byte[i]
		&& (byte[i] & msk_and) != msk_and) {
				return -1; // error
			}
		}
		memcpy(cp, byte, bl);
		parser->index += bl;
	}

	if (utf8_ValidRune(*cp)) {
		if (codepoint != NULL) {
			*codepoint = *cp; // allow nulls.
		}
		return 0;
	} else {
		return -2; // error
	}
}
