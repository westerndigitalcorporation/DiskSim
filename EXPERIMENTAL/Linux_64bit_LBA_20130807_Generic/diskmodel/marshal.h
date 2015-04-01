
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



#ifndef _DM_MARSHAL_H
#define _DM_MARSHAL_H

struct dm_marshal_hdr {
  int len; // includes this hdr
  int type;
}; 

char *dm_unmarshal_disk(struct dm_marshal_hdr *, 
			 void **,
			 void *parent);

// returns a ptr to the next byte in the buffer
// 2nd argument is set to the address of the unmarshaled thing
// 3rd argument is a pointer to the structure that is the
// "parent" of this structure so back-pointers can be fixed
typedef char*(*dm_unmarshal_t)(struct dm_marshal_hdr *,
				void **result,
				void *parent);

struct dm_marshal_module {
  dm_unmarshal_t unmarshal; /* unmarshaler */
  void **fn_table;
  int fn_table_len;
};

extern struct dm_marshal_module dm_disk_marshal_mod;
extern struct dm_marshal_module dm_layout_g1_marshal_mod;
extern struct dm_marshal_module dm_mech_g1_marshal_mod;

extern struct dm_marshal_module *dm_marshal_mods[];

struct marshaled_fn;

struct dm_disk_if * dm_unmarshal(struct dm_marshal_hdr *h, int bufflen);
struct dm_marshal_hdr * dm_marshal(struct dm_disk_if *d);
void marshal_fn(void *fn, int typ, struct marshaled_fn *result);
void *unmarshal_fn(int *buff, int typ);
void marshal_fns(void **fns, int fns_len, char *b, int typ);
void unmarshal_fns(void **fns, int fns_len, char *b, int typ);

typedef enum {
  DM_DISK_TYP,
  DM_LAYOUT_G1_TYP,
  DM_MECH_G1_TYP,
} dm_marshal_type_t;

#define DM_MARSHAL_MOD_MAX 3


#endif /* _DM_MARSHAL_H */
