typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;
typedef long long		vlong;

#define nil ((void*)0)
#define Nelem(x) (sizeof((x))/sizeof(*(x)))

enum {
	Packetsz=	4,

	Nanosecond=	1,
	Microsecond=	1000 * Nanosecond,
	Millisecond=	1000 * Microsecond,
	Second=		1000 * Millisecond
};

typedef struct Packet	Packet;

struct Packet {
	uchar b[Packetsz];
};

extern int debug;

/*
 * u.c
 */
#undef panic
void	panic(const char*, ...);
int	dprint(const char*, ...);
char*	stringsep(char**, const char*);
char*		stringdup(const char*);
/*
 * time*.c
 */
void	usleeps(int);
vlong	utime(void);
