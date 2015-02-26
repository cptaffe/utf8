
// Copyright (c) 2015 Connor Taffe

// This file contains tests for the utf8 library.
#include <stdio.h>

#include "utf8.h"

// requires working utf8_encode
bool test_validation() {
	bool valid = utf8_valid(0xc0) | utf8_valid(0xc1);
	for (int32_t i = 0xf5; i <= 0xff; i++) {
		valid |= utf8_valid(i);
	}
	// if tests are successful
	if (!valid) {
		valid = true;
		char str[5] = {0};
		utf8_rune *rune = ((utf8_rune *) str);
		for (int32_t i = 0; i < 0x10ffff; i++) {
			*rune = utf8_encode(i);
			valid &= utf8_valid(*rune);
			if (!valid) {
				printf("'%s', U+%X-encode->%#x declared invalid.\n", (char *) str, i, *rune);
				return false;
			}
		}
		// success
		return true;
	} else {
		return false;
	}
}

bool test_encode() {
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

int main() {
	// utf8_encode testing
	if (test_encode()) {
		puts("encode success.");
	} else {
		puts("encode failure.");
		return 1;
	}

	// utf8_valid testing
	if (test_validation()) {
		puts("validation success.");
	} else {
		puts("validation failure.");
		return 1;
	}
}
