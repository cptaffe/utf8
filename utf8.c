
// Copyright (c) 2015 Connor Taffe

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utf8.h"

const int32_t utf8_CODEPOINT_MAX = 0x10ffff;
const size_t utf8_RUNE_MAXLEN = 4;

// returns null on failure
utf8_parser *utf8_pinit(const char *str) {
	utf8_parser *p;

	// assertions
	if (!str) {
		return NULL;
	}

	// init p
	p = calloc(sizeof(utf8_parser), 1);
	p->str = str;
	return p;
}

void utf8_pfree(utf8_parser *parser) {
	free(parser);
}

utf8_rune utf8_pget(utf8_parser *parser) {
	const char *str;
	utf8_rune rune;

	// assertions
	if (parser == NULL || parser->str ==  NULL) {
		return utf8_RUNE_ERROR;
	}

	str = &parser->str[parser->index];
	// strlen excludes the null terminator, which we need to parse.
	rune = utf8_getr(str, strlen(str) + 1);
	if (utf8_isvalid(rune)) {
		parser->index += utf8_runelen(parser->str[parser->index]);
	}
	return rune; // propogate error, if exists
}

// used for finding start rune
bool utf8_isstartbyte(const uint8_t rune) {
	// first bit is 0 or first two bits are 11.
	return ((rune & 0xc0) == 0xc0 || (rune & 0x80) == 0);
}

int utf8_runelen(const char byte) {
	uint8_t mask = 0;
	int i, bytelength = 0;


	for (i = utf8_RUNE_MAXLEN - 1; i >= 0; i--) {
		mask |= 1 << (i + (8 - utf8_RUNE_MAXLEN)); // eight is bits in a byte.
		if ((byte & mask) != mask) {
			// find maximum bits set before zero bit is found.
			break;
		}
		bytelength++;
	}

	if (bytelength == 1) {
		// 10xxxxxx is reserved for non-startbytes.
		return -1;
	} else if (bytelength == 0) {
		// encoding specifies 0xxxxxxx means 1 byte.
		bytelength++;
	}

	return bytelength;
}

// negative values are errors
int utf8_strlen(const char *str) {
	utf8_parser *parser;
	int i;

	// assertions
	if (str == NULL) {
		return -1; // error
	}

	// assert init was successful
	parser = utf8_pinit(str);
	if (parser == NULL) {
		return -1; // error
	}

	for (i = 0;; i++) {
		utf8_rune rune = utf8_pget(parser);
		if (rune == 0) {
			utf8_pfree(parser);
			return i; // null terminator
		} else if (!utf8_isvalid(rune)) {
			utf8_pfree(parser);
			return -1; // error
		}
	}
}

// reads rune from arbitrary memory
utf8_rune utf8_getr(const void *mem, size_t size) {
	uint8_t *bytes;

	// assertions
	if (mem == NULL || size == 0) {
		return utf8_RUNE_ERROR;
	}

	bytes = (uint8_t *) mem;

	// check if first byte is a starting byte.
	if (utf8_isstartbyte(bytes[0])) {
		int len;

		// decode rune
		len = utf8_runelen(bytes[0]);
		if (len > 0) {
			if (len <= size) {
				int i;

				// copy rune
				utf8_rune rune = 0;
				memcpy(&rune, mem, len);

				// check for valid following characters
				for (i = 1; i < len; i++) {
					// check first two bytes for 10xxxxxx
					if ((((uint8_t *) &rune)[i] & 0xc0) != 0x80) {
						return utf8_RUNE_INVALID;
					}
				}

				// have good rune.
				return rune;
			} else {
				return utf8_RUNE_SHORT;
			}
		} else {
			return utf8_RUNE_INVALID;
		}
	} else {
		return utf8_RUNE_INVALID;
	}
}

// returns codepoint value of utf-8 character
int32_t utf8_decode(const utf8_rune rune) {
	int32_t codepoint = 0;
	utf8_rune mask = 0;
	int bytelen, i;

	// assertions
	if (!utf8_isvalid(rune)) {
		return utf8_CP_INVALID;
	}

	// get number of bits.
	bytelen = utf8_runelen(rune);
	if (bytelen <= 0) {
		// out of bounds
		return utf8_CP_INVALID;
	} else if (bytelen == 1) {
		return rune; // rune is code-point.
	}

	// forge mask
	for (i = 0; i < (7 - bytelen); i++) {
		mask += (1 << i);
	}

	// shift first bits by the the number of bits in each
	// subsequent byte (6) once for every remaining byte
	codepoint = (rune & mask) << (bytelen - 1) * 6;
	mask = 0x3f;
	for (i = 1; i < bytelen; i++) {
		// mask each byte after the first.
		// shift by (bl - i) so lower bits have higher values,
		// multiply this by six (number of bits per byte),
		// which leaves the shift needed to place lower bytes near
		// the first byte and provide the inverse ordering needed
		// to get a correct number.
		codepoint |= ((((uint8_t *) &rune)[i] & mask)
			<< (((bytelen - i) - 1) * 6));
	}

	return codepoint;
}

// returns encoded rune from codepoint
utf8_rune utf8_encode(const int32_t codepoint) {
	utf8_rune rune = 0;
	int bytelen = 0;

	// get number of bytes
	// limits as indicated in RFC 3629.
	if (codepoint >= 0 && codepoint < 0x80) {
		bytelen = 1;
	} else if (codepoint >= 0x80 && codepoint < 0x800) {
		bytelen = 2;
	} else if (codepoint >= 0x800 && codepoint < 0xffff) {
		bytelen = 3;
	} else if (codepoint >= 0xffff && codepoint <= 0x10ffff) {
		bytelen = 4;
	} else {
		// not an encodeable character value
		return utf8_RUNE_INVALID; // error
	}

	if (bytelen == 1) {
		rune = (utf8_rune) codepoint; // no special transform
	} else {
		uint32_t mask = 0, code = 0;
		int i, offset;

		/* generate the rune heading.
			|              8-bits              |
			<byte-length 1-bits><0-bit><payload>
		*/
		for (i = 0; i < bytelen; i++) {
			code |= (1 << (7 - i));
		}

		/* generate the byte headings.
			10xxxxxx, where x is payload (code-point) bits.
			^ this bit is the important one.

			We skip 8 * offset, which gets us to the beginning
			of the wanted byte, and then add 7, to get us to the
			index of the last bit of that byte.
		*/
		for (i = 0; i < bytelen; i++) {
			code |= (1 << ((8 * i) + 7));
		}

		offset = bytelen - 1;

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
		for (i = 0; i < (7 - bytelen); i++) {
			mask |= (1 << i) << (offset * 6);
		}

		// or mask, after being shifted back down.
		code |= (mask & codepoint) >> (offset * 6);

		// now we iterate over the next bytes and mask their bits.
		for (i = 1; i < bytelen; i++) {
			// 6 bit mask for each byte, 0x3f = 0b00111111.
			// we shift the mask by the inverse of our counter
			// (we order the last parts to the beginning), and
			// subtract one because we only do this for bytes
			// after the 1st. Then we multiply by six as six
			// is the number of bits in a byte sans header.
			code |= ((codepoint & (0x3f << (((bytelen - i) - 1) * 6)))
				// and reverse the shift, then shift back up i bytes.
				>> (6 * ((bytelen - i) - 1))) << (8 * i);
		}
		rune = code;
	}
	return rune;
}


// checks byte for validity.
bool utf8_isvalid(const utf8_rune rune) {
	// RFC 3629 mandates the the first byte indicate
	// number of following bytes.
	int len = utf8_runelen(rune);
	if (len <= 0) {
		return false;
	} else {
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
		return true;
	}
}
