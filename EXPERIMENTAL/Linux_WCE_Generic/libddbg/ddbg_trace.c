
/* libddbg (version 1.0)
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



/* libddbg (version 1.0)
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


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


#include "libddbg.h"
#include <bitvector.h>

struct ddbg_class {
  char *name;
  BITVECTOR(instances, DDBG_MAX_INSTANCE);
};

static struct ddbg_class *ddbg_classes = 0;
static int ddbg_classes_len = 0;
static int ddbg_class_max = 0;
static FILE *ddbg_tracefile = 0;

// register a new class.  Returns an int class id to be used with
// subsequent calls
int ddbg_register(char *classname) {

  if(ddbg_class_max < ddbg_classes_len) {
    goto found;
  }

  ddbg_classes_len *= 2;
  ddbg_classes_len++;
  ddbg_classes = (struct ddbg_class *)realloc(ddbg_classes, ddbg_classes_len * sizeof(struct ddbg_class));
  
 found:
  ddbg_classes[ddbg_class_max].name = strdup(classname);
  bit_zero(ddbg_classes[ddbg_class_max].instances, DDBG_MAX_INSTANCE);
  ddbg_class_max++;

  return ddbg_class_max - 1;
}

// start logging (class,instance) traces.  -1 is the wildcard which logs
// all instances from that class.
void ddbg_enable(int aclass, int ainstance) {
  ddbg_assert(aclass < ddbg_class_max);
  ddbg_assert((ainstance < DDBG_MAX_INSTANCE) || (ainstance == -1));
  if(ainstance != -1) {
    BIT_SET(ddbg_classes[aclass].instances, ainstance);
  }
  else {
    bit_setall(ddbg_classes[aclass].instances, DDBG_MAX_INSTANCE);
  }
}

// stop logging (class,instance) traces.  -1 is the wildcard which
// squelches all instances from that class.
void ddbg_disable(int class, int instance) {
  ddbg_assert(class < ddbg_class_max);
  ddbg_assert((instance < DDBG_MAX_INSTANCE) || (instance == -1));
  if(instance != -1) {
    BIT_RESET(ddbg_classes[class].instances, instance);
  }
  else {
    bit_zero(ddbg_classes[class].instances, DDBG_MAX_INSTANCE);
  }
}



void ddbg_trace(int class, int instance, char *fmt, ...)
{
  ddbg_assert(class < ddbg_class_max);
  ddbg_assert(instance < DDBG_MAX_INSTANCE);

#ifndef _LIBDDBG_FREEBSD
 
  if(BIT_TEST(ddbg_classes[class].instances, instance) && ddbg_tracefile) {
    va_list ap;

    fprintf(ddbg_tracefile, "(%s,%d): ", ddbg_classes[class].name, instance);
    
    va_start(ap, fmt);
    vfprintf(ddbg_tracefile, fmt, ap);

    fprintf(ddbg_tracefile, "\n");
  }
#endif // _LIBDDBG_FREEBSD
}




void ddbg_setfile(FILE *f) {
  ddbg_tracefile = f;
}
