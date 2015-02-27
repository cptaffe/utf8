
// Copyright (c) 2015 Connor Taffe

#ifndef CONN_UTF8_H_
#define CONN_UTF8_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

extern const int utf8_CODEPOINT_MAX;

#ifdef HAVE_STDIO

// utf8_fget
// takes a FILE pointer and returns a rune or 0 on eof.
utf8_rune utf8_fget(FILE *file);

// utf8_funget
// takes a FILE pointer and pushes a rune, returns non-zero on error.
int utf8_funget(FILE *file, const utf8_rune r);

#endif // HAVE_STDIO

// utf8_rune
// a rune is a single utf-8 encoded character,
// stored in a 4-byte integer.
// negative values are considered errors.
typedef int32_t utf8_rune;

// rune value defines
enum {
	utf8_RUNE_ERROR = INT32_MIN,
	utf8_RUNE_INVALID,
};

// utf8_runelen
// accepts a Rune and returns its length in bytes.
int utf8_runelen(const utf8_rune r);

// utf8_isvalid
// accepts a Rune and returns whether it is
// a valid character as a bool.
bool utf8_isvalid(const utf8_rune cp);

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

// utf8_runeslen
// accepts a String and returns the length of
// the first Rune in bytes.
int utf8_runeslen(const char *str);

// utf8_strchkbom
// accepts a String and checks for a
// byte-order-mark at the beginning of the string.
bool utf8_strchkbom(const char *str);


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

// utf8_pgets
// returns string of as yet unparsed runes.
const char *utf8_pgets(utf8_parser *p);

#endif // CONN_UTF8_H_
