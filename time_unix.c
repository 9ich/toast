#include <stdlib.h>
#include <time.h>
#include "a.h"

void
usleeps(int nsec)
{
	vlong s;

	s = utime();
	while(utime() - s < (vlong)nsec)
		;
}

vlong
utime(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec*Second + now.tv_nsec;
}
