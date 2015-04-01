
/* diskmodel (version 1.0)
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



#include "dm.h"
#include "marshal.h"


struct dm_disk_if *
dm_unmarshal(struct dm_marshal_hdr *h, int bufflen) {
  
  struct dm_disk_if *result;

  /* malformed */
  if(bufflen < sizeof(struct dm_marshal_hdr)) return 0;
  if(h->len < bufflen) return 0;
  // don't know what this is
  if(h->type != DM_DISK_TYP) return 0;

  dm_marshal_mods[DM_DISK_TYP]->unmarshal(h, (void **)&result, 0);
  return result;
}


char *
dm_unmarshal_disk(struct dm_marshal_hdr *h, 
		   void **result,
		   void *parent)
{
  char *ptr = (char *)h;
  struct dm_marshal_hdr *hdrtmp;
  struct dm_disk_if *d = malloc(sizeof(*d));
  
  ptr += sizeof(struct dm_marshal_hdr);

  memcpy((char *)d, ptr, sizeof(struct dm_disk_if));
  ptr += sizeof(struct dm_disk_if);

  hdrtmp = (struct dm_marshal_hdr *)ptr;

  switch(hdrtmp->type) {
  case DM_LAYOUT_G1_TYP:
    ptr = dm_marshal_mods[DM_LAYOUT_G1_TYP]->unmarshal((void *)ptr,
							 (void **)&d->layout, 
							 d);
    break;
    
  default:
    ddbg_assert(0);
    break;
  }

  ptr = dm_marshal_mods[DM_MECH_G1_TYP]->unmarshal((void *)ptr, 
						     (void **)&d->mech, 
						     d);

  *result = d;
  return ptr;
}


struct dm_marshal_hdr *
dm_marshal(struct dm_disk_if *d) {
  struct dm_marshal_hdr *result = 0;
  char *ptr = 0;
  int alloc_size = 0;


  alloc_size = sizeof(struct dm_disk_if);
  alloc_size += sizeof(struct dm_marshal_hdr);

  alloc_size += d->layout->dm_marshaled_len(d);
  alloc_size += sizeof(struct dm_marshal_hdr);

  alloc_size += d->mech->dm_marshaled_len(d);
  alloc_size += sizeof(struct dm_marshal_hdr);

  result = malloc(alloc_size);

  result->type = DM_DISK_TYP;
  result->len = alloc_size;

  ptr = (char *)result + sizeof(struct dm_marshal_hdr);
  memcpy(ptr, (char *)d, sizeof(struct dm_disk_if));
  // no fn ptrs in disk struct
  ptr += sizeof(struct dm_disk_if);
  ptr = d->layout->dm_marshal(d, ptr);
  d->mech->dm_marshal(d, ptr);

  

  return result;
}

struct dm_marshal_module dm_disk_marshal_mod = 
{ dm_unmarshal_disk, 0, 0 };

struct dm_marshal_module *dm_marshal_mods[] = {
  &dm_disk_marshal_mod,
  &dm_layout_g1_marshal_mod,
  &dm_mech_g1_marshal_mod,
};


// functions to marshal and unmarshal function pointers

// note: sizeof(struct marshaled_fn) must not be larger than
// sizeof(void*)
struct marshaled_fn {
  uint16_t typ;
  uint16_t code;
};

void marshal_fn(void *fn, int typ, struct marshaled_fn *result) {
  int c;

  result->typ = typ;

  for(c = 0; c < dm_marshal_mods[typ]->fn_table_len; c++) {
    if(dm_marshal_mods[typ]->fn_table[c] == fn) {
      result->code = c;
      return;
    }
  }

  ddbg_assert(0);
}

void *unmarshal_fn(int *buff, int typ) {
  struct marshaled_fn *dmf = (struct marshaled_fn *)buff;
  return dm_marshal_mods[typ]->fn_table[dmf->code];
}


// fns is an array containing fns_len function pointers
// b is a buffer that you want to marshal the function pointers into
// typ is which module these functions belong to
void
marshal_fns(void **fns, int fns_len, char *b, int typ) {
  int *buff = (int *)b;
  int c;

  for(c = 0; c < fns_len; c++) {
    marshal_fn(fns[c], typ, (struct marshaled_fn *)buff); 
    buff++;
  }
}

// unmarshal fns_len functions from buff for module type into fns
void
unmarshal_fns(void **fns, int fns_len, char *b, int typ) {
  int *buff = (int *)b;
  int c;

  for(c = 0; c < fns_len; c++) {
    fns[c] = unmarshal_fn(buff, typ); 
    buff++;
  }
}


