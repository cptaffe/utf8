
// Copyright (c) 2015 Connor Taffe

// This file contains tests for the utf8 library.
#include <stdio.h>

#include "utf8.h"

bool test_valid_invalid() {
	bool valid = utf8_valid(0xc0) | utf8_valid(0xc1);
	for (int i = 0xf5; i <= 0xff; i++) {
		valid |= utf8_valid(i);
	}
	return !valid;
}

int main() {
	// utf8_valid testing
	if (test_valid_invalid()) {
		puts("validation success.");
	} else {
		puts("validation failure.");
	}

	
}
