
// Copyright (c) 2015 Connor Taffe

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utf8.h"

const int utf8_CODEPOINT_MAX = 0x10ffff;

// utf-8 byte order mark (bom).
const char UTF8_BOM[] = {0xef, 0xbb, 0xbf, 0x0};

// get Rune from file.
utf8_rune utf8_fget(FILE *file) {
	if (!file) {
		return utf8_RUNE_ERROR; // error
	}

	utf8_rune c = fgetc(file);
	if (c == EOF) {
		return 0;
	}
	int len = utf8_runelen(c);
	if (len < 0) {
		return 0;
	}
	for (int i = 1; i < len; i++) {
		char j = fgetc(file);
		if (j == EOF) {
			return 0;
		}
		((uint8_t *) &c)[i] = j;
	}

	// check for validity.
	if (utf8_isvalid(c)) {
		return c;
	} else {
		return utf8_RUNE_INVALID;
	}
}

int utf8_funget(FILE *file, const utf8_rune r) {
	if (!file) {
		return 1; // error
	} else if (r < 0 || r > utf8_CODEPOINT_MAX) {
		return 2; // error
	}

	for (int i = 3; i >= 0; i--) {
		int ret = ungetc(((uint8_t *) &r)[i], file);
		if (ret == EOF) {
			return 3; // error
		}
	}
	return 0;
}

// compare two strings (not length dependent)
int utf8_strcmp(const char *a, const char *b) {
	// assertions
	assert(a && b);

	size_t alen, blen, len;
	if ((alen = strlen(a)) >= (blen = strlen(b))) {
		len = alen;
	} else {
		len = blen;
	}

	return strncmp(a, b, len);
}

// returns the length of the current token.
int utf8_runelens(const char *str) {
	// assertions
	if (!str) {
		return -1;
	}

	size_t len = strlen(str);
	if (len < 4) {
		// str is too short to be used as a rune.
		utf8_rune r = 0;
		// copy the shortest length (string or rune).
		void *ret = memcpy(&r, str,
			(len > sizeof(utf8_rune)) ? sizeof(utf8_rune) : len);
		if (!ret) {
			return -1;
		}
		// return len from rune.
		return utf8_runelen((const utf8_rune) r);
	} else {
		// return len from rune.
		return utf8_runelen(*((utf8_rune *) str));
	}
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

	if (bytelength == 1) {
		return -1; // error, invalid rune header.
	}
	return bytelength;
}

// checks for bom, if found, increments over it.
bool utf8_strchkbom(const char *str) {
	// length safety
	if (strlen(str) >= sizeof(UTF8_BOM)) {
		if (utf8_strcmp(str, UTF8_BOM)) {
			return true;
		}
	}
	return false;
}

// used for finding start rune
bool utf8_isstartbyte(const uint8_t rune) {
	// first bit is 0 or first two bits are 11.
	if ((rune & 0xc0) == 0xc0 || (rune & 0x80) == 0) {
		return true;
	} else {
		return false;
	}
}

// returns codepoint value of utf-8 character
int32_t utf8_decode(const utf8_rune rune) {
	// do not decode error runes
	if (rune < 0 && !utf8_isvalid(rune)) {
		return utf8_RUNE_INVALID;
	}

	int32_t cp = 0;
	// get number of bits.
	int bl = utf8_runelen(rune);
	if (bl < 0) {
		// out of bounds
		return utf8_RUNE_INVALID;
	} else if (bl == 0) {
		return rune; // rune is code-point.
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
utf8_rune utf8_encode(const int32_t cp) {

	utf8_rune rune = 0;
	// get number of bytes
	int bl = 0;
	// limits as indicated in RFC 3629.
	if (cp >= 0 && cp < 0x80) {
		bl = 1;
	} else if (cp >= 0x80 && cp < 0x800) {
		bl = 2;
	} else if (cp >= 0x800 && cp < 0xffff) {
		bl = 3;
	} else if (cp >= 0xffff && cp <= 0x10ffff) {
		bl = 4;
	} else {
		// not an encodeable character value
		return utf8_RUNE_INVALID; // error
	}

	if (bl == 1) {
		rune = (utf8_rune) cp; // no special transform
	} else {
		uint32_t mask = 0;
		uint32_t code = 0;

		/* generate the rune heading.
			|              8-bits              |
			<byte-length 1-bits><0-bit><payload>
		*/
		for (int i = 0; i < bl; i++) {
			code |= (1 << (7 - i));
		}

		/* generate the byte headings.
			10xxxxxx, where x is payload (code-point) bits.
			^ this bit is the important one.

			We skip 8 * offset, which gets us to the beginning
			of the wanted byte, and then add 7, to get us to the
			index of the last bit of that byte.
		*/
		for (int i = 0; i < bl; i++) {
			code |= (1 << ((8 * i) + 7));
		}

		int offset = bl - 1;

		/* masks first byte
			<length of encoding>0<code-point bits>
			where the mask would be 0 for <length of encoding>
			and 0, but 1 for <code-point bits>.
			Then shift past the bits used in subsequent bytes.
			Each subsequent byte contains a 2-bit header (10) and
			a 6-bit code-point payload, so we shift the offset * 6:
				<header-bits><offset # of 6-bit groups>
				     ^<----- (individual bit offset)
			                <------------------------- (6-bit groups offset)
		*/
		for (int i = 0; i < (7 - bl); i++) {
			mask |= (1 << i) << (offset * 6);
		}

		// or mask, after being shifted back down.
		code |= (mask & cp) >> (offset * 6);

		// now we iterate over the next bytes and mask their bits.
		for (int i = 1; i < bl; i++) {
			// 6 bit mask for each byte, 0x3f = 0b00111111.
			// we shift the mask by the inverse of our counter
			// (we order the last parts to the beginning), and
			// subtract one because we only do this for bytes
			// after the 1st. Then we multiply by six as six
			// is the number of bits in a byte sans header.
			code |= ((cp & (0x3f << (((bl - i) - 1) * 6)))
				// and reverse the shift, then shift back up i bytes.
				>> (6 * ((bl - i) - 1))) << (8 * i);
			rune = code;
		}
	}
	return rune;
}


// checks byte for validity.
bool utf8_isvalid(const utf8_rune rune) {
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
				// never appear.
				if (octet == 0xc0 || octet == 0xc1
					|| (octet >= 0xf5 && octet <= 0xff)) {
					return false; // invalid octet
				} else if (i > 0 && ((octet & 0xc0) >> 6) != 2) {
					// 0xc0 catches the first two bits,
					// shifting by six places them at the last 2 bits.
					// Furthermore, every byte has a 0b10 header except
					// the first byte.
					return false;
				}
			}
		}
		return true;
	}
}

// negative values are errors
int utf8_strlen(const char *str) {
	// assertions
	if (!str) {
		return -1; // error
	}

	// assert init was successful
	utf8_parser *parser = utf8_pinit(str);
	if (!parser) {
		return -1;
	}

	uint32_t i;
	for (i = 0; utf8_pget(parser) >= 0; i++);
	utf8_pfree(parser);
	return i;
}

utf8_rune utf8_readrune(const char *str, size_t len) {
	// assertions
	if(!str || !(len <= sizeof(utf8_rune))) {
		return utf8_RUNE_ERROR;
	}

	utf8_rune cp;
	void *mem = memcpy(&cp, str, len);
	if (!mem) {
		return utf8_RUNE_ERROR;
	}
	return cp;
}

// allocate zeroed memory
static inline void *alloc(size_t size) {
	return calloc(size, 1);
}

// returns null on failure
utf8_parser *utf8_pinit(const char *str) {
	// assertions
	if (!str) {
		return NULL;
	}

	utf8_parser *p = alloc(sizeof(utf8_parser));
	p->str = str;
	return p;
}

void utf8_pfree(utf8_parser *parser) {
	free(parser);
}

const char *utf8_pgets(utf8_parser *parser) {
	// assertions
	if (!parser || !parser->str) {
		return NULL; // error
	}

	return &parser->str[parser->index];
}

// return values:
// zero indicates success.
// one indicates eof.
// negative numbers are errors.
utf8_rune utf8_pget(utf8_parser *parser) {
	// assertions
	if (!parser || !parser->str) {
		return utf8_RUNE_ERROR;
	}

	utf8_rune cp;
	const char *str = utf8_pgets(parser);
	if (!str) {
		return utf8_RUNE_ERROR;
	}

	// get byte length of token.
	const int bl = utf8_runelens(str);
	if (bl < 0) {
		// byte length out of bounds
		return utf8_RUNE_INVALID;
	}

	if (bl == 0) {
		// standard ascii value
		cp = utf8_readrune(str, 1);
		parser->index++;
	} else if (bl == 1) {
		// unsupported character
		return utf8_RUNE_INVALID;
	} else {
		// multi-byte unicode value
		// check for first two bits are 10
		for (int i = 1; i < bl; i++) {
			if ((str[i] & 0xc0) != 0x80) {
				return utf8_RUNE_INVALID; // error
			}
		}
		cp = utf8_readrune(str, bl);
		parser->index += bl;
	}

	if (utf8_isvalid(cp)) {
		return cp;
	} else {
		// invalid rune
		return utf8_RUNE_INVALID;
	}
}
