
// Copyright (c) 2015 Connor Taffe

#ifndef CONN_UTF8_H_
#define CONN_UTF8_H_

// C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

extern const int utf8_CODEPOINT_MAX;
extern const size_t utf8_RUNE_MAXLEN;

// utf8_rune
// a rune is a single utf-8 encoded character,
// stored in a 4-byte integer.
// negative values are considered errors.
typedef int32_t utf8_rune;

// rune value defines
enum {
	utf8_CP_ERROR = INT32_MIN,
	utf8_CP_INVALID,
};

enum {
	// 0xc0 and 0xc1 are guaranteed not to appear.
	utf8_RUNE_ERROR = 0xc0,
	utf8_RUNE_INVALID,
	// 0xf5-0xff are guaranteed to not be valid.
	utf8_RUNE_SHORT = 0xf5,
};

// utf8_getr
// returns a rune from some memory, of max length size.
utf8_rune utf8_getr(const void *mem, size_t size);

// utf8_runelen
// accepts a byte and returns the following rune's length.
int utf8_runelen(const char byte);

// utf8_isvalid
// accepts a Rune and returns whether it is
// a valid character as a bool.
bool utf8_isvalid(const utf8_rune cp);

// utf8_isstartbyte
// accepts a uint8_t and returns whether it is
// a start rune as a bool.
bool utf8_isstartbyte(const uint8_t rune);

// utf8_encode
// accepts a 4-byte codepoint and a pointer to a Rune.
// it returns a 64-bit signed integer that either
// represents the size of the rune, or a negative value
// representing an error.
utf8_rune utf8_encode(const int32_t cp);

// utf8_decode
// accepts a Rune and a 4-byte codepoint pointer.
// it returns an integer that indicates error on non-zero values.
int32_t utf8_decode(const utf8_rune rune);

// utf8_strlen
// returns the number of Runes in a String.
int utf8_strlen(const char *str);


// Parser
// the parser is used to store the location in a string
// and progressively parse a string.
typedef struct {
	const char *str;
	size_t index;
} utf8_parser;

// utf8_pinit
// accepts a String as input, and returns
// a Parser pointer. If the pointer is null,
// an error has occured. The pointer must be free'd by
// Parser_free.
utf8_parser *utf8_pinit(const char *str);

// utf8_pfree
// accepts a parser to free and returns nothing.
void utf8_pfree(utf8_parser *p);

// utf8_pget
// accepts a Parser and a Rune pointer codepoint.
// on success codepoint will point contain a parsed rune,
// on error a non-zero value will be returned.
utf8_rune utf8_pget(utf8_parser *p);

// C++ compatibility
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // CONN_UTF8_H_
