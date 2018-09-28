#include <windows.h>
#include "a.h"

void
usleeps(int usec)
{
	int n;
	DWORD s;

	usec /= Millisecond;
	if(usec < 1)
		usec = 1;
	for(n = 1; timeBeginPeriod(n) != TIMERR_NOERROR; n++)
		;
	s = timeGetTime();
	while(timeGetTime() - s < usec)
		;
	timeEndPeriod(n);
}

vlong
utime(void)
{
	int n;
	vlong t;

	for(n = 1; timeBeginPeriod(n) != TIMERR_NOERROR; n++)
		;
	t =  (vlong)(timeGetTime()) * Millisecond;
	timeEndPeriod(n);
	return t;
}
