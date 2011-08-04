/*  charclass.h - A higher-level wrapper for character classes
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

#ifndef CHARCLASS_H 
#define CHARCLASS_H

#ifdef __cplusplus
/* *INDENT-OFF* */
extern  "C" {
/* *INDENT-ON* */
#endif

#include "flexdef.h"
#include "unicode.h"

/* Char Class functions */

typedef struct charclass_range range;
typedef struct charclass_set charclass;

struct charclass_range {
	range * next;
	int start;
	int end;
};

struct charclass_set {
	range * first;
	int next_item;
	int max_item;
	enum encoding mode;
};

/* Allocate and initialize memory for a charclass */
extern charclass * charclass_init (void);

/* Copy an existing charclass */
extern charclass * charclass_copy (charclass * set);

/* Free all memory associated with a charclass */
extern void charclass_free (charclass * set);

/* Add a range to a charclass */
extern void charclass_add (charclass * set, int start, int end);

/* Remove a range from a charclass */
extern void charclass_remove (charclass * set, int start, int end);

/* Negate a charclass */
extern void charclass_negate (charclass * set);

/* Diff of 2 charclasses - set1 is both modified and returned; set2 is freed */
extern charclass * charclass_set_diff (charclass * set1, charclass * set2);

/* Union of 2 charclasses - set1 is both modified and returned; set2 is freed */
extern charclass * charclass_set_union (charclass * set1, charclass * set2);

/* The maximum number of Chars needed to represent a single codepoint.
   In UTF-8, thats the number of bytes; in UTF-16, that's up to 2, if a
   surrogate pair is needed */
extern int charclass_maxlen (charclass * set);

/* The minimum number of Chars needed to represent a single codepoint.
 * In UTF-8, thats the number of bytes; in UTF-16, that's up to 2, if a
 * surrogate pair is needed */
extern int charclass_minlen (charclass * set);

/* Convert a charclass into a (set of) states.  How this is done depends on
   global flags.   set will be freed  before returning */
extern int charclass_mkstate (charclass * set);

/* Return whether a particular codepoint is in this range */
extern bool charclass_contains (charclass * set, int codepoint);

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#endif

/* vim:set noexpandtab cindent tabstop=4 softtabstop=4 shiftwidth=4 textwidth=0: */
