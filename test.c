
// Copyright (c) 2015 Connor Taffe

// This file contains tests for the utf8 library.
#include <stdio.h>
#include <string.h>

#include "utf8.h"

// change what these function pointers
// point to to alter the API.

bool(*const func_isvalid)(const utf8_rune) = &utf8_isvalid;
utf8_rune(*const func_encode)(const int32_t) = &utf8_encode;
int32_t(*const func_decode)(const utf8_rune) = &utf8_decode;

// Utility Functions

// prints bits in num
static void print_bits(int32_t num) {
	for (int i = 0; i < (sizeof(num) * 8); i++) {
		putchar(((num & (1 << i)) >> i) + '0');
	}
}

// prints bits in num,
// printing differing 1 bits as bright white.
static void print_bits_diff(int32_t num, int32_t num2) {
	for (int i = 0; i < (sizeof(num) * 8); i++) {
		int8_t j = ((num & (1 << i)) >> i);
		int8_t k = ((num2 & (1 << i)) >> i);
		char c = j + '0';
		if (j != k && j) {
			// print 1's that don't belong in bright white.
			printf("\x1b[37;1m%c\x1b[0m", c);
		} else {
			putchar(c);
		}
	}
}

// requires working utf8_encode
bool test_validation() {
	bool valid = func_isvalid(0xc0) | func_isvalid(0xc1);
	for (int32_t i = 0xf5; i <= 0xff; i++) {
		valid |= func_isvalid(i);
	}
	// if tests are successful
	if (!valid) {
		valid = true;
		char str[sizeof(utf8_rune) + 1] = {0};
		utf8_rune *rune = ((utf8_rune *) str);
		for (int32_t i = 0; i < 0x10ffff; i++) {
			*rune = func_encode(i);
			valid &= func_isvalid(*rune);
			if (!valid) {
				printf("'%s', U+%X-encode->%#x declared invalid.\n", (char *) str, i, *rune);
				return false;
			}
		}
		// success if we reach this point
		return true;
	} else {
		return false;
	}
}

bool test_encode_range() {
	// out of range negative values,
	// just testing a few
	int32_t test_arr[] = {INT32_MIN, -200, -1, INT32_MAX, 0x11ffff};
	for (int32_t i = 0; i < (sizeof(test_arr) / sizeof(int32_t)); i++) {
		utf8_rune r = func_encode(test_arr[i]);
		if (r != utf8_RUNE_INVALID) {
			// incorrect!
			if (r == utf8_RUNE_ERROR) {
				printf("returned ERROR (%#x) on malformed rune, not INVALID (%#x).\n", utf8_RUNE_ERROR, utf8_RUNE_INVALID);
			} else {
				printf("encoded out of bounds: %d (%#x)\n", test_arr[i], test_arr[i]);
			}
			return false;
		}
	}
	return true;
}

bool test_encode_char(uint32_t code, char *ref) {
	char str[sizeof(utf8_rune) + 1] = {0};
	*((utf8_rune *) str) = func_encode(code);
	if (!utf8_isvalid(*((utf8_rune *) str))) {
		printf("returned error %#x, an invalid rune.\n", *((utf8_rune *) str));
		return false;
	}
	if (strncmp((char *) str, ref, 4) != 0) {
		// debug messages.
		printf("U+%X->'%s' vs '%s'\n", code, ref, str);
		printf("encode: "); print_bits_diff(*((int32_t *) str), *((int32_t *) ref)); putchar('\n');
		printf("ref:    "); print_bits_diff(*((int32_t *) ref), *((int32_t *) str)); putchar('\n');
		printf("num:    "); print_bits(code); putchar('\n');
		return false;
	}
	return true;
}

bool test_encode() {
	// out of range
	if (!test_encode_range()) {
		return false;
	}
	// enocde a few characters to test correctness.
	if (!test_encode_char(0x7684, "\u7684")) {
		return false;
	}
	if (!test_encode_char(0x7680, "\u7680")) {
		return false;
	}
	return true;
}

// Tests decode in terms of encode.
// Assumes encode will encode valid code-points correctly.
// Practically, if something breaks here, it means one of encode
// or decode is broken.
bool test_decode_with_encode() {
	// enocde/decode all possible characters.
	// Assuming encode will encode valid code-points correctly.
	for (int i = 0; i < 0x10ffff; i++) {
		utf8_rune r = func_encode(i);
		if (!utf8_isvalid(r)) {
			// check for validity.
			printf("returned error %#x, an invalid rune.\n", r);
			return false;
		}
		if (r == utf8_RUNE_ERROR) {
			printf("encode returned an error on U+%X\n", i);
			return false;
		}
		int32_t cp = func_decode(r);
		if (cp < 0) {
			printf("decode returned an error on '%.4s'\n", (char *) &r);
			return false;
		}
		if (cp != i) {
			// debug messages
			printf("U+%X--encode-->'%.4s'--decode-->U+%X\n", i, (char *) &r, cp);
			printf("decode: "); print_bits_diff(cp, i); putchar('\n');
			printf("ref:    "); print_bits_diff(i, cp); putchar('\n');
			printf("num:    "); print_bits(r); putchar('\n');
			return false;
		}
	}

	// Test for decoding invalid runes beginning with 10.
	utf8_rune runes[] = {0b10111111};
	for (int i = 0; i < (sizeof(runes) / sizeof(utf8_rune)); i++) {
		utf8_rune r = func_decode(runes[i]);
		if (r != utf8_CP_INVALID) {
			if (r == utf8_CP_ERROR) {
				printf("returned ERROR (%#x) on malformed rune, not INVALID (%#x).\n", utf8_CP_ERROR, utf8_CP_INVALID);
			} else {
				printf("returned %#x on invalid rune %#x\n", r, runes[i]);
			}
			return false;
		}
	}

	// Testing decode on invalid rune values.
	return true;
}

// requires working utf8_encode.
bool test_parser() {
	char *str = "\u7684\u7680";
	utf8_rune runes[] = {0x7684, 0x7680};
	utf8_parser *p = utf8_pinit(str);
	if (!p) {
		return false;
	}

	int i = 0;
	// ends loop on error or null terminator.
	for (utf8_rune r = 0; (r = utf8_pget(p)) > 0; i++) {
		if (i >= 2) {
			puts("parser did not return 0 at null.");
			utf8_pfree(p);
			return false;
		}

		utf8_rune rune = utf8_encode(runes[i]);
		if (r != rune) {
			// debug messages, only print 4 characters as runes
			// are only 4 bytes.
			printf("U+%X->'%.4s' vs '%.4s'\n", runes[i], (char *) &rune, (char *) &r);
			printf("parser: "); print_bits_diff(r, rune); putchar('\n');
			printf("ref:    "); print_bits_diff(rune, r); putchar('\n');
			printf("num:    "); print_bits(runes[i]); putchar('\n');
			utf8_pfree(p);
			return false;
		}
	}
	utf8_pfree(p);
	return true;
}

int main() {
	int ret = 0;

	// utf8_encode testing
	if (test_encode()) {
		puts("encode success.");

		// utf8_decode testing (dependent on utf8_encode)
		if (test_decode_with_encode()) {
			puts("decode success.");
		} else {
			puts("decode failure.");
			ret = 1;
		}

		// utf8_valid testing (dependent on utf8_encode)
		if (test_validation()) {
			puts("validation success.");
		} else {
			puts("validation failure.");
			ret = 1;
		}

		// utf8_pget testing (dependent on utf8_encode)
		if (test_parser()) {
			puts("parser success.");
		} else {
			puts("parser failure.");
			ret = 1;
		}
	} else {
		puts("encode failure.");
		ret = 1;
	}

	return ret;
}
