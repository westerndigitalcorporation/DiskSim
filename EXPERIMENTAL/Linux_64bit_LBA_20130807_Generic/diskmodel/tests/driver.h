#ifndef DRIVER_H
#define DRIVER_H

int addBucket(char *name);

// one outstanding time per bucket
int startClock(int b);
int stopClock(int b);

#endif
// End of file
