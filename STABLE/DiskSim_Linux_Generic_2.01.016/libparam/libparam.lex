
/* libparam (version 1.0)
 * Authors: John Bucy, Greg Ganger
 * Contributors: John Griffin, Jiri Schindler, Steve Schlosser
 *
 * Copyright (c) of Carnegie Mellon University, 2001-2008.
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this
 * software, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to reproduce, use, and prepare derivative works of this
 * software is granted provided the copyright and "No Warranty"
 * statements are included with all reproductions and derivative works
 * and associated documentation. This software may also be
 * redistributed without charge provided that the copyright and "No
 * Warranty" statements are included in all redistributions.
 *
 * NO WARRANTY. THIS SOFTWARE IS FURNISHED ON AN "AS IS" BASIS.
 * CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER
 * EXPRESSED OR IMPLIED AS TO THE MATTER INCLUDING, BUT NOT LIMITED
 * TO: WARRANTY OF FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY
 * OF RESULTS OR RESULTS OBTAINED FROM USE OF THIS SOFTWARE. CARNEGIE
 * MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF ANY KIND WITH
 * RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
 * INFRINGEMENT.  COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY USE
 * OF THIS SOFTWARE OR DOCUMENTATION.  
 */



%x src

%{
#include <string.h>
#include <libgen.h> // dirname(), basename()
#include <libddbg/libddbg.h>
#include <errno.h>

#include "libparam.h"
#include "libparam.tab.h"

#define MAX_INPUT_FILES 32
  int top_file = 0;
  struct lp_input_file input_files[MAX_INPUT_FILES];
  int lp_lineno = 0;


  /* stack for pathnames recursed */

  char *paths[MAX_INPUT_FILES];
  int top_path = 0;
  char *lp_cwd;
  char *lp_filename;
%}

%option noyywrap
%option yylineno

source source
semi ;
lbrace \{
rbrace \}
lbrak \[
rbrak \]
comma ,
decint -?[0-9]+ 
hexint -?0x[0-9a-fA-F]+
float -?[0-9]+\.[0-9]+(e[+-][0-9]+)?
equal =
qu \?
colon :
string [^ \t\n\f;{},\[\]:\?=]+
comment #[^\n\f]*
newline [\r\n]
whitespace [ \t]+
dotdot \.\.
topology topology
instantiate instantiate
as as
%%

{whitespace} ; 
{newline} { lp_lineno++; };

{semi}   { return SEMI; };
{lbrak}  { return LBRAK; };
{rbrak}  { return RBRAK; };
{comma}  { return COMMA; };
{equal}  { return EQUAL; };
{lbrace} { return LBRACE; };
{rbrace} { return RBRACE; };
{comment} ;
{qu} { return QU; }
{colon} { return COLON; }
{dotdot} { return DOTDOT; }
{source} BEGIN(src);

{topology} { return TOPOLOGY; }
{instantiate} { return INSTANTIATE; }
{as} { return AS; }

{decint} { 
  libparamlval.i = atoi(yytext); return DECINT; 
};

{hexint} { 
  libparamlval.i = strtol(yytext, 0, 16); return HEXINT; 
};

{float} { 
  libparamlval.d = strtod(yytext, 0); 
  return FLOAT; 
};

{string} {
/*    printf("STRING: \"%s\"\n", yytext);  */
  libparamlval.s = strdup(yytext); return STRING; 
};

<src>{string} {
  char *dirc, *basec;
  char *dir, *base;

  // NEVER modify lp_filename; it is shared between all objects that 
  // came from that file.

  input_files[top_file].b = YY_CURRENT_BUFFER;
  input_files[top_file].lineno = lp_lineno;
  input_files[top_file].filename = lp_filename;
  top_file++;

  lp_filename = strdup(yytext);

  dirc = strdup(lp_filename);
  basec = strdup(lp_filename);
  dir = dirname(dirc);
  base = basename(basec);

  paths[top_path++] = lp_cwd;


  // XXX move all of this logic into util.c

  // we maintain a current directory here (cwd) which is what we search
  // relative to.

  // if the dir is ".", leave cwd alone -- keep the previous value
  if(strcmp(dir, ".")) {
    // if the dir is an absolute path, replace cwd with it
    if(dir[0] == '/') {
      lp_cwd = dir;
    }
    // otherwise, append it to the current cwd
    else {
      char *new_cwd = calloc(LP_PATH_MAX, sizeof(char));
      snprintf(new_cwd, LP_PATH_MAX, "%s/%s", lp_cwd, dir);
      lp_cwd = new_cwd;
    }
  }


  // now, try to find the file.
  // if the path is absolute and it doesn't exist, fail.
  // Otherwise: first try appending the base name to the cwd.
  // If that doesn't exist, loop through lp_searchpath and try
  // p[i] . dir . base for each p[i] in the searchpath

  lp_filename = calloc(LP_PATH_MAX, sizeof(char));
  {
    char *path = lp_search_path(lp_cwd, yytext);
    if(path) {
      char *pathtmp = strdup(path);
      lp_cwd = strdup(dirname(pathtmp));
      free(pathtmp);

      lp_filename = path;

      yyin = fopen(lp_filename, "r");
      ddbg_assert3(yyin != 0, ("Couldn't open %s : %s", lp_filename, strerror(errno)));
      goto cont;    
    }
    else {
      fprintf(stderr, "goto FAIL %s %s", lp_cwd, yytext);
      goto fail;
    }
  }
  

 fail:
    fprintf(stderr, "*** error: couldn't open %s : %s\n", yytext, strerror(errno));
    yyterminate();

 cont:
    ddbg_assert(yyin != 0);
    yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
    lp_lineno = 0;

  BEGIN(INITIAL);
}

<<EOF>> {

    /* printf("lexer got EOF\n"); */

  if(--top_file < 0) { yyterminate(); }
  else {
    top_path--;
    lp_cwd = paths[top_path];

    fclose(yyin);
    libparam_delete_buffer(YY_CURRENT_BUFFER);
    libparam_switch_to_buffer(input_files[top_file].b);

    lp_lineno = input_files[top_file].lineno;

/*      free(lp_filename); */
    lp_filename = input_files[top_file].filename;
  }
}
%%








