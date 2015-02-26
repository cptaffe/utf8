
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
typedef uint32_t utf8_Rune;

// RuneLen
// accepts a Rune and returns its length in bytes.
int utf8_RuneLen(const utf8_Rune r);

// isValidRune
// accepts a Rune and returns whether it is
// a valid character as a bool.
bool utf8_isValidRune(const utf8_Rune cp);

// RuneEncode
// accepts a 4-byte codepoint and a pointer to a Rune.
// it returns a 64-bit signed integer that either
// represents the size of the rune, or a negative value
// representing an error.
int utf8_RuneEncode(const uint32_t cp, utf8_Rune *rune);

// RuneDecode
// accepts a Rune and a 4-byte codepoint pointer.
// it returns an integer that indicates error on non-zero values.
int utf8_RuneDecode(const utf8_Rune rune, uint32_t *cp);

// GetRune
// takes a FILE pointer and returns a rune or 0 on eof.
utf8_Rune utf8_GetRune(FILE *file);

// String
// the string allows for O(1) queries of a string's length
// strings are constant, so they cannot be mutated.
typedef struct {
	size_t len;
	const uint8_t *str;
} utf8_String;

// String_init
// accepts a c-string, and returns a String pointer,
// this pointer, if null, indicates an error.
// the pointer must be free'd by String_free.
utf8_String *utf8_String_init(const uint8_t *str);

// String_free
// accepts a String to free and returns nothing.
void utf8_String_free(utf8_String *str);

// StringRuneCount
// returns the number of Runes in a String.
int utf8_StringRuneCount(const utf8_String *str);

// StringRuneLen
// accepts a String and returns the length of
// the first Rune in bytes.
int utf8_StringRuneLen(const utf8_String *str);

// StringBomCheck
// accepts a String and checks for a
// byte-order-mark at the beginning of the string.
bool utf8_StringBomCheck(const utf8_String *str);


// Parser
// the parser is used to store the location in a string
// and progressively parse a string.
typedef struct {
	const utf8_String *str;
	size_t index;
} utf8_Parser;

// Parser_init
// accepts a String as input, and returns
// a Parser pointer. If the pointer is null,
// an error has occured. The pointer must be free'd by
// Parser_free.
utf8_Parser *utf8_Parser_init(const utf8_String *str);

// Parser_free
// accepts a parser to free and returns nothing.
void utf8_Parser_free(utf8_Parser *parser);

// ParserGet
// accepts a Parser and a Rune pointer codepoint.
// on success codepoint will point contain a parsed rune,
// on error a non-zero value will be returned.
int utf8_ParserGet(utf8_Parser *parser, utf8_Rune *codepoint);

// ParserGetString
// returns string of as yet unparsed runes.
utf8_String *utf8_ParserGetString(utf8_Parser *parser);

#endif // CONN_UTF8_H_
