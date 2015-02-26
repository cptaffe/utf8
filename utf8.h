
// Copyright (c) 2015 Connor Taffe

#ifndef CONN_UTF8_H_
#define CONN_UTF8_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Rune
// a Rune is a single utf-8 encoded character,
// stored in a 4-byte integer.
// negative values are considered errors.
typedef int32_t utf8_rune;

// RuneLen
// accepts a Rune and returns its length in bytes.
int utf8_runelen(const utf8_rune r);

// isValidRune
// accepts a Rune and returns whether it is
// a valid character as a bool.
bool utf8_valid(const utf8_rune cp);

// RuneEncode
// accepts a 4-byte codepoint and a pointer to a Rune.
// it returns a 64-bit signed integer that either
// represents the size of the rune, or a negative value
// representing an error.
utf8_rune utf8_encode(const uint32_t cp);

// RuneDecode
// accepts a Rune and a 4-byte codepoint pointer.
// it returns an integer that indicates error on non-zero values.
int32_t utf8_decode(const utf8_rune rune);

// GetRune
// takes a FILE pointer and returns a rune or 0 on eof.
#ifdef HAVE_STDIO
utf8_rune utf8_fgetr(FILE *file);
#endif // HAVE_STDIO

// TODO: Add methods, splice out, or otherwise get rid of String.

// String
// the string allows for O(1) queries of a string's length
// strings are constant, so they cannot be mutated.
typedef struct {
	size_t len;
	const char *str;
} utf8_str;

extern const utf8_str UTF8_BOM;

// String_init
// accepts a c-string, and returns a String pointer,
// this pointer, if null, indicates an error.
// the pointer must be free'd by String_free.
utf8_str *utf8_strinit(const char *str);

// String_free
// accepts a String to free and returns nothing.
void utf8_strfree(utf8_str *str);

// RuneCountInString
// returns the number of Runes in a String.
int utf8_strlen(const utf8_str *str);

// StringRuneLen
// accepts a String and returns the length of
// the first Rune in bytes.
int utf8_runeslen(const utf8_str *str);

// StringBomCheck
// accepts a String and checks for a
// byte-order-mark at the beginning of the string.
bool utf8_strchkbom(const utf8_str *str);


// Parser
// the parser is used to store the location in a string
// and progressively parse a string.
typedef struct {
	const utf8_str *str;
	size_t index;
} utf8_parser;

// Parser_init
// accepts a String as input, and returns
// a Parser pointer. If the pointer is null,
// an error has occured. The pointer must be free'd by
// Parser_free.
utf8_parser *utf8_pinit(const utf8_str *str);

// Parser_free
// accepts a parser to free and returns nothing.
void utf8_pfree(utf8_parser *p);

// ParserGet
// accepts a Parser and a Rune pointer codepoint.
// on success codepoint will point contain a parsed rune,
// on error a non-zero value will be returned.
utf8_rune utf8_pget(utf8_parser *p);

// ParserGetString
// returns string of as yet unparsed runes.
utf8_str *utf8_pgets(utf8_parser *p);

#endif // CONN_UTF8_H_
