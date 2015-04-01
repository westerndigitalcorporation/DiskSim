
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


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <libddbg/libddbg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <unistd.h>
#include <fcntl.h>

#include <libgen.h>

#include "libparam.h"
#include "bitvector.h"

void dump_lp_block( const struct lp_block *b, char *name );


static lp_topoloader_t topoloader = 0;


void lp_register_topoloader(lp_topoloader_t l) 
{
  topoloader = l;
}




char *lp_builtin_names[] = {
  0,
  "Block",
  "String",
  "Int",
  "Double",
  "List", 
  "Topospec"
};

// FILE *libparamin = 0;
struct lp_subtype **lp_typetbl;
int lp_typetbl_len;

struct lp_tlt **lp_tlts;
int lp_tlts_len;

//extern char lp_filename[];
//extern char lp_cwd[];

char **lp_searchpath = 0;
int lp_searchpath_len = 0;


extern int lp_lineno;

static void destroy_block(struct lp_block *b);
static void destroy_value(struct lp_value *v);
static void destroy_param(struct lp_param *p);
static void destroy_list(struct lp_list *l);
static void destroy_topospec(struct lp_topospec *t);

int dumb_split(char *s, char **t, int *i);
int dumb_split2(char *s, char **s1, char **s2);


static int lp_max_mod = 0;
static int lp_mod_size = 0;
struct lp_mod **lp_modules = 0;

static char **overrides = 0;
static int overrides_len = 0;

#define outputfile stdout


struct lp_value *lp_new_intv(int i) 
{
  struct lp_value *v = calloc(1, sizeof(struct lp_value));
  v->v.i = i;
  v->t = I;
  return v;
}

struct lp_value *lp_new_doublev(double d) 
{
  struct lp_value *v = calloc(1, sizeof(struct lp_value));
  v->v.d = d;
  v->t = D;
  return v;
}

struct lp_value *lp_new_stringv(char *s) 
{
  struct lp_value *v = calloc(1, sizeof(struct lp_value));
  v->v.s = s;
  v->t = S;
  return v;
}

struct lp_value *lp_new_listv(struct lp_list *l)
{
  struct lp_value *v = calloc(1, sizeof(struct lp_value));
  v->v.l = l;
  v->t = LIST;
  return v;
}

struct lp_value *lp_new_blockv(struct lp_block *b)
{
  struct lp_value *v = calloc(1, sizeof(struct lp_value));
  v->v.b = b;
  v->t = b->type;
  return v;
}


struct lp_param *
lp_new_param(char *name, char *source, struct lp_value *v)
{
  struct lp_param *result = calloc(1, sizeof(struct lp_param));
  result->source_file = source;
  result->name = name;
  result->v = v;

  return result;
}

struct lp_list *
lp_new_list(void) 
{
  struct lp_list *l = calloc(1, sizeof(struct lp_list));
  l->values = calloc(8, sizeof(struct lp_value *));
  l->values_len = 8;
  l->values_pop = 0;
  l->linelen = 1;
  return l;
}

struct lp_block *
lp_new_block(void)
{
  struct lp_block *b = calloc(1, sizeof(struct lp_block));
  b->params = calloc(8, sizeof(struct lp_param *));
  b->params_len = 8;
  return b;
}

struct lp_list *
lp_new_intlist(int *intarr, int len)
{
  int i;
  struct lp_list *l = lp_new_list();

  for(i = 0; i < len; i++) {
    lp_list_add(l, lp_new_intv(intarr[i]));
  }
  return l;
}



int lp_register_module(struct lp_mod *m) {
  int c;
  for(c = 0; c < lp_max_mod; c++) {
    if(!strcmp(m->name, lp_modules[c]->name)) {
      return -1;
    }
  }
  if(c >= lp_mod_size) {
    lp_mod_size *= 2; lp_mod_size++;
    lp_modules = realloc(lp_modules, lp_mod_size * sizeof(int *));
  }
  if(c > 0) 
    lp_modules[c] = m;
  else 
    lp_modules[0] = m;

  lp_max_mod++;

  return c;
}




static void die(char *msg) {
  fprintf(stderr, "*** error: FATAL: %s\n", msg);
  exit(1);
}

static void destroy_param(struct lp_param *p)
{
  free(p->name);
  destroy_value(p->v);
  free(p);
  p = NULL;
}


static void destroy_list(struct lp_list *l) {
  int c;
  for(c = 0; c < l->values_len; c++)
    if(l->values[c])
      destroy_value(l->values[c]);

  free(l->values);
  free(l);
  l = NULL;
}


static void destroy_value(struct lp_value *v) {
  int d;
  switch(v->t) {
  case S:
    free(v->v.s); 
    break;
    
  case LIST:
    destroy_list(v->v.l);
    break;
    
  case I: 
  case D: break;

  case TOPOSPEC:
    for(d = 0; d < v->v.t.len; d++)
      destroy_topospec(&v->v.t.l[d]);

    free(v->v.t.l);
    break;
      
  default:
    destroy_block(v->v.b);
  }

  free(v);
  v = NULL;
}


static void destroy_block(struct lp_block *b) {
  int c;
  for(c = 0; c < b->params_len; c++) {
    if(b->params[c]) {
      destroy_param(b->params[c]);
    } 
  }

  free(b->name);
  free(b->params);
  free(b);
  b = NULL;
}

static void destroy_topospec(struct lp_topospec *t)
{
  free(t->type);
  free(t->name);
  destroy_list(t->l);
  t = NULL;
}


static struct lp_block *copy_block(const struct lp_block *);
static struct lp_list *copy_list(const struct lp_list *);
static struct lp_value *copy_value(const struct lp_value *);
static struct lp_param *copy_param(const struct lp_param *);

static struct lp_block *copy_block(const struct lp_block *b)
{
  int c;
  struct lp_block *result = calloc(1, sizeof(struct lp_block));
  memcpy(result, b, sizeof(struct lp_block));
  result->params = calloc(b->params_len, sizeof(result->params[0]));

  if(b->name) {
    result->name = strdup(b->name);
  }

  for(c = 0; c < b->params_len; c++) {
    if(b->params[c]) {
      result->params[c] = copy_param(b->params[c]);
    }
  }

  return result;
}


static struct lp_list *copy_list(const struct lp_list *l) {
  int c;
  struct lp_list *result = calloc(1, sizeof(*result));
  memcpy(result, l, sizeof(struct lp_list));
  result->values = calloc(l->values_len, sizeof(result->values[0]));
  memcpy(result->values, l->values, l->values_len * sizeof(int *));
  for(c = 0; c < l->values_len; c++) {
    if(l->values[c]) {
      result->values[c] = copy_value(l->values[c]);
    }
  }
  return result;
}


static struct lp_value *copy_value(const struct lp_value *v) {
  struct lp_value *result = calloc(1, sizeof(struct lp_value));
  memcpy(result, v, sizeof(struct lp_value));
  switch(v->t) {
  case S:
    result->v.s = strdup(v->v.s);
    break;

  case LIST:
    result->v.l = copy_list(v->v.l);
    break;

  case BLOCK:
    result->v.b = copy_block(v->v.b);
    break;

  default:
    break;
  }
  
  return result;
}

static struct lp_param *copy_param(const struct lp_param *p) {
  struct lp_param *result = calloc(1, sizeof(struct lp_param));
  result->name = strdup(p->name);
  result->v = copy_value(p->v);
  return result;
}


static int indent_level = 0;

static void indent(FILE *f) {
  char *space = calloc(1, 3*indent_level+1);
  memset(space, (int)' ', 3*indent_level);
  space[3*indent_level] = 0;
  fprintf(f, "%s", space);
  free(space);
}

void unparse_source(char *source, FILE *outfile) {
  fprintf(outfile, "source %s\n\n", source);
}

void unparse_type(int t, FILE *outfile) {
  if(t < 0) {
    fprintf(outfile, "%s", lp_builtin_names[abs(t)]);
  }
  else {
    if(t > lp_max_mod) {
      fprintf(outfile, "<UNKNOWN TYPE>");
    }
    else {
      fprintf(outfile, "%s", lp_modules[t]->name);
    }
  }
}

void unparse_param(struct lp_param *p, FILE *outfile) {
  fprintf(outfile, "%s = ", p->name);
  if(p->source_file && p->v->source_file) {
    if(strcmp(p->source_file, p->v->source_file)) {
      unparse_source(p->v->source_file, outfile);
      return;
    }
  }

  unparse_value(p->v, outfile);
}

void unparse_value(struct lp_value *v, FILE *outfile) {
  int c;
  switch(v->t) {
  case I:
    fprintf(outfile, "%d", v->v.i);
    break;
  case D:
    if(v->v.d == 0.0) {
      fprintf(outfile, "0.0");
    }
    else {
      fprintf(outfile, "%f", v->v.d);
    }
    break;
  case S:
    fprintf(outfile, "%s", v->v.s);
    break;
  case LIST:
    unparse_list(v->v.l, outfile);
    break;
  case TOPOSPEC:
    for(c = 0; c < v->v.t.len; c++)
      unparse_topospec(&v->v.t.l[c], outfile);
    break;
  default:
  case BLOCK:
    unparse_block(v->v.b, outfile);
    break;
  }
}

void unparse_list(struct lp_list *l, FILE *outfile) {
  int printed = 0;
  int c;

/*    indent(outfile); */

  fprintf(outfile, "[ ");
  indent_level++;
  for(c = 0; c < l->values_len; c++) { 
    if(!l->values[c]) 
      continue;

    if(c) {
      fprintf(outfile, ", ");
      if(!(c % l->linelen)) {
	fprintf(outfile, "\n");
	indent(outfile);
      }
    }
    else {
      fprintf(outfile, "\n");
      indent(outfile);
    }

    printed++;
    unparse_value(l->values[c], outfile);
  }
  indent_level--;

  if((c > 0) && printed) {
    fprintf(outfile, "\n");
    indent(outfile);
  }
  fprintf(outfile, "]");
}


void unparse_block(struct lp_block *b, FILE *outfile) {
  int c;
/*    indent(outfile); */
  if(b->name) {
    /* a block definition */
    fprintf(outfile, "%s %s {\n", lp_modules[b->type]->name, b->name);
  }
  else {
    /* a block value */
    fprintf(outfile, "%s {\n", lp_modules[b->type]->name);

  }

  indent_level++;
  for(c = 0; c < b->params_len; c++) {
    if(!b->params[c]) continue;

    if(c) fprintf(outfile, ",\n");

    unparse_param(b->params[c], outfile);
  }
  indent_level--;
  fprintf(outfile, "\n");
  indent(outfile);
  fprintf(outfile, "}");
  if(b->name) {
    fprintf(outfile, " # end of %s spec", b->name);
    fprintf(outfile, "\n\n");
  }
}

void unparse_topospec(struct lp_topospec *t, FILE *outfile) {
  fprintf(outfile, "%s %s ", t->type, t->name);
  unparse_list(t->l, outfile);
  // unparse_list() prints a trailing newline...
  //  fprintf(outputfile, "\n"); 
}

// somewhat of a hack
void unparse_tl_topospec(struct lp_topospec *t, FILE *outfile) 
{
  fprintf(outfile, "topospec ");
  unparse_topospec(t, outfile);
  fprintf(outfile, "\n\n");
}

void unparse_inst(struct lp_inst *i, FILE *outfile) {
  fprintf(outfile, "instantiate ");
  unparse_list(i->l, outfile);
  fprintf(outfile, " as %s\n\n", i->name);
}

void unparse_tlt(struct lp_tlt *tlt, FILE *outfile, char *infile) {

  if(tlt->source_file && infile) {
    if(strcmp(tlt->source_file, infile)) {
      unparse_source(tlt->source_file, outfile);
      return;
    }
  }
  
  switch(tlt->what) {
  case TLT_BLOCK: unparse_block(tlt->it.block, outfile); break;
  case TLT_TOPO:  unparse_tl_topospec(tlt->it.topo, outfile); break;
  case TLT_INST:  unparse_inst(tlt->it.inst, outfile); break;
  default:        ddbg_assert(0); break;
  };
  
}

void lp_unparse_tlts(struct lp_tlt **tlts, 
		     int tlts_len, 
		     FILE *outfile, 
		     char *infile) 
{
  int c;
  for(c = 0; c < tlts_len; c++) {
    if(tlts[c] != 0) {
      unparse_tlt(tlts[c], outfile, infile);
    }
  }
}



/* instantiate all the elements of <l> as <name> */
int lp_inst_list(struct lp_inst *i)
{
  int c;
  
  /*      unparse_block(spec, outputfile); */
  for(c = 0; c < i->l->values_len; c++) {
    if(!i->l->values[c]) continue;
    
    ddbg_assert3(i->l->values[c]->t == S, 
	       ("bad type for component %s", i->l->values[c]->v.s));
    
    lp_instantiate(i->l->values[c]->v.s, i->name);
  }
  
  return 0;
}

/* instantiate <targ> as <name> */
int *lp_instantiate(char *targ, char *name) {
  char *nametmp;
  struct lp_block *spec;
  int *obj;
    
/*    unparse_block(b, outputfile); */

  spec = lp_lookup_spec(name);
  ddbg_assert3(spec != 0, ("no such type %s.\n", name));

  // this is a bit of a hack; we swap the name of the component
  // being instantiated with the name of the module that's being
  // instantiated so that the loader function sees the target
  // name
  nametmp = spec->name;
  spec->name = targ;
  
  //  fprintf(stderr, "*** Instantiating %s as %s\n", targ, name);

  obj = lp_override_inst(spec, 
			 targ,
			 lp_modules[spec->type]->fn,
			 overrides, 
			 overrides_len);

  // swap the name back
  spec->name = nametmp;

  if(!obj) {
    return 0;
  }
  
  if(lp_modules[spec->type]->callback) {
    lp_modules[spec->type]->callback(lp_modules[spec->type]->ctx, obj);
  }

  return obj;
}

/* 0 on success, nonzero on error */
int check_types(struct lp_block *b) {
  int c = 0;

  /* we'll do all of the type checking here so that 
   * the per-mod loaders only have to sanitize values */

  for(c = 0; c < b->params_len; c++) {
    if(b->params[c]) {
      if(lp_param_name(b->type, b->params[c]->name) == -1) {

	fprintf(stderr, "*** warning: parameter %s not valid in context %s\n",
		b->params[c]->name, lp_modules[b->type]->name);
	
      }

      else if(lp_modules[b->type]->modvars[lp_param_name(b->type, b->params[c]->name)].type
	 != PTYPE(b->params[c])) {

	/* implicitly convert ints to doubles */
	if((lp_modules[b->type]->modvars[lp_param_name(b->type, b->params[c]->name)].type
	    == D)
	   && (PTYPE(b->params[c]) == I)) {
	  b->params[c]->v->t = D;
	  DVAL(b->params[c]) = (double) IVAL(b->params[c]);
	}
	  
	else { 
	  fprintf(stderr, "*** error: type error: %s::\"%s\" cannot take type ",
		  lp_modules[b->type]->name,
		  b->params[c]->name);
	  
/*  	  unparse_type(PTYPE(b->params[c]), stderr); */
	  fprintf(stderr, "\n");

	  die("check_types() failed");
	}
      }
      


      if(PTYPE(b->params[c]) >= BLOCK) {
	check_types(BVAL(b->params[c]));
      }
      else if(PTYPE(b->params[c]) == LIST) {
	int d;
	struct lp_list *l = LVAL(b->params[c]);
	for(d = 0; d < l->values_len; d++) {
	  if(l->values[d]) {
	    if(l->values[d]->t >= BLOCK) {
	      check_types(l->values[d]->v.b);
	    }
	  }
	}
      }

    }
  }
  return 0;
}

void load_block(struct lp_block *b) {
  int n;

  lp_lookup_type(b->name, &n);
  lp_typetbl[n]->spec = b;
}




void load_topo(struct lp_topospec *t, int len) 
{
/*    unparse_topospec(t, outputfile); */
  topoloader(t, len);
}





void printvars(void) {
  int c, d;
  for(c = 0; c < lp_max_mod; c++) {
    for(d = 0; d < lp_modules[c]->modvars_len; d++) {
      printf("%s::%s\n", lp_modules[c]->name, 
	     lp_modules[c]->modvars[d].name);
    }
  }
}



void dummy (struct lp_block *b) { 
  fprintf(stderr, "*** error: %s cannot be declared at top-level.\n",
	  b->name);
  exit(1);
}


struct lp_list *lp_list_add(struct lp_list *l, 
			    struct lp_value *v)
{
  int c, newlen;
  for(c = 0; c < l->values_len; c++) {
    if(!l->values[c]) goto done;
  }
  newlen = 2 * c * sizeof(struct lp_value *);
  l->values = realloc(l->values, newlen);
  bzero(&(l->values[c]), newlen / 2);
  l->values_len *= 2;

 done:
  l->values_pop++;
  l->values[c] = v;
  
  return l;
}





/* maps a modtype and parameter name to a nonnegative numeric
 * representation.  -1 returned on error */

int lp_param_name(int m, char *n)
{
  int c = 0;
  if(!RANGE(m,0,lp_max_mod)) return -1;
  if(!n) return -1;

  while((c < lp_modules[m]->modvars_len)
	&& strcmp(lp_modules[m]->modvars[c].name, n)) c++;

  if(c >= lp_modules[m]->modvars_len) {
    return -1;
  }
  else {
    return c;
  }
}


int lp_mod_name(char *n) {
  int c = 0;
  if(!n) return -1;

  while((c < lp_max_mod) && strcmp(lp_modules[c]->name, n)) c++;

  if(c >= lp_max_mod) {
    return -1;
  }
  else {
    return c;
  }
}

/* get the base type of name */
char *lp_lookup_base_type(char *name, int *n) {
  int c;
  for(c = 0; c < lp_typetbl_len; c++) {
    if(lp_typetbl[c]) {
      if(!strcmp(name, lp_typetbl[c]->sub)) {
	if(lp_typetbl[c]->super) {
	  return lp_lookup_base_type(lp_typetbl[c]->super, n);
	}
	else {
	  break;
	}
      }
    }
  }

  if(n) *n = c;
  return name;
}

char *lp_lookup_type(char *name, int *n) {
  int c;
  for(c = 0; c < lp_typetbl_len; c++) {
    if(lp_typetbl[c]) {
      if(!strcmp(name, lp_typetbl[c]->sub)) {
	if(n) *n = c;
	return lp_typetbl[c]->super;
      }
    }
  }
  return 0;
}



/* find the specification for name or return 0 if it doesn't exist.
 * New "wildcard" behavior: if name is null, return the first
 * spec in the typetbl with non-zero parent -- i.e. only 
 * match a user-provided spec, not a builtin type!
 */
struct lp_block *lp_lookup_spec(char *name) {
  int c;

  for(c = 0; c < lp_typetbl_len; c++) 
    if(lp_typetbl[c]) {
      if(!name) {
	if(lp_typetbl[c]->super) {
	  return lp_typetbl[c]->spec;
	}
      }
      else if(!strcmp(name, lp_typetbl[c]->sub)) {
	return lp_typetbl[c]->spec;
      }
    }

  return 0;
}


struct lp_tlt *lp_new_tl_topo(struct lp_topospec *t, char *source_file)
{
  struct lp_tlt *result = calloc(1, sizeof(*result));
  result->source_file = source_file;
  result->what = TLT_TOPO;
  result->it.topo = t;
  lp_add_tlt(result);
  return result;
}

struct lp_tlt *lp_new_tl_inst(struct lp_inst *i, char *source_file)
{
  struct lp_tlt *result = calloc(1, sizeof(*result));
  result->source_file = source_file;
  result->what = TLT_INST;
  result->it.inst = i;
  lp_add_tlt(result);
  return result;
}

struct lp_tlt *lp_new_tl_block(struct lp_block *b, char *source_file)
{
  struct lp_tlt *result = calloc(1, sizeof(*result));
  result->source_file = source_file;
  result->what = TLT_BLOCK;
  result->it.block = b;
  lp_add_tlt(result);
  return result;
}


void lp_add_tlt(struct lp_tlt *tlt) {
  int c;
  int found = 0;

  for(c = 0; c < lp_tlts_len; c++) {
    if(!lp_tlts[c]) {
      found = c; break;
    }
  }

  if(!found) {
    int newlen = lp_tlts_len ? 2 * lp_tlts_len : 2;
    int zerocnt = lp_tlts_len ? lp_tlts_len : 2;
    
    int newsize = newlen * sizeof(struct lp_tlts *);
    zerocnt *= sizeof(struct lp_tlts *);

    lp_tlts = realloc(lp_tlts, newsize);
    memset(lp_tlts + lp_tlts_len, 0, zerocnt);
    found = lp_tlts_len;
    lp_tlts_len = newlen;
  }

  lp_tlts[c] = tlt;
}


int lp_add_type(char *newt, char *parent) {
  int c;
  int newlen;
  struct lp_subtype *st = calloc(1, sizeof(*st));
  st->super = strdup(parent); st->sub = strdup(newt);

  if(!lp_lookup_type(newt,0)) {
    for(c = 0; c < lp_typetbl_len; c++) 
      if(!lp_typetbl[c]) {
	goto done;
      }


    newlen = lp_typetbl_len ? 2 * lp_typetbl_len : 2;

    lp_typetbl = realloc(lp_typetbl, newlen * sizeof(int *));

    bzero(&(lp_typetbl[c]), c * sizeof(int *));
    lp_typetbl_len = newlen;
  }
  else return -1;
  
  done:
  lp_typetbl[c] = st;
  return 0;
}


int lp_add_param(struct lp_param ***b, int *plen,
		 struct lp_param *p)
{
  int c;

  /* look for dupe params */
  for(c = 0; c < *plen; c++) {
    if(!(*b)[c]) continue;
    if(!strcmp((*b)[c]->name, p->name)) {
      fprintf(stderr, "*** error: redefined %s\n", p->name);
      return -1;
    }
  }


  for(c = 0; c < *plen; c++) {
    if(!(*b)[c]) {
      (*b)[c] = p;
      break;
    }
  }
  if(c == *plen) {
    /* didn't find a free slot -- double the array */
    int newlen = 2 * (*plen) + 1;
    (*b) = realloc((*b), newlen * sizeof(int *));
    bzero(&((*b)[*plen]), ((*plen) + 1) * sizeof(int*));
    (*b)[(*plen)] = p;
    *plen = newlen;
  }

  return 0;
}

/* copy all of the params in parent not defined in child into
 * child */
int lp_setup_subtype(struct lp_block *parent,
		     struct lp_block *child)
{
  int c, d;

  for(c = 0; c < parent->params_len; c++) {
    int found = 0;
    for(d = 0; d < child->params_len; d++) {
      if(!strcmp(child->params[d]->name, parent->params[c]->name)) {
	found = 1;
      }
    }
    if(!found) {
      lp_add_param(&child->params, &child->params_len, parent->params[c]);
    }
  }

  return 0;
}




void lp_init_typetbl(void) {
  int c;
  lp_typetbl = calloc(lp_max_mod, sizeof(struct subtype *));


  for(c = 0; c < lp_max_mod; c++) {

    lp_typetbl[c] = calloc(1, sizeof(struct lp_subtype));
    //    bzero(lp_typetbl[c], sizeof(struct lp_subtype));
    lp_typetbl[c]->sub = strdup(lp_modules[c]->name);
  }

  lp_typetbl_len = lp_max_mod;
}

void lp_release_typetbl(void) {
  int c;

  for(c = 0; c < lp_typetbl_len; c++) {
    if(!lp_typetbl[c]) continue;

    if(lp_typetbl[c]->spec)
      destroy_block(lp_typetbl[c]->spec);

    if(lp_typetbl[c]->sub)
      free(lp_typetbl[c]->sub);

    if(lp_typetbl[c]->super) 
      free(lp_typetbl[c]->super); 

    free(lp_typetbl[c]);
  }

  free(lp_typetbl);
  lp_typetbl = NULL;
}



/* splits s into a trailing number (i) and the leading part */
int dumb_split(char *s, char **t, int *i) {
  int c = strlen(s) - 1;
  
  while(c && isdigit(s[c])) c--;
  if(!c) return -1;
  c++;
  (*i) = atoi(&s[c]);
  *t = strdup(s);
  (*t)[c] = 0;
  return 0;
}

/* separate 'foo:bar' into 'foo' and 'bar' */
int dumb_split2(char *s, char **s1, char **s2) {
  int c = 0;
  (*s1) = strdup(s);
  while(s[c] && (s[c] != ':')) c++;
  (*s1)[c] = 0;
  if(s[c]) (*s2) = strdup(s + c + 1);
  else return -1;

  return 0;
}





static int param_override(struct lp_block *b, 
			  char *pname,
			  char *pval) 
{
  int c;
  
  for(c = 0; c < b->params_len; c++) {
    if(!b->params[c]) continue;
    if(!strcmp(b->params[c]->name, pname)) {
      switch(b->params[c]->v->t) {
      case I:
	b->params[c]->v->v.i = atoi(pval);
	break;
      case D:
	b->params[c]->v->v.d = atof(pval);
	break;
      case S:
	free(b->params[c]->v->v.s);
	b->params[c]->v->v.s = strdup(pval);
	return 0;
	break;

      default:
	ddbg_assert(0);
      }

      return -1;
    } 
  }

  
  return -1;
}

			  

int range_match(char *range, char *name) {
  char *base1, *base2, *base3;
  char r1[128], r2[128];
  int i1, i2, i3;

  if(!strcmp(range, name)) { return 1; }

  if(!strcmp(range, "*")) return 1;

  if(sscanf(range, "%s .. %s", r1, r2) != 2) { 
    char *prefix; int junk;

    dumb_split(name, &prefix, &junk);

    /* i.e. driver* matches driver2 and driver and driver2344 but not
     * driverqux 
     */
    if((strlen(range) == ( strlen(prefix) + 1)) &&
       !strncmp(range,prefix,strlen(prefix)) &&
       (range[strlen(prefix)] == '*')) return 1;

    return 0; 
  }

  
  dumb_split(r1, &base1, &i1);
  dumb_split(r2, &base2, &i2);
  if(strcmp(base1, base2) || (i1 < 0) || (i2 < i1)) {
    fprintf(stderr, "*** error: bad range \"%s .. %s\"\n", r1, r2);
    return 0;
  }

  dumb_split(name, &base3, &i3);
  if(!strcmp(base3, base2) && (i3 >= i1) && (i3 <= i2)) {
    return 1;
  }
  else {
    return 0;
  }
}


/* instantiate a component with overrides 
 * tname type to instantiate,
 * cname name of instantiated component
 * loader is the module loader function for tname
 */
int *lp_override_inst(const struct lp_block *spec, 
		  char *cname, 
		  lp_modloader_t loader,
		  char **overrides,
		  int overrides_len)
{
  int c, d;
  struct lp_block *spec_copy = NULL;
  char *p1, *p2; 
  int *result;

#if 0
  dump_lp_block( spec, "lp_override_inst" );
#endif

  spec_copy = copy_block(spec);

  for(c = 0; c < overrides_len; c += 3) {
    if(range_match(overrides[c], cname)) {
      
      /* overrides[c+2] could be an int, a real, a string or a list or
       * a block.  need parser to deal with lists and blocks
       * reasonably so we aren't going to deal with them here */
     
      if(!dumb_split2(overrides[c+1], &p1, &p2)) {
	/* descend hierarchy */
	for(d = 0; d < spec_copy->params_len; d++) {
	  if(!spec_copy->params[d]) continue;
	  if(!strcmp(p1, spec_copy->params[d]->name)) {
	    if(spec_copy->params[d]->v->t != BLOCK) {
	      fprintf(stderr, "*** error: tried to recurse through non-block parameter.\n");
	      return 0;
	    }
	    else {
	      param_override(spec_copy->params[d]->v->v.b, p2, 
			     overrides[c+2]);
	    }
	  }
	}

      }
      else {
	param_override(spec_copy, 
		       overrides[c+1], 
		       overrides[c+2]);
	
      }
    }
  }



  if(!check_types(spec_copy)) {
    result = loader(spec_copy, 0);
  }

  // XXX don't leak (segfaults)
  //  destroy_block(spec_copy);
  return result; 
}


extern void libparamparse(void);

int lp_loadfile(FILE *in, 
		struct lp_tlt ***tlts, 
		int *tlts_len, 
		char *infile,
		char **cli_overrides,
		int cli_overrides_len) {

  char *lp_sp;
  char *dirc;
  char *dir;
  
  int stdout_save;
  int devnull;
  top_file = 0;
  lp_lineno = 1;

  dirc = strdup(infile);
  dir = dirname(dirc);

  lp_filename = infile;
  lp_cwd = dir;
  libparamin = in;

  overrides = cli_overrides;
  overrides_len = cli_overrides_len;

  // do the searchpath
  lp_sp = getenv("LP_PATH");
  if(lp_sp) {
    char *p = lp_sp;
    char *colon;
    
    lp_searchpath = calloc(LP_MAX_SP, sizeof(char*));

    while(*p) {
      colon = strchr(p, ':');
      if(colon) {
	char *next;

  	*colon = 0;

	next = colon+1;
	lp_searchpath[lp_searchpath_len++] = strdup(p);

  	p = next;
      }
      else {
	lp_searchpath[lp_searchpath_len++] = strdup(p);
	break;
      }
    }
  }

  // this creates a fresh array every time its run so caller 
  // is responsible for freeing it
  lp_tlts = 0;
  lp_tlts_len = 0;
  
  fflush(stdout);
  stdout_save = dup(1);
  devnull = open("/dev/null", O_RDONLY);
  dup2(devnull, 1);

  libparamparse();

  fflush(stdout);
  dup2(stdout_save, 1);

  close(devnull);
  close(stdout_save);


  if(tlts) {
    *tlts = lp_tlts;
  }
  if(tlts_len) {
    *tlts_len = lp_tlts_len;
  }

  return 0;
}



#define LP_STACK_MAX 32
int
lp_loadparams(void *it, struct lp_block *b, struct lp_mod *m) {
  int c;
  int needed = 0;
  int param_stack[LP_STACK_MAX];
  int stack_ptr = 0;  // index of first free slot
  // XXX not static size
  char *paramvec = calloc(m->modvars_len, sizeof(char));


  // This is pretty gross; there should be a better solution.
  // dirname() munges its operand.  Its result may also be a static
  // buffer somewhere, hence the 2 copies.
  char *tmp = strdup(b->source_file);
  lp_cwd = strdup(dirname(tmp));
  //  free(tmp);
 
  for(c = 0; c < b->params_len; c++) {
    int pnum, deps;
    
    if(!b->params[c]) continue;
    
  TOP:
    pnum = lp_param_name(lp_mod_name(m->name), b->params[c]->name);
    
    // Don't initialize things more than once.
    // Should warn here, probably.
    if(BIT_TEST(paramvec, pnum)) continue;
    
    
    if(stack_ptr > 0) {
      for(c = 0; c < b->params_len; c++) {
	if(lp_param_name(lp_mod_name(m->name), b->params[c]->name) == needed)
	  goto FOUND;
      }
      break;
    }


  FOUND:
  
    deps = m->param_deps[pnum](paramvec);
    if(deps > -1) {
      ddbg_assert(stack_ptr < LP_STACK_MAX);
      param_stack[stack_ptr++] = c;
      needed = deps;
      continue;
    }
    else {
      switch(PTYPE(b->params[c])) {
      case I:     
	((lp_paramloader_int)m->param_loaders[pnum])(it, IVAL(b->params[c])); 
	break; 
      case D:     
	((lp_paramloader_double)m->param_loaders[pnum])(it, DVAL(b->params[c])); 
	break; 
      case S:     
	((lp_paramloader_string)m->param_loaders[pnum])(it, SVAL(b->params[c])); 
	break; 
      case LIST:  
	((lp_paramloader_list)m->param_loaders[pnum])(it, LVAL(b->params[c])); 
	 break; 
	 
      case BLOCK:
      default:    
	((lp_paramloader_block)m->param_loaders[pnum])(it, BVAL(b->params[c])); 
	break; 
      }
    }

    BIT_SET(paramvec, pnum);
    if(stack_ptr > 0) { 
      c = param_stack[--stack_ptr]; 
      goto TOP;  
    }
  }

  for(c = 0; c < m->modvars_len; c++) {
    if(m->modvars[c].req && !BIT_TEST(paramvec,c)) {
      fprintf(stderr, "*** error: in %s spec -- missing required parameter %s\n", m->name, m->modvars[c].name);
      break;
    }
  }

  free(paramvec);
  paramvec = NULL;

  return 0; // ???
}


char *
lp_search_path(char *cwd, char *name)
{
  char *cand = calloc(LP_PATH_MAX, sizeof(char));
  struct stat s;
  int i;
  
  if(name[0] == '/')
    if(stat(name, &s))
      goto fail;

  snprintf(cand, LP_PATH_MAX, "%s/%s", cwd, name);

  if(!stat(cand, &s))
    goto succ;
    
  for(i = 0; i < lp_searchpath_len; i++) {
    snprintf(cand, LP_PATH_MAX, 
	     "%s/%s", lp_searchpath[i], name);

    if(!stat(cand, &s)) {
      goto succ;
    }
  }

 fail:
  free(cand);
  return 0;

 succ:
  return cand;

}

/*
typedef enum {
  BLOCK = -1,
  S     = -2,
  I     = -3,
  D     = -4,
  LIST  = -5,
  TOPOSPEC = -6,
  UV    = -7
} lp_type_t;
*/
void dump_lp_block( const struct lp_block *b, char *name )
{
    int paramsIndex;
    struct lp_param *param;

    printf( "\n\nlp_block: %s", name );
    printf( "\n  source file: %s",  b->source_file );
    printf( "\n  name       : %s",  b->name );
    printf( "\n  loader     : %p",  b->loader );
    printf( "\n  params_len : %d",  b->params_len );
    printf( "\n    params   : %p", b->params );
    for( paramsIndex = 0; paramsIndex < b->params_len; paramsIndex++ )
    {
        param = b->params[paramsIndex];
        if( NULL == param )
            break;
        printf( "\n      index      : %d", paramsIndex );
        printf( "\n      source file: %s", b->params[paramsIndex]->source_file );
        printf( "\n      name       : %s", b->params[paramsIndex]->name );
    }
    fflush( stdout );
}

// End of file

