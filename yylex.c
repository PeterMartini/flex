/* yylex - scanner front-end for flex */

/*  Copyright (c) 1990 The Regents of the University of California. */
/*  All rights reserved. */

/*  This code is derived from software contributed to Berkeley by */
/*  Vern Paxson. */

/*  The United States Government has rights in this work pursuant */
/*  to contract no. DE-AC03-76SF00098 between the United States */
/*  Department of Energy and the University of California. */

/*  This file is part of flex. */

/*  Redistribution and use in source and binary forms, with or without */
/*  modification, are permitted provided that the following conditions */
/*  are met: */

/*  1. Redistributions of source code must retain the above copyright */
/*     notice, this list of conditions and the following disclaimer. */
/*  2. Redistributions in binary form must reproduce the above copyright */
/*     notice, this list of conditions and the following disclaimer in the */
/*     documentation and/or other materials provided with the distribution. */

/*  Neither the name of the University nor the names of its contributors */
/*  may be used to endorse or promote products derived from this software */
/*  without specific prior written permission. */

/*  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR */
/*  PURPOSE. */

#include <ctype.h>
#include "flexdef.h"
#include "charclass.h"
#include "parse.h"

static void
sf_diff (const int cur, scanflags_t * added, scanflags_t * removed)
{
	int i = 0, max = 8 * sizeof(scanflags_t), prev = cur - 1;

	scanflags_t mask, unset;

	if (cur > 1) {
		const scanflags_t unset = ~ _sf_stk[prev];
		for (i = 0; i < max; i++) {
			mask = 1 << i;
			*added |= ((_sf_stk[cur] & mask) & (unset & mask));
		}
		for (i = 0; i < max; i++ ) {
			mask = 1 << i;
			*removed |= ((_sf_stk[prev] & mask) & (_sf_stk[cur] ^ mask));
		}
	} else {
		*added = _sf_stk[cur];
		*removed = ~ *added;
		if (*added & _SF_UNICODE) {
			*removed &= ~_SF_UNICODE;
		} else {
			*removed |= _SF_UNICODE;
		}
	}
}

/* yylex - scan for a regular expression token */

int     yylex ()
{
	int     toktype;
	static int beglin = false;
	static int group = 0;
	extern char *yytext;

	if (eofseen)
		toktype = EOF;
	else
		toktype = flexscan ();

	if (toktype == EOF || toktype == 0) {
		eofseen = 1;

		if (sectnum == 1) {
			synerr (_("premature EOF"));
			sectnum = 2;
			toktype = SECTEND;
		}

		else
			toktype = 0;
	}

	if (trace) {
		if (beglin) {
			fprintf (stderr, "%d\t", num_rules + 1);
			beglin = 0;
		}

		if (group) {
			scanflags_t added = 0, removed = 0;
			sf_diff (group, &added, &removed);
			if (added || removed) {
				(void) putc('?', stderr);
				if (added & _SF_UNICODE) (void) putc('u', stderr);
				if (added & _SF_DOT_ALL) (void) putc('s', stderr);
				if (added & _SF_CASE_INS) (void) putc('i', stderr);
				if (added & _SF_SKIP_WS) (void) putc('x', stderr);
				if (removed) {
					(void) putc('-', stderr);
					if (removed & _SF_SKIP_WS) (void) putc('x', stderr);
					if (removed & _SF_CASE_INS) (void) putc('i', stderr);
					if (removed & _SF_DOT_ALL) (void) putc('s', stderr);
					/* _SF_UNICODE is actually multiple bits */
					if (removed & _SF_UNICODE) (void) putc('u', stderr);
				}
				(void) putc(':', stderr);
			}
			group = 0;
		}

		switch (toktype) {
		case '(':
			if ( ! ( lex_compat || posix_compat) )
				group = _sf_top_ix;
		case '<':
		case '>':
		case '^':
		case '$':
		case '"':
		case '[':
		case ']':
		case '{':
		case '}':
		case '|':
		case ')':
		case '-':
		case '/':
		case '\\':
		case '?':
		case '.':
		case '*':
		case '+':
		case ',':
			(void) putc (toktype, stderr);
			break;

		case '\n':
			(void) putc ('\n', stderr);

			if (sectnum == 2)
				beglin = 1;

			break;

		case SCDECL:
			fputs ("%s", stderr);
			break;

		case XSCDECL:
			fputs ("%x", stderr);
			break;

		case SECTEND:
			fputs ("%%\n", stderr);

			/* We set beglin to be true so we'll start
			 * writing out numbers as we echo rules.
			 * flexscan() has already assigned sectnum.
			 */
			if (sectnum == 2)
				beglin = 1;

			break;

		case NAME:
			fprintf (stderr, "'%s'", nmstr);
			break;

		case CHAR:
			switch (yylval.number) {
			case '<':
			case '>':
			case '^':
			case '$':
			case '"':
			case '[':
			case ']':
			case '{':
			case '}':
			case '|':
			case '(':
			case ')':
			case '-':
			case '/':
			case '\\':
			case '?':
			case '.':
			case '*':
			case '+':
			case ',':
				fprintf (stderr, "\\%c", yylval.number);
				break;

			default:
				if (!isascii (yylval.number) || !isprint (yylval.number))
					fprintf (stderr,
						 "\\%.3o",
						 (unsigned int) yylval.number);
				else
					(void) putc (yylval.number, stderr);
				break;
			}

			break;

		case NUMBER:
			fprintf (stderr, "%d", yylval.number);
			break;

		case EOF_OP:
			fprintf (stderr, "<<EOF>>");
			break;

		case OPTION_OP:
			fprintf (stderr, "%s ", yytext);
			break;

		case OPT_OUTFILE:
		case OPT_PREFIX:
		case CCE_ALNUM:
		case CCE_ALPHA:
		case CCE_BLANK:
		case CCE_CNTRL:
		case CCE_DIGIT:
		case CCE_GRAPH:
		case CCE_LOWER:
		case CCE_PRINT:
		case CCE_PUNCT:
		case CCE_SPACE:
		case CCE_UPPER:
		case CCE_XDIGIT:
		case CCE_NEG_ALNUM:
		case CCE_NEG_ALPHA:
		case CCE_NEG_BLANK:
		case CCE_NEG_CNTRL:
		case CCE_NEG_DIGIT:
		case CCE_NEG_GRAPH:
		case CCE_NEG_LOWER:
		case CCE_NEG_PRINT:
		case CCE_NEG_PUNCT:
		case CCE_NEG_SPACE:
		case CCE_NEG_UPPER:
		case CCE_NEG_XDIGIT:
			fprintf (stderr, "%s", yytext);
			break;

			case CCL_OP_UNION:
				fprintf (stderr, "{+}");
				break;

			case CCL_OP_DIFF:
				fprintf (stderr, "{-}");
				break;

		case 0:
			fprintf (stderr, _("End Marker\n"));
			break;

		default:
			fprintf (stderr,
				 _
				 ("*Something Weird* - tok: %d val: %d\n"),
				 toktype, yylval.number);
			break;
		}
	}

	return toktype;
}
