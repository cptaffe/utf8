
// Copyright (c) 2015 Connor Taffe

#ifndef CONN_UTF8_H_
#define CONN_UTF8_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint32_t utf8_Rune;
typedef struct {
	size_t len;
	const uint8_t *str;
} utf8_String;

typedef struct {
	const utf8_String *str;
	size_t index;
} utf8_Parser;

// utf8 parser
utf8_Parser *utf8_Parser_init(const utf8_String *str);
void utf8_Parser_free(utf8_Parser *parser);

utf8_String *utf8_String_init(const uint8_t *str);
void utf8_String_free(utf8_String *str);

int utf8_StringRuneLen(const utf8_String *str);
int utf8_RuneLen(const utf8_Rune r);
bool utf8_BomCheck(const utf8_String *str);
bool utf8_ValidRune(const utf8_Rune cp);
int utf8_RuneCount(const utf8_String *str);
int utf8_Parser_Get(utf8_Parser *parser, utf8_Rune *codepoint);

uint32_t utf8_RuneDecode(const utf8_Rune rune);
int utf8_RuneEncode(const uint32_t cp, utf8_Rune *rune);

#endif // CONN_UTF8_H_
