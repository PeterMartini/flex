/*  unicode.h - Unicode transformation layer
 *
 *  Copyright (c) 2011 Peter Martini
 *  All rights reserved.
 *
 *  This file is part of flex.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE.
 */

#ifndef UNICODE_H
#define UNICODE_H

#ifdef __cplusplus
/* *INDENT-OFF* */
extern  "C" {
/* *INDENT-ON* */
#endif

enum unicode_constants {
	UNICODE_MIN = 0,
	UNICODE_MAX = 0x10ffff
};

enum utf8_constants {
	UTF8_MIN = 0,
	UTF8_ENDBYTE1 = 0x7f,
	UTF8_ENDBYTE2 = 0x7ff,
	UTF8_ENDBYTE3 = 0xffff,
	UTF8_ENDBYTE4 = UNICODE_MAX
};

enum encoding {
	ASCII,
	UTF8
};

static inline int utf8len (int codepoint)
{
	return (codepoint <  UTF8_MIN	  ? 0 :
			codepoint <= UTF8_ENDBYTE1 ? 1 :
			codepoint <= UTF8_ENDBYTE2 ? 2 :
			codepoint <= UTF8_ENDBYTE3 ? 3 :
			codepoint <= UTF8_ENDBYTE4 ? 4 :
										 0  );
}

static inline int codepointlen (int codepoint, enum encoding mode)
{
	switch(mode){
		case ASCII: return 1;
		case UTF8: return utf8len(codepoint);
		default: flexfatal("Unknown mode in codepointlen");
	};
}

static inline int utf8b1c1 (int codepoint){ return (codepoint); }
static inline int utf8b2c1 (int codepoint){ return (0xc0 | ((codepoint >>  6) & 0x1f));}
static inline int utf8b2c2 (int codepoint){ return (0x80 | ( codepoint		& 0x3f));}
static inline int utf8b3c1 (int codepoint){ return (0xe0 | ((codepoint >> 12) & 0x0f));}
static inline int utf8b3c2 (int codepoint){ return (0x80 | ((codepoint >>  6) & 0x3f));}
static inline int utf8b3c3 (int codepoint){ return (0x80 | ( codepoint		& 0x3f));}
static inline int utf8b4c1 (int codepoint){ return (0xf0 | ((codepoint >> 18) & 0x0f));}
static inline int utf8b4c2 (int codepoint){ return (0x80 | ((codepoint >> 12) & 0x3f));}
static inline int utf8b4c3 (int codepoint){ return (0x80 | ((codepoint >>  6) & 0x3f));}
static inline int utf8b4c4 (int codepoint){ return (0x80 | ( codepoint		& 0x3f));}

/* Tiny wrapper to shorten all of the ECS building */
static inline void tryecs(int ccl)
{
	if (useecs)
		mkeccl(ccltbl + cclmap[ccl], ccllen[ccl], nextecm, ecgroup, csize, csize);
}

/* Create a series to represent a single codepoint */ 
extern int mkcodepoint (unsigned int codepoint, enum encoding);

/* Create a pattern to match a range of codepoints */
extern int mkrange (int start, int end, enum encoding);

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#endif

/* vim:set noexpandtab cindent tabstop=4 softtabstop=4 shiftwidth=4 textwidth=0: */
