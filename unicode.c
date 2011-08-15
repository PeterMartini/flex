/*  unicode.c - Unicode transformation layer
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

/* Note: The utf8 option forces csize to be 256; if this changes, various assumptions
   in the code will need to be revisited */

#include "flexdef.h"
#include "unicode.h"

enum {
	INITIAL_LIST_LENGTH = 64,
	LIST_INC = 32
};

static int mkutf8codepoint(unsigned int num);
static int mkutf8range(int start, int end);
static int mkutf8rangecont(int start, int end, bool cont);
static int contbyteccl(void);
static int asciiccl(void);

/** Create a series to represent a single codepoint
 *  @param codepoint The codepoint to encode
 *  @return A state that will match that codepoint
 */
int mkcodepoint (unsigned int codepoint, enum encoding mode)
{
	int state = 0;
	switch(mode){
		case ASCII: state = mkstate(codepoint); break;
		case UTF8: state = mkutf8codepoint(codepoint); break;
		default: flexfatal("Unknown mode in mkcodepoint"); break;
	};
	return state;
}

/** Create a pattern to match a range of codepoints
 *  @param start The first codepoint to match, inclusive
 *  @param end The last codepoint to match, inclusive
 *  @return A state that will match anything in that range
 */
int mkrange (int start, int end, enum encoding mode)
{
	int i = 0, ccl = 0, state = 0;
	switch(mode){
		case ASCII:
			ccl = cclinit();
			for(i=MAX(start,1);i<=end;i++)
				ccladd(ccl,i);
			if(start == 0)
				ccladd(ccl,0);
			tryecs(ccl);
			state = mkstate(-ccl);
			break;
		case UTF8:
			state = mkutf8range(start,end);
			break;
		default:
			flexfatal("Uknown mode in mkrange");
	};
	return state;
}

/** Create a state representing a single UTF-8 codepoint
 *  @param num The codepoint to match
 *  @return A machine that matches that codepoint
 */
static int mkutf8codepoint ( unsigned int num )
{
	int state = 0;

	if(num <= UTF8_ENDBYTE1)
	{
		state = mkstate(num);
	}
	else if(num <= UTF8_ENDBYTE2)
	{
		state = mkstate(utf8b2c1(num));
		state = link_machines(state,mkstate(utf8b2c2(num)));
	}
	else if(num <= UTF8_ENDBYTE3)
	{
		state = mkstate(utf8b3c1(num));
		state = link_machines(state,mkstate(utf8b3c2(num)));
		state = link_machines(state,mkstate(utf8b3c3(num)));
	}
	else if(num <= UTF8_ENDBYTE4)
	{
		state = mkstate(utf8b4c1(num));
		state = link_machines(state,mkstate(utf8b4c2(num)));
		state = link_machines(state,mkstate(utf8b4c3(num)));
		state = link_machines(state,mkstate(utf8b4c4(num)));
	}
	else
	{
		synerr("Invalid codepoint detected");
		return 0;
	}

	return state;
}

/** Return a machine that will match a range of UTF-8 encoded codepoints
 *  @param start The first codepoint in the range
 *  @param end The last codepoint in the range
 *  @return A machine that will match any code point in the range
 */
static int mkutf8range ( int start, int end )
{
	int state = 0, nextstate = 0, rangestart = 0, rangeend = 0;

	if(start > end)
	{
		flexfatal("mkutf8range called with start > end");
	}

	if(start == end)
	{
		return mkutf8codepoint(start);
	}

	if(end > UNICODE_MAX)
	{
		synerr("Invalid codepoint detected");
		return 0;
	}

	if(start < UTF8_ENDBYTE1)
	{
		rangestart = start;
		rangeend = MIN(end,UTF8_ENDBYTE1);
		state = mkutf8rangecont(rangestart,rangeend,false);
	}

	if(start < UTF8_ENDBYTE2 && end > UTF8_ENDBYTE1)
	{
		rangestart = MAX(start,UTF8_ENDBYTE1+1);
		rangeend = MIN(end,UTF8_ENDBYTE2);
		nextstate = mkutf8rangecont(rangestart,rangeend,false);
		state = state ? mkor(state,nextstate) : nextstate;
	}

	if(start < UTF8_ENDBYTE3 && end > UTF8_ENDBYTE2)
	{
		rangestart = MAX(start,UTF8_ENDBYTE2+1);
		rangeend = MIN(end,UTF8_ENDBYTE3);
		nextstate = mkutf8rangecont(rangestart,rangeend,false);
		state = state ? mkor(state,nextstate) : nextstate;
	}

	if(end > UTF8_ENDBYTE3)
	{
		rangestart = MAX(start,UTF8_ENDBYTE3+1);
		rangeend = MIN(end,UTF8_ENDBYTE4);
		nextstate = mkutf8rangecont(rangestart,rangeend,false);
		state = state ? mkor(state,nextstate) : nextstate;
	}

	return state;
}

/** Helper function to recursively generate machines matching
 *  ranges of continuation bytes
 *  @param start The bottom of the range
 *  @param end The top of the range
 *  @param cont If true, all bits are part of continuation bytes
 *  @return A machine that matches all of the code points between (inclusively)
 */
static int mkutf8rangecont ( int start, int end, bool cont )
{
	int retstate = 0, state = 0, nextstate = 0, ccl = 0;
	int masklen = 0, top = 0, bytes = 0;
	int b1start = 0, remstart = 0, b1end = 0, remend = 0;
	int i = 0;

	if ( !cont && end > 0xffff) bytes = 4;
	else if( (!cont && end > 0x7ff) || (cont && end >= (1 << 12))) bytes = 3;
	else if( (!cont && end > 0x7f) || (cont && end >= (1 << 6))) bytes = 2;
	else if(end > 0) bytes = 1;
	else if(end == 0) return 0; /* Nothing to process! */
	else flexfatal("Negative end in mkutf8rangecont");

	/* Continuation bytes are formed by copying 6 bits to a byte
	   and setting the high bit.  So, we can divide the remainder
	   of the number into 6 bit chunks and handle them that way */
	masklen  = (bytes - 1) * 6;
	top	  = (1 << masklen) - 1;
	remstart = start & top;
	remend   = end   & top;

	b1start  = start >> masklen;
	b1end	= end   >> masklen;
	if(cont)
	{
		b1start = b1start | 0x80;
		b1end   = b1end   | 0x80;
	} else if(bytes > 1){
		/* 110xxxx is a 2 byte sequence, 1110xxx is 3, 11110xxx is 4 */
		b1start = b1start | (0xff ^ ((1 << (8 - bytes)) - 1 ));
		b1end   = b1end   | (0xff ^ ((1 << (8 - bytes)) - 1 ));
	} /* else no masking needed */

	if(b1start == b1end){
		state = mkstate(b1start);
		nextstate = mkutf8rangecont(remstart,remend,true);
		retstate = nextstate ? link_machines(state,nextstate) : state;
	} else if(bytes == 1){
		/* Make a simple range */
		if(b1start == 0 && b1end == 127)
			ccl = asciiccl();
		else if(b1start == 128 && b1end == 191)
			ccl = contbyteccl();
		else {
			ccl = cclinit();
			/* Add each byte individually and in order, taking care to
			   add 0 as csize and add at the end */
			for(i=MAX(b1start,1);i<=b1end;i++)
				ccladd(ccl,i);
			if(b1start==0)
				ccladd(ccl,0);
			tryecs(ccl);
		}
		retstate = mkstate(-ccl);
	} else {
		/* Make up to 3 parts: a partial range on the low byte,
							   a whole range on the middle bytes
							   a partial range on the high byte */

		int rangestart = (remstart > 0) ? b1start + 1 : b1start;
		int rangeend   = (remend < top) ? b1end   - 1 : b1end;

		/* If this is a partial block or its the only full block, make a simple state */
		if(remstart > 0 || (rangestart == rangeend))
		{
			state = mkstate(b1start);
			nextstate = mkutf8rangecont(remstart,top,true);
			retstate = nextstate ? link_machines(state,nextstate) : state;
		}

		/* Make a range of first bytes covering all remainders */
		if(rangestart < rangeend)
		{
			if(rangestart == 128 && rangeend == 191)
				ccl = contbyteccl();
			else
			{
				ccl = cclinit();
				for(i=rangestart;i<=rangeend;i++)
					ccladd(ccl,i);
				tryecs(ccl);
			}
			state = mkstate(-ccl);

			nextstate = mkutf8rangecont(0,top,true);
			state = nextstate ? link_machines(state,nextstate) : state;

			retstate = retstate ? mkor(retstate,state) : state;
		}

		/* If this is a partial block or its the only full block, make a simple state */
		if(remend < top || remstart)
		{
			state = mkstate(b1end);
			nextstate = mkutf8rangecont(0,remend,true);

			state = nextstate ? link_machines(state,nextstate) : state;

			retstate = retstate ? mkor(retstate,state) : state;
		}

	}

	return retstate;
}

/** Return a CCL that will match anything ASCII
 *  @return That singleton CCL
 */
static int asciiccl(void)
{
	static int ccl = 0;
	int i = 0;

	if(!ccl)
	{
		ccl = cclinit();
		for(i=1;i<128;i++)
			ccladd(ccl,i);
		ccladd(ccl,0);
		tryecs(ccl);
	}

	return ccl;
}

/** Return a CCL that will match any valid continuation byte 
 *  @return That singleton CCL
 */
static int contbyteccl(void)
{
	static int ccl = 0;
	int i = 0;

	if(!ccl)
	{
		ccl = cclinit();
		for(i=128;i<192;i++)
			ccladd(ccl,i);
		tryecs(ccl);
	}

	return ccl;
}

/* vim:set noexpandtab cindent tabstop=4 softtabstop=4 shiftwidth=4 textwidth=0: */
