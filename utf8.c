
// Copyright (c) 2015 Connor Taffe

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utf8.h"

// utf-8 byte order mark (bom).
const char _bom[] = {0xef, 0xbb, 0xbf};
const utf8_str UTF8_BOM = {
	.str = (char *) _bom,
	.len = sizeof(_bom)
};

// get Rune from file.
utf8_rune utf8_fgetr(FILE *file) {
	if (!file) {
		return 0; // error
	}

	utf8_rune c = getc(file);
	if (c == EOF) {
		return 0;
	}
	int len = utf8_runelen(c);
	if (len < 0) {
		return 0;
	}
	for (int i = 1; i < len; i++) {
		char j = getc(file);
		if (j == EOF) {
			return 0;
		}
		((uint8_t *) &c)[i] = j;
	}

	// check for validity.
	if (utf8_valid(c)) {
		return c;
	} else {
		return 0;
	}
}

// returns null on failure
utf8_parser *utf8_pinit(const utf8_str *str) {
	// assertions
	if (!str) {
		return NULL;
	}

	utf8_parser *p = malloc(sizeof(utf8_parser));
	memset(p, 0, sizeof(utf8_parser));
	p->str = str;
	return p;
}

void utf8_pfree(utf8_parser *parser) {
	free(parser);
}

utf8_str *utf8_strlinit(const char *str, size_t len) {
	// assertions
	if (!str) {
		return NULL;
	}

	// alloc string
	utf8_str *string = malloc(sizeof(utf8_str));
	if (!string) {
		return NULL;
	}

	string->str = str;
	string->len = len;
	return string;
}

utf8_str *utf8_strinit(const char *str) {
	return utf8_strlinit(str, strlen((const char *) str));
}

void utf8_strfree(utf8_str *str) {
	free(str);
}

// compare two strings (not length dependent)
int utf8_strcmp(const utf8_str *a, const utf8_str *b) {
	// assertions
	assert(a && b);

	size_t len;
	if (a->len >= b->len) {
		len = a->len;
	} else {
		len = b->len;
	}

	return strncmp((const char *) a->str, (const char *) b->str, len);
}

// returns the length of the current token.
int utf8_runelens(const utf8_str *str) {
	// assertions
	if (!(str && str->str)) {
		return -1;
	}

	utf8_rune r = 0;
	// copy the shortest length (string or rune).
	void *ret = memcpy(&r, str->str,
		(str->len > sizeof(utf8_rune)) ? sizeof(utf8_rune) : str->len);
	if (!ret) {
		return -1;
	}

	// return len from rune.
	return utf8_runelen((const utf8_rune) r);
}

int utf8_runelen(const utf8_rune r) {
	// 4 are in utf-8, 6 was the first spec.
	const int max_bytes = 4; // arbitrary maximum, 0 <= max_bytes <= 8

	// build mask
	uint8_t mask = 0;
	int bytelength = 0;
	uint8_t *rune = (uint8_t *) &r;

	for (int i = max_bytes - 1; i >= 0; i--) {
		mask |= 1 << (i + (8 - max_bytes)); // eight is bits in a byte.
		if ((rune[0] & mask) != mask) {
			// find maximum bits set before zero bit is found.
			break;
		}
		bytelength++;
	}

	return bytelength;
}

// checks for bom, if found, increments over it.
bool utf8_strchkbom(const utf8_str *str) {
	// length safety
	if (str->len >= sizeof(UTF8_BOM)) {
		if (utf8_strcmp(str, &UTF8_BOM)) {
			return true;
		}
	}
	return false;
}

// returns codepoint value of utf-8 character
int32_t utf8_decode(const utf8_rune rune) {
	int32_t cp = 0;
	// get number of bits.
	int bl = utf8_runelen(rune);
	if (bl < 0) {
		return -1;
	} else if (bl == 0) {
		bl = 1;
	}

	// forge mask
	utf8_rune mask = 0;
	for (int i = 0; i < (7 - bl); i++) {
		mask += (1 << i);
	}

	cp = rune & mask;
	// byte order is backwards on this machine (endianness).
	// shift first bits by the the number of bits in each
	// subsequent byte (6) once for every remaining byte (bl)
	cp <<= (bl - 1) * 6;
	mask = 0x3f;
	for (int i = 1; i < bl; i++) {
		// for each byte after the 1st, do:
		uint32_t c = ((uint8_t *) &rune)[i] & mask;
		// or with cp to combine bits,
		// c is shifted by (bl - i) which means
		// lower bits have higher values, multiply this
		// by six (number of bits per byte), and that will
		// give the shift needed to place lower bytes near
		// the first byte and provide the inverse ordering needed
		// to get a correct number.
		cp |= (c << (((bl - i) - 1) * 6));
	}

	return cp;
}

// returns encoded rune from codepoint
utf8_rune utf8_encode(const uint32_t cp) {
	utf8_rune rune = 0;
	// get number of bytes
	int bl = 0;
	// limits as indicated in RFC 3629.
	if (cp < 0x80) {
		bl = 1;
	} else if (cp >= 0x80 && cp < 0x800) {
		bl = 2;
	} else if (cp >= 0x800 && cp < 0xffff) {
		bl = 3;
	} else if (cp >= 0xffff && cp <= 0x10ffff) {
		bl = 4;
	} else {
		// not an encodeable character value
		return -1; // error
	}

	if (bl == 1) {
		rune = (utf8_rune) cp; // no special transform
	} else {
		uint32_t mask = 0;
		uint32_t code = 0;

		// here we generate the rune heading.
		for (int i = 0; i < bl; i++) {
			code |= (1 << (7 - i));
		}

		// here we generate the byte headings.
		for (int i = 0; i < bl; i++) {
			// shift by bits in a byte (8) times one plus
			// the current byte offset, minus one.
			code |= (1 << ((8 * (i + 1)) - 1));
		}

		// construct mask for the first encoded byte,
		// the bytes fit after the header, which is n 1 bits and
		// a 0 bit, so 7 - bl will give us 8 - bl - 1, which is
		// a byte's bit content, minus the header 1 bits, minus a
		// zero bit.
		for (int i = 0; i < (7 - bl); i++) {
			mask |= (1 << i);
		}
		// the bits we want for the first byte's content are located
		// pas the bits we will use in subsequent bytes, since there
		// are 6 bits (sans 10 header) in each non-front byte, we will
		// skip the numby of bytes times the number of bits (6) for each
		// I subtract one from bl here so it shifts for n-1 bytes.
		mask <<= (bl - 1) * 6;
		// cp is anded with the mask we created to catch just the bits
		// in thte mask's range.
		uint32_t c = (mask & cp);
		// here we shift back, undoing the mask's shift to put the bits
		// back in the front of the number.
		c >>= ((bl - 1) * 6);
		code |= c; // or code with c.

		// now we iterate over the next bytes and mask their bits.
		for (int i = 1; i < bl; i++) {
			// 6 bit mask for each byte.
			mask = 0x3f;
			// we shift the mask by the inverse of our counter
			// (we order the last parst to the beginning), and
			// subtract one because we only do this for bytes
			// after the 1st. Then we multiply by six as six
			// is the number of bits in a byte sans header.
			mask <<= (((bl - i) - 1) * 6);
			// we then capture the bits with an and.
			uint32_t c = cp & mask;
			// and reverse the shift.
			c >>= (((bl - i) - 1) * 6);
			// starting from here, we can position it properly
			c <<= (i * 8);
			code |= c;
			rune = code;
		}
	}
	return rune;
}


// checks byte for validity.
bool utf8_valid(const utf8_rune rune) {
	// TODO: implement validation tests.

	// RFC 3629 mandates the the first byte indicate
	// number of following bytes.
	int len = utf8_runelen(rune);
	if (len < 0) {
		return false;
	} else {
		if (len > 0) {
			for (int i = 0; i < len; i++) {
				uint8_t octet = ((int8_t  *) &rune)[i];

				// RFC 3629 mandates the octets C0, C1, F5 to FF
				// never appear. Furthermore, every byte is not
				// zero except for ascii-compatible starting bytes,
				// which these octets are not.
				if (octet == 0xc0 || octet == 0xc1
					|| (octet >= 0xf5 && octet <= 0xff) || octet == 0) {
					return false; // invalid octet
				}
			}
		}
		return true;
	}
}

// negative values are errors
int utf8_strlen(const utf8_str *str) {
	// assertions
	if (!str || !str->str) {
		return -1; // error
	}

	// assert init was successful
	utf8_parser *parser = utf8_pinit(str);
	assert(parser != NULL);

	uint32_t i;
	for (i = 0; utf8_pget(parser) >= 0; i++);
	utf8_pfree(parser);
	return i;
}

utf8_rune utf8_readrune(const utf8_str *str, size_t len) {
	// assertions
	assert(str && str->str);
	assert(len <= sizeof(utf8_rune));

	utf8_rune cp;
	memcpy(&cp, str->str, len);
	return cp;
}

utf8_str *utf8_pgets(utf8_parser *parser) {
	// assertions
	if (!parser || !parser->str || !parser->str->str) {
		return NULL; // error
	}

	return utf8_strlinit(&parser->str->str[parser->index], parser->str->len - parser->index);
}

// return values:
// zero indicates success.
// one indicates eof.
// negative numbers are errors.
utf8_rune utf8_pget(utf8_parser *parser) {
	// assertions
	if (!parser || !parser->str || !parser->str->str) {
		return -1; // error
	}

	utf8_rune cp;
	const utf8_str *str = utf8_pgets(parser);
	if (!str) {
		return -1;
	}

	// get byte length of token.
	const int bl = utf8_strlen(str);
	if (bl < 0 || bl > str->len) {
		return bl; // error
	}

	if (bl == 0) {
		// standard ascii value
		if (str->str[0] == 0) {
			return 1; // eof, error
		} else {
			cp = utf8_readrune(str, 1);
			parser->index++;
		}
	} else if (bl == 1) {
		// unsupported character
		return 1;
	} else {
		// multi-byte unicode value
		// check for first two bits are 10
		uint8_t mask = (1 << 6) + (1 << 7);
		for (int i = 1; i < bl; i++) {
			// TODO: testing!
			if ((str->str[i] & mask) != (1 << 7)) {
				return -1; // error
			}
		}
		cp = utf8_readrune(str, bl);
		parser->index += bl;
	}

	if (utf8_valid(cp)) {
		return cp;
	} else {
		return -2; // error
	}
}
