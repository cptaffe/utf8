
// Copyright (c) 2015 Connor Taffe

// This file contains tests for the utf8 library.
#include <stdio.h>
#include <string.h>

#include "utf8.h"

// change what these function pointers
// point to to alter the API.

bool(*const func_isvalid)(const utf8_rune) = &utf8_isvalid;


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
			*rune = utf8_encode(i);
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
	// out of range negative values
	for (int32_t i = INT32_MIN; i < 0; i++) {
		if (utf8_encode(i) != utf8_RUNE_ERROR) {
			// incorrect!
			return false;
		}
	}
	// out of range positive numbers
	for (int32_t i = INT32_MAX; i > 0x10ffff; i--) {
		if (utf8_encode(i) != utf8_RUNE_ERROR) {
			// incorrect!
			return false;
		}
	}
	return true;
}

static void print_bits(int32_t num) {
	for (int i = 0; i < (sizeof(num) * 8); i++) {
		putchar(((num & (1 << i)) >> i) + '0');
	}
}

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

bool test_encode_char(uint32_t code, char *ref) {
	char str[sizeof(utf8_rune) + 1] = {0};
	*((utf8_rune *) str) = utf8_encode(code);
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
	/*if (!test_encode_range()) {
		return false;
	}*/
	// enocde a few characters to test correctness.
	if (!test_encode_char(0x7684, "\u7684")) {
		return false;
	}
	return true;
}

int main() {
	int ret = 0;

	// utf8_encode testing
	if (test_encode()) {
		puts("encode success.");

		// utf8_valid testing (dependent on utf8_encode)
		if (test_validation()) {
			puts("validation success.");
		} else {
			puts("validation failure.");
			ret = 1;
		}
	} else {
		puts("encode failure.");
		ret = 1;
	}

	return ret;
}
