/*
 * This file is part of flex.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the University nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.
 */

%{
/* A template scanner file to build "scanner.c". */
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
/*#include "parser.h" */

#if 0
#define err_abort() do{printf("ERROR: flex line %d. input line %d.\n", __LINE__, yylineno); abort();} while(0)
#define ok(message) do{printf("OK: %s, flex line %d. input line %d.\n", message, __LINE__, yylineno);}while(0)
#else
#define err_abort()
#define ok(message)
#endif

int utf8matches = 0; const int utf8target = 11141120; 
int nonutf8matches = 0; const int nonutf8target = 16;

%}

%option 8bit outfile="scanner.c" prefix="test"
%option nounput nomain noyywrap 
%option warn


%%

(?us:.) 			ok("UTF-8 match"); utf8matches++;
(?s-u:.)			ok("Not a UTF-8 match"); nonutf8matches++;

%%

int main(void);

int
main ()
{
	yyin = stdin;
	yyout = stdout;
	yylex();
	
	if(utf8matches == utf8target && nonutf8matches == nonutf8target){
		printf("TEST RETURNING OK.\n");
		return 0;
	} else {
		printf("TEST FAILED: %i/%i UTF-8 tests matched; %i/%i non UTF-8\n",
		utf8matches,utf8target,nonutf8matches,nonutf8target);
		return 1;
	}
}

/* vim:set noexpandtab cindent tabstop=4 softtabstop=4 shiftwidth=4 textwidth=0: */
