#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "a.h"

int debug = 0;	/* -d */

void
panic(const char *fmt, ...)
{
	va_list varg;

	fprintf(stderr, "panic: ");
	va_start(varg, fmt);
	vfprintf(stderr, fmt, varg);
	va_end(varg);
	fprintf(stderr, "\n");
	fflush(stderr);
	exit(2);
}

int
dprint(const char *fmt, ...)
{
	int n;
	va_list varg;

	if(!debug)
		return 0;
	va_start(varg, fmt);
	n = vfprintf(stderr, fmt, varg);
	va_end(varg);
	return n;
}

char*
stringsep(char **pp, const char *delim)
{
	char *s, *tok;
	const char *p;
	int c, cc;

	if(pp == nil || *pp == nil)
		return nil;
	s = *pp;
	for(tok = s;;){
		c = *s++;
		p = delim;
		do{
			if((cc = *p++) == c){
				if(c == '\0')
					s = nil;
				else
					s[-1] = '\0';
				*pp = s;
				return tok;
			}
		}while(cc != '\0');
	}
	return nil;
}

char*
stringdup(const char *s)
{
	char *p;

	if((p = malloc(strlen(s)+1)) == nil)
		panic("out of memory");
	strcpy(p, s);
	return p;
}

