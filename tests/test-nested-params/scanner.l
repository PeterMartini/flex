/*
 * This file is part of flex.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#define err_abort() do{printf("ERROR: flex line %d. input line %d (%s).\n", __LINE__, yylineno, yytext); abort();} while(0)
#define ok(message) do{printf("OK: %s, flex line %d. input line %d.\n", message, __LINE__, yylineno);}while(0)

%}

%option 8bit outfile="scanner.c" prefix="test"
%option nounput nomain noyywrap 
%option warn


%%

(?i:"ONE LEVEL")                                   ok("One level matched");
(?i:"TWO "(?-i:"levels"))                          ok("Two levels matched");
(?i:"THREE "(?-i:"direct "(?i:"LEVELS")))          ok("Three direct levels matched");
(?i:"THREE "(?:"INDIRECT "(?-i:"levels")" 1"))     ok("Three indirect levels 1 matched");
(?i:"THREE "(?:"INDIRECT "(?:"LEVELS")" 2"))       ok("Three indirect levels 2 matched");
(?i:"THREE "(?-i:"indirect "(?:"LEVELS")" 3"))     err_abort(); /* This shouldn't match */
(?i:"THREE "(?-i:"indirect "(?:"levels")" 3"))     ok("Three indirect levels 3 matched");
(?-i:"three "(?:"INDIRECT "(?i:"LEVELS")" 4"))     err_abort(); /* This shouldn't match */
(?-i:"three "(?:"indirect "(?i:"LEVELS")" 4"))     ok("Three indirect levels 4 matched");
\n                                                 { }
.*                                                 err_abort();

%%

int main(void);

int
main ()
{
    yyin = stdin;
    yyout = stdout;
    yylex();
    printf("TEST RETURNING OK.\n");
    return 0;
}
