
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


%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libddbg/libddbg.h>
#include "libparam.h"

#define YYERROR_VERBOSE

void yyerror (char *s) {
  fprintf(stderr, "*** error: on line %d of %s: ", lp_lineno, lp_filename);
  fprintf(stderr, "%s\n", s);
}

%}

%token LBRACE RBRACE SEMI EQUAL TRUE FALSE DECINT HEXINT FLOAT STRING
%token LBRAK RBRAK COMMA QU COLON NEWLINE SOURCE DOTDOT 

/* these are all keywords */
%token AS INSTANTIATE TOPOLOGY


%union {
  double d;
  long i;
  char *s;

  struct lp_block *b;
  struct { struct lp_block **b; int blen; } blocks;

  struct lp_param *p;
  struct { struct lp_param **p; int plen; } params;

  struct lp_list *l;

  struct lp_value *v;

  struct { struct lp_topospec *l; int len; } topo;
  struct lp_inst *inst;
}

%type <b> BLOCKDEF BLOCKVAL
%type <blocks> BLOCKS

%type <d> FLOAT
%type <i> HEXINT DECINT NUM 
%type <s> STRING
%type <p> PARAM

%type <p> EXP
%type <params> EXPS
%type <l> LIST LITEMS
%type <v> VALUE
%type <topo> TOPOSPEC
%type <inst> INST

%%


BLOCKS: { }
| BLOCKS TOPOLOGY TOPOSPEC {
/*      printf("blocks topospec\n"); */

  lp_new_tl_topo($3.l, lp_filename);  
  load_topo($3.l, $3.len);
}
| BLOCKS BLOCKDEF 
{ 
/*    printf("blocks blockdef\n"); */
  lp_new_tl_block($2, lp_filename);
  load_block($2); 
}
/*  | BLOCKS INSTANTIATE LIST AS STRING { */
| BLOCKS INST {   
  lp_new_tl_inst($2, lp_filename);
  lp_inst_list($2);
}
/* | STRING EQUAL BLOCKVAL {
 * printf("global var assignment %s\n", $1);
 * globalvar($1, $3);
 * } 
 */
;

INST: INSTANTIATE LIST AS STRING {
  $$ = calloc(1, sizeof(struct lp_inst));
  $$->source_file = lp_filename;
  $$->l = $2;
  $$->name = $4;
};

TOPOSPEC: STRING STRING LIST {
/*    printf("topospec\n"); */

  $$.l = calloc(1, sizeof(struct lp_topospec));
  $$.l->source_file = lp_filename;
  $$.len = 1;
  $$.l[0].type = $1;
  $$.l[0].name = $2;
  $$.l[0].l = $3;
}
/*  | STRING LIST { */
/*    int c; */
/*    int c2 = 0; */
/*    $$.l = malloc($2->values_len * sizeof(struct lp_topospec)); */
/*    bzero($$.l, $2->values_len * sizeof(struct lp_topospec)); */
/*    $$.len = $2->values_len; */
/*    if($2) for(c = 0; c < $2->values_len; c++) { */
/*      if(!($2->values[c])) continue; */
/*      $$.l[c2].name = $2->values[c]->v.s; */
/*      $$.l[c2].type = $1; */
/*      $$.l[c2].l = 0; */
/*      c2++; */
/*    } */
  /* else ... better be a device */
/*  } */
;



BLOCKDEF: STRING STRING LBRACE EXPS RBRACE
{
  struct lp_block *parent;

/*    printf("start of blockdef: %s\n", $2); */

  $$ = calloc(1, sizeof(struct lp_block));
  $$->source_file = lp_filename;

  $$->type = lp_mod_name(lp_lookup_base_type($1,0));
  if($$->type == -1) {
    fprintf(stderr, "*** error: Unknown block type: %s\n", $1);
    YYABORT;
  }

  if(lp_lookup_type($2,0)) {
    fprintf(stderr, "*** warning: ignoring redefinition of %s\n", $2);
  }
  else {
    parent = lp_lookup_spec($1);

    $$->name = $2;
    $$->params = $4.p;
    $$->params_len = $4.plen;
    if(lp_add_type($2, $1)) {
      fprintf(stderr, "*** error: couldn't add type %s::%s\n", $1, $2);
    }
    
    if(parent) lp_setup_subtype(parent, $$);
  }
  
  free($1);
};



BLOCKVAL: STRING LBRACE EXPS RBRACE {
  $$ = calloc(1, sizeof(struct lp_block));
  $$->source_file = lp_filename;
  $$->name = 0;
  $$->params = $3.p;
  $$->params_len = $3.plen;
  $$->type = lp_mod_name($1);

  if($$->type == -1) {
    fprintf(stderr, "*** error: no such module %s\n", $1);
    YYABORT;
  }

  $$->loader = lp_modules[$$->type]->fn;

  free($1);
};



EXPS: { $$.p = 0; $$.plen = 0; }
| EXP { 
/*      printf("first exp\n");  */
  $$.p = calloc(1, 8 * sizeof(int *)); 
  $$.plen = 8; 
  $$.p[0] = $1;
}
| LIST EQUAL VALUE {

  int c; struct lp_param *tmp;

/*    printf("another exp\n");  */

  $$.p = calloc(1, $1->values_len * sizeof(int *));
  $$.plen = $1->values_len;

  for(c = 0; c < $1->values_len; c++) {
    if(!$1->values[c]) continue;
    ddbg_assert($1->values[c]->t == S);
    tmp = calloc(1, sizeof(struct lp_param));
    tmp->name = $1->values[c]->v.s;
    tmp->v = $3;
    if(lp_add_param(&($$.p), &($$.plen), tmp)) YYABORT;
  }

}
| EXPS COMMA LIST EQUAL VALUE {
    int c; struct lp_param *tmp;

  for(c = 0; c < $3->values_len; c++) {
    if(!$3->values[c]) continue;
    ddbg_assert($3->values[c]->t == S);
    tmp = lp_new_param($3->values[c]->v.s, 0, $5);
/* malloc(sizeof(struct lp_param)); */
/*     tmp->name =  */
/*     tmp->v = $5; */
    if(lp_add_param(&($1.p), &($1.plen), tmp)) YYABORT;
  }
  $$ = $1;
}
| EXPS COMMA EXP
{ 

/*    printf("nth exp.\n"); */
  if(lp_add_param(&($1.p), &($1.plen), $3)) YYABORT;

  $$ = $1;
};



EXP: PARAM EQUAL VALUE {
    /* printf("got a param exp: %s (%s)\n", $1->name, $1->source_file); */
  
  $$ = $1;

/* malloc(sizeof(struct lp_param));  */
/*   $$->source_file = lp_filename; */
/*   $$->name = $1;  */
  $$->v = $3;  
}
;



NUM: DECINT
 | HEXINT
;


LIST: LBRAK LITEMS RBRAK
{
  $$ = $2;
};


LITEMS:  { 
  $$ = lp_new_list();
  /*  bzero($$, sizeof(struct lp_list)); */
  /*  $$->values_len = 0; */
  /*  $$ */
}
| VALUE {
/*    printf("first list item\n"); */
  $$ = lp_new_list();
/*   bzero($$, sizeof(struct lp_list)); */
/*   $$->values = malloc(8 * sizeof(int *)); */
/*   bzero($$->values, 8 * sizeof(int *)); */
/*   $$->values_len = 8; */
  $$->values_pop = 1; 
  $$->values[0] = $1; 
}
| STRING DOTDOT STRING {
  char *s1, *s2;
  char temp[1024];
  int i1, i2, n, c;
  if(dumb_split($1, &s1, &i1) || dumb_split($3, &s2, &i2)) {
    fprintf(stderr, "*** error: bad lex range\n");
    YYABORT;
  }
  /* done with these */
  free($1);
  free($3);

  if(strcmp(s1,s2)) {
    fprintf(stderr, "*** error: bad lex range\n");
    YYABORT;
  }
  n = i2 - i1 + 1;
  if(n <= 0) {
    fprintf(stderr, "*** error: bad lex range\n");
    YYABORT;
  }

  $$ = lp_new_list();
/*   bzero($$, sizeof(struct lp_list)); */
/*   $$->values_len = n; */
/*   $$->values = malloc(n * sizeof(int *)); */

  for(c = 0; c < n; c++) {
    struct lp_value *v;
    snprintf(temp, 1024, "%s%d", s2, c + i1);
/*     $$->values[c] = malloc(sizeof(struct lp_value)); */
/*     $$->values[c]->t = S; */
    v = lp_new_stringv(strdup(temp));
    //    $$->values[c]->v.s = strdup(temp);
    $$ = lp_list_add($$, v);
  }


}

| LITEMS COMMA STRING DOTDOT STRING {
  char *s1, *s2;
  char temp[1024];
  int i1, i2, n, c;
  struct lp_value *tmp;

  if(dumb_split($3, &s1, &i1) || dumb_split($5, &s2, &i2)) {
    fprintf(stderr, "*** error: bad lex range\n");
    YYABORT;
  }
  /* done with these */
  free($3);
  free($5);

  if(strcmp(s1,s2)) {
    fprintf(stderr, "*** error: bad lex range\n");
    YYABORT;
  }
  n = i2 - i1 + 1;
  if(n <= 0) {
    fprintf(stderr, "*** error: bad lex range\n");
    YYABORT;
  }

  for(c = 0; c < n; c++) {
    snprintf(temp, 1024, "%s%d", s2, c + i1);
    tmp = lp_new_stringv(strdup(temp));
/*       = malloc(sizeof(struct lp_value)); */
/*     tmp->t = S; */
/*     tmp->v.s = strdup(temp); */
    $$ = lp_list_add($1, tmp);
  }


}

| LITEMS COMMA VALUE {
/*    printf("list item\n"); */
  $$ = lp_list_add($1, $3);
};



VALUE: NUM {
  $$ = lp_new_intv($1);
  $$->source_file = lp_filename;
}|
 FLOAT {
  $$ = lp_new_doublev($1);
  $$->source_file = lp_filename;
}
| STRING {
   $$ = lp_new_stringv($1);
  $$->source_file = lp_filename;
}
| LIST {
  $$ = lp_new_listv($1);
  $$->source_file = lp_filename;
}
| BLOCKVAL {
  $$ = lp_new_blockv($1);
  $$->source_file = lp_filename;
  $$->t = BLOCK;
}
| BLOCKDEF { 
  $$ = calloc(1, sizeof(struct lp_value));
  $$->source_file = lp_filename;
  $$->v.b = $1;
  $$->t = BLOCK;
}
| TOPOSPEC {
/*    printf("topospec value\n"); */
  $$ = calloc(1, sizeof(struct lp_value));
  $$->source_file = lp_filename;
  $$->v.t.l = $1.l;
  $$->v.t.len = $1.len;

  $$->t = TOPOSPEC;
}
;



PARAM: STRING {
/*    printf("empty param\n");  */
  $$ = calloc(1, sizeof(struct lp_param));
  $$->name = $1;
  $$->source_file = lp_filename;
}
| PARAM STRING {
  char *tmp = calloc(2 * (strlen($1->name) + strlen($2)), sizeof(*tmp)); 

  $$ = $1;
  strcat(tmp, $1->name);
  strcat(tmp, " ");
  strcat(tmp, $2);

  free($$->name); free($2);

  $$->name = tmp;  
};



%%

/*  void main(void) { */

  //  printvars();
/*    disksim =  */
/*      disksim_initialize_disksim_structure(malloc(sizeof(struct disksim)), */
/*  					 sizeof(struct disksim)); */
/*    lp_init_typetbl(); */

/*    disksim->startaddr = malloc(1<<24); */
/*    disksim->curroffset = 0; */
/*    disksim->totallength = 1<<24; */

/*    top_file = 0; */
/*    input_files[0] = disksim_param_create_buffer(stdin, 1<<14); */

/*    yyparse(); */
/*  } */








