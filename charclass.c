/*  charclass.c - A higher-level wrapper for character classes
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
 *     notice, this set of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this set of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE.
 */

#include "flexdef.h"
#include "charclass.h"
#include "unicode.h"

enum {
	INITIAL_LIST_LENGTH = 64,
	LIST_INC = 32
};

static range * next(charclass * set);

/** Allocate and initialize memory for a charclass
 *  @return An empty charclass
 */
charclass * charclass_init (void)
{
	charclass * ret = flex_alloc(sizeof(charclass));
	if (ret == NULL)
		flexfatal(_("Could not allocate memory in charclass_init"));

	ret->next_item = 0;
	ret->max_item = INITIAL_LIST_LENGTH;
	ret->first = allocate_array(ret->max_item, sizeof(range));
	ret->mode = (sf_utf8() ? UTF8 : ASCII);

	return ret;
}

/** Copy an existing charclass
 *  @param set The charclass to copy
 *  @return The new charclass
 */
charclass * charclass_copy (charclass * set)
{
	charclass * ret = flex_alloc(sizeof(charclass));
	if (ret == NULL)
		flexfatal(_("Could not allocate memory in charclass_init"));

	ret->next_item = set->next_item;
	ret->max_item = set->max_item;
	ret->first = allocate_array(ret->max_item, sizeof(range));

	range * olditem = set->next_item ? set->first : NULL;
	range * newitem;
	int next = 0;
	while (olditem) {
		newitem = &ret->first[next];
		newitem->start = olditem->start;
		newitem->end = olditem->end;
		newitem->next = olditem->next ? &ret->first[next+1] : NULL;

		olditem = olditem->next;
		next++;
	}

	return ret;
}

/** Free all memory associated with a charclass
 *  @param set The charclass
 *  @return None
 */
void charclass_free (charclass * set)
{
	if (set == NULL)
		flexfatal("charclass_free passed a null pointer");

	flex_free(set->first);
	flex_free(set);
}

/** Add a range to a charclass
 *  @param set The charclass
 *  @param start The start point of the range to add, inclusive
 *  @param end The end point of the range to add, inclusive
 *  @return None
 */
void charclass_add (charclass * set, int start, int end)
{
	if (set == NULL)
		flexfatal("charclass_add passed a null pointer");

	if (start > end)
		flexfatal(_("negative range in character class"));

	range * newitem = NULL;
	range * item = set->first;
	range * lastitem = NULL;

	/* If this is first, set it and quit */
	if (set->next_item == 0) {
		set->next_item = 1;
		item->start = start;
		item->end = end;
		item->next = NULL;
		return;
	}

	/* Loop through the current items.
	   If the new range overlaps or is adjacent, subsume the old.
	   If the new range preceeds the next, add a new item. */
	while (item) {
		if (start <= (item->end + 1) && end >= (item->start - 1)) {
			if (! newitem) {
				newitem = item;
				newitem->start = MIN(start,newitem->start);
				newitem->end = MAX(end,newitem->end);
			} else {
				/* Absorb the next item (and let it dangle) */
				newitem->end = MAX(newitem->end,item->end);
				newitem->next = item->next;
			}
		} else if (end < item->start) {
			/* If this will be first, create a new item and do a swap.
			   Otherwise, just insert it before */
			if (lastitem) {
				newitem = next(set);
				lastitem->next = newitem;
				newitem->next = item;
			} else {
				/* item == set->first, so we can't move it */
				newitem = item;
				item = next(set);
				item->start = newitem->start;
				item->end = newitem->end;
				item->next = newitem->next;
			}
			newitem->start = start;
			newitem->end = end;
			newitem->next = item;
			break;
		}

		lastitem = item;
		item = item->next;
	}

	/* If no item was found, add to the end */
	if (!newitem) {
		newitem = next(set);
		newitem->start = start;
		newitem->end = end;

		if (lastitem)
			lastitem->next = newitem;
	}

	return;
}

/** Remove a range from a charclass
 *  @param set The charclass
 *  @param start The start point of the range to remove, inclusive
 *  @param end The end point of the range to remove, inclusive
 *  @return None
 */
void charclass_remove (charclass * set, int start, int end)
{
	if (set == NULL)
		flexfatal("charclass_remove passed a null pointer");

	if (start > end)
		flexfatal("negative range in charclass_remove");

	range * newitem = NULL;
	if (set->next_item == 0)
		return;

	range *item = set->first, *lastitem = NULL, *nextitem = NULL;

	while (item) {

		nextitem = item->next;

		/* Since everything's sorted, it's safe to abort now */
		if (item->start > end)
			break;

		/* Not in a range we have yet */
		if (start > item->end) {
			lastitem = item;
			item = nextitem;
			continue;
		}

		/* The range we have is completely contained, and can be removed */
		if (start <= item->start && end >= item->end) {
			if (lastitem) {
				lastitem->next = nextitem;
				/* lastitem shouldn't be reset */
				item = nextitem;
				continue;
			} else {
				if (nextitem) {
					item->start = nextitem->start;
					item->end = nextitem->end;
					item->next = nextitem->next;
					/* Try again */
					continue;
				} else {
					/* Everything's gone! */
					set->next_item = 0;
					break;
				}
			}
		} else if (start > item->start && end >= item->end) {
			item->end = start - 1;
		} else if (end < item->end && start <= item->start) {
			item->start = end + 1;
			break;
		} else if (start > item->start && end < item->end) {
			/* Add a new element between this and the next. */
			item->next = next(set);
			item->next->next = nextitem;
			nextitem = item->next;
			nextitem->start = end + 1;
			nextitem->end = item->end;

			item->end = start - 1;

			break;
		}

		lastitem = item;
		item = nextitem;
	}
}

/** Negate a charclass.  In Unicode mode, the range is 0 to UNICODE_MAX;
 *  otherwise, the range is 0 to csize
 *  @param set The charclass
 *  @return None
 */
void charclass_negate (charclass * set)
{
	if (set == NULL)
		flexfatal("charclass_negate passed a null pointer");

	range * curitem , * nextitem;
	int nextstart = 0;
	int min = 1, max = sf_unicode() ? UNICODE_MAX : csize - 1;

	/* Short circuit if this is empty */
	if (set->next_item == 0) {
		curitem = next(set);
		curitem->start = min;
		curitem->end = max;
		return;
	}

	/* Change first to be the range from 0 to first->start - 1 */
	curitem = set->first;
	if (curitem->start > 0) {
		int start = curitem->start, end = curitem->end;

		/* Initialize the first subrange */
		curitem->start = 0;
		curitem->end = start - 1;

		/* If needed, insert a new item after the first */
		if (end != max) {
			nextitem = next(set);
			nextitem->next = curitem->next;
			nextitem->start = start;
			nextitem->end = end;
			curitem->next = nextitem;
			curitem = nextitem;
		} else {
			curitem = NULL;
		}
	} else if (curitem->end == max) {
		set->next_item = 0;
		curitem = NULL;
	}

	/* For each item, change range to item->end + 1 to next->start - 1 */
	while (curitem) {
		nextitem = curitem->next;

		curitem->start = curitem->end + 1;
		if (!nextitem) {
			curitem->end = max;
		} else {
			curitem->end = nextitem->start - 1;
			if (nextitem->end == max) {
				curitem->next = NULL;
				break;
			}
		}

		curitem = nextitem;
		nextitem = nextitem ? nextitem->next : NULL;
	}
}

/** Set difference of two charclasses
 *  @param set1 The set to start with and return
 *  @param set2 The set of codepoints to remove from set1
 *  @return set1, after set2 is removed
 */
charclass * charclass_set_diff (charclass * set1, charclass * set2)
{
	if (set1 == NULL)
		flexfatal("charclass_set_diff passed a null pointer (set1)");

	if (set2 == NULL)
		flexfatal("charclass_set_diff passed a null pointer (set2)");

	if (set1->mode != set2->mode)
		flexfatal("charclass_set_diff cannot join two sets with different encodings");

	/* If set1 is empty, there's nothing left to remove */
	if (set1->next_item == 0) {
		charclass_free(set2);
		return set1;
	}

	/* set2 is empty, which makes this a no-op */
	if (set2->next_item == 0) {
		charclass_free(set2);
		return;
	}

	range * item = set2->first;
	while (item) {
		charclass_remove(set1, item->start, item->end);
		if (set1->next_item == 0)
			break;
		item = item->next;
	}

	charclass_free(set2);
	return set1;
}

/** Set union of two charclasses
 *  @param set1 The set to start with and return
 *  @param set2 The set of codepoints to add to set1
 *  @return set1, after set2 is added
 */
charclass * charclass_set_union (charclass * set1, charclass * set2)
{
	if (set1 == NULL)
		flexfatal("charclass_set_union passed a null pointer (set1)");

	if (set2 == NULL)
		flexfatal("charclass_set_union passed a null pointer (set2)");

	if (set1->mode != set2->mode)
		flexfatal("charclass_set_diff cannot join two sets with different encodings");

	/* set1 is empty, so the result is just set2 */
	if (set1->next_item == 0) {
		charclass_free(set1);
		return set2;
	}

	/* set2 is empty, which makes this a no-op */
	if (set2->next_item == 0) {
		charclass_free(set2);
		return set1;
	}

	range * item = set2->first;
	while (item) {
		charclass_add(set1, item->start, item->end);
		item = item->next;
	}

	charclass_free(set2);
	return set1;
}

/* The maximum number of Chars needed to represent a single codepoint.
 * In UTF-8, thats the number of bytes; in UTF-16, that's up to 2, if a
 * surrogate pair is needed
 * @param set The charclass to check
 * @return The maximum number of Chars */
int charclass_maxlen (charclass * set)
{
	if (set == NULL)
		flexfatal("charclass_maxlen passed a null pointer");

	if (set->next_item == 0)
		flexfatal("charclass_maxlen passed an empty character class");

	range * item = set->first;
	while (item->next)
		item = item->next;

	return codepointlen(item->end,set->mode);
}

/* The minimum number of Chars needed to represent a single codepoint.
 * In UTF-8, thats the number of bytes; in UTF-16, that's up to 2, if a
 * surrogate pair is needed
 * @param set The charclass to check
 * @return The minimum number of Chars */
int charclass_minlen (charclass * set)
{
	if (set == NULL)
		flexfatal("charclass_minlen passed a null pointer");

	if (set->next_item == 0)
		flexfatal("charclass_minlen passed an empty character class");

	return codepointlen(set->first->start,set->mode);
}

/** Return a state.  This does not free the range
 *  @param set The set to start with
 *  @return A new state representing the set of codepoints
 */
int charclass_mkstate (charclass * set)
{
	if (set == NULL)
		flexfatal("charclass_mkstate passed a null pointer");

	if (set->next_item == 0)
		flexfatal("charclass_mkstate passed an empty character class");

	/* For UTF-8 and UTF-16, a range can span multiple bytes.  If it does,
	   we delegate state creation to child functions and or them together.
	   Otherwise, a simple CCL will do. */
	int maxlen = charclass_maxlen(set);
	int state = NIL;
	int ccl = (maxlen == 1 ? cclinit() : 0);
	int i = 0;

	range * item = set->first;
	bool needsnull = item->start == 0;
	while (item) {
		if (maxlen == 1) {
			/* Add each possible value to the CCL, but 0 must go at the end */
			for (i=MAX(1,item->start);i<=item->end;i++) {
				ccladd(ccl,i);
			}
		} else 
			/* mkor works fine with one parameter NIL */
			state = mkor(state, mkrange(item->start,item->end,set->mode));

		item = item->next;
	}
	/* Now add 0 if needed */
	if (maxlen == 1 && needsnull) {
		ccladd(ccl,0);
	}

	/* Convert the CCL to a state */
	if (ccl) {
		tryecs(ccl);
		state = mkstate(-ccl);
	}

	charclass_free(set);

	return state;
}

/* Return whether a particular codepoint is in this range
 *  @param set The charclass to check
 *  @param codepoint The value to look for
 *  @return Whether codepoint is contained in the set
 */
extern bool charclass_contains (charclass * set, int codepoint)
{
	if (set == NULL)
		flexfatal("charclass_contains passed a null pointer");

	if (set->next_item == 0)
		flexfatal("charclass_contains passed an empty character class");

	range * item = set->first;
	while (item) {
		if (item->start <= codepoint && codepoint <= item->end)
			return true;
		if (item->start > codepoint)
			break;
		item = item->next;
	}

	return false;
}

/** INTERNAL: Return the next free item in the set, reallocating as necessary.
 *  This routine will call flexfatal itself it alloc fails.
 *  @param set The particular charclass to check/extend
 *  @return The next available item
 */
range * next (charclass * set)
{
	set->next_item++;

	if (set->next_item < set->max_item)
		set->first[set->next_item].next = NULL;

	if (set->next_item == 1)
		return set->first;

	if (set->next_item < set->max_item)
		return &set->first[set->next_item];

	set->max_item += LIST_INC;
	range * block = allocate_array(set->max_item, sizeof(range));

	int index = 0;
	range * nextitem = set->first;
	range * newitem = &block[0];
	while (nextitem) {
		newitem->start = nextitem->start;
		newitem->end = nextitem->end;
		newitem->next = &block[++index];

		newitem = newitem->next;
		nextitem = nextitem->next;
	}
	newitem->next = NULL;

	return newitem;
}

/* vim:set noexpandtab cindent tabstop=4 softtabstop=4 shiftwidth=4 textwidth=0: */
