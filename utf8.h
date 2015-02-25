
// Copyright (c) 2015 Connor Taffe

#ifndef CONN_UTF8_H_
#define CONN_UTF8_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	const char *str;
	int index;
	int len;
} utf8_Parser;

// utf8 parser
utf8_Parser *utf8_Parser_init(const char *str);
void utf8_Parser_free(utf8_Parser *parser);

int utf8_RuneLen(const char *str);
bool utf8_BomCheck(const char *str);
bool utf8_ValidRune(const uint32_t cp);
int utf8_RuneCount(const char *str);
int utf8_Parser_Get(utf8_Parser *parser, uint32_t *codepoint);


#endif // CONN_UTF8_H_
