/*
 * Test crap.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "a.h"

enum {
	Nhash=	128
};

enum {
	SRgain=	1,	/* gain, 0dB - 18dB. */
	SRkeylock,	/* key lock on/off. */
	SRauto,		/* auto-whitebalance and auto-shading status. */
	SRpage,		/* OSD page. */
	SRpoint		/* menu item, 1st - 13th. */
};

enum {
	Rsmode=		0x01,
	Rslevel=	0x03,	/* auto shutter level. */
	Rspeakavg,			/* peak:average ratio. */
	Rsspeed,			/* speed (auto mode). */
	Rsarea,				/* picture area. */
	Rsmanual=	0x0D,	/* speed (manual mode). */
	Rssynchro,			/* synchro scan interval, or off. */

	Rgmode=		0x20,	/* master gain mode. */
	Rgmax,				/* gain limit (auto mode). */
	Rgain,				/* gain (manual mode). */

	Rwbmode=	0x30,
	Rwbtemp,			/* colour temperature. */
	RwbRpaint,			/* red setting (AWB mode). */
	RwbBpaint,			/* blue setting (AWB mode). */
	Rwbarea,
	RwbRgain=	0x39,	/* red gain (manual mode). */
	RwbBgain,			/* blue gain (manual mode). */
	RwbATWRpaint,		/* red setting (ATW mode). */
	RwbATWBpaint,		/* blue setting (ATW mode). */
	RwbAWBR=	0xA0,	/* red (AWB mode) (read-only). */
	RwbAWBB,			/* red (AWB mode) (read-only). */

	Rpgammaenable=	0x40,
	Rpgamma,
	RpDTLgain=	0x44,	/* DTL gain setting. */
	Rpped=		0x46,	/* master pedestal setting. */
	RpDNR=		0x48,	/* digital noise reduction on/off. */

	Rmatrix=	0x50,	/* matrix on/off. */
	RmRhue,				/* red hue. */
	RmRgain,			/* red gain. */
	RmBhue,
	RmBgain,
	RmYehue,
	RmYegain,
	RmCyhue,
	RmCygain,
	RmMghue,
	RmMggain,

	Rhphase=	0x70,	/* H phase with ext. sync signal */

	Routput=	0x80,	/* output mode. */
	Rshading=	0x84,	/* shading correction mode. */
	Rmanushading,		/* shading correction value (manual mode). */
	Rrgbsync,
	Rbaudrate=	0x88,
	Rosd,				/* analog/digital/both OSD. */
	Rscene=		0x90,	/* scene file. */
	Nreg
};

enum {
	Poff=		0,
	Pindex,
	Pshutter,
	Pgain,
	Pwhitebalance,
	Pprocess,
	Pmatrix,
	Psync,
	Poption,
	Pcolourbar=	0xFF
};

enum {
	Anone=	0,		/* initial state. */
	AWBactive,
	AWBaccept,
	AWBtempmaxR,	/* bad AWB -- R gain hit max. */
	AWBtempminB,	/* bad AWB -- B gain hit min. */
	AWbtempminR,	/* bad AWB -- R gain hit min. */
	AWBtempmaxB,	/* bad AWB -- B gain hit max. */
	AWBlevelhigh,	/* bad AWB -- level too high. */
	AWBlevellow,	/* bad AWB -- level too low. */
	AWBnotavail,	/* bad AWB -- not available. */
	AWBreject,		/* bad AWB. */
	ABBactive,
	ABBaccept,
	ABBreject,
	ABBlensopen,	/* bad ABB -- lens open. */
	ASHDactive,
	ASHDaccept,
	ASHDreject,
	ASHDlimit,		/* ASHD accepted. */
	ASHDhigh,		/* bad ASHD -- level high. */
	ASHDlow,		/* bad ASHD -- level low. */
	ASHDnotavail	/* bad ASHD -- not available. */
};

enum {
	AWBdur=		1400 * Millisecond,
	ABBdur=		800 * Millisecond,
	ASHDdur=	400 * Millisecond
};

typedef int (*Inst)(Packet);
typedef struct Resp	Resp;

struct Resp {
	Packet	c;
	Inst	inst;
	Resp	*next;
};

extern int		openport(const char*);
extern void		closeport(void);
extern char*	defaultport(void);
extern Packet	recvcmd(void);
extern void		sendresp(Packet);
static Resp*	lookup(Packet, Inst);
static void		installinsts(void);
static void		initmem(void);
static long		memread(ushort);
static int		memwrite(ushort, ushort);
static void		runauto(void);
static int		rrand(int, int);
static ulong	hash(uchar p[Packetsz]);
static int		pkcmp(Packet, Packet);
static uchar*	pkchr(uchar*, uchar);
static void		ack(Packet);
static void		errmode(void);
static void		err12(void);
static void		err34(void);

static int	r_bad(Packet);
static int	r_disp(Packet);
static int	r_page(Packet);
static int	r_menu(Packet);
static int	r_data(Packet);
static int	r_awb(Packet);
static int	r_abb(Packet);
static int	r_ashd(Packet);
static int	r_reset(Packet);
static int	r_file(Packet);
static int	r_keylock(Packet);
static int	r_gain(Packet);
static int	r_shutdown(Packet);
static int	r_read(Packet);
static int	r_write(Packet);
static int	r_statusread(Packet);

static Resp *resptab[Nhash];
static struct {
	uchar	c[Packetsz];
	Inst	inst;
} resps[] = {
	{{0x4B, 0x01, 0xFF, 0xFF},	r_disp},
	{{0x4B, 0x02, 0xFF, 0xFF},	r_page},
	{{0x4B, 0x03, 0xFF, 0xFF},	r_menu},	/* up */
	{{0x4B, 0x04, 0xFF, 0xFF},	r_menu},	/* down */
	{{0x4B, 0x05, 0xFF, 0xFF},	r_data},	/* up */
	{{0x4B, 0x06, 0xFF, 0xFF},	r_data},	/* down */
	{{0x4B, 0x07, 0xFF, 0xFF},	r_awb},
	{{0x4B, 0x08, 0xFF, 0xFF},	r_abb},
	{{0x4B, 0x09, 0xFF, 0xFF},	r_ashd},
	{{0x4B, 0x0A, 0xFF, 0xFF},	r_reset},	/* 'preset' */
	{{0x4B, 0x0B, 0xFF, 0xFF},	r_file},
	{{0x4B, 0x0D, 0xFF, 0xFF},	r_keylock},	/* on */
	{{0x4B, 0x0E, 0xFF, 0xFF},	r_keylock},	/* off */
	{{0x4B, 0x11, 0xFF, 0xFF},	r_gain},
	{{0x77, 0x77, 0x77, 0x77},	r_shutdown},	/* special */
};
static vlong autotime;
static int lensopen;
static int menuscreen;		/* menu screen is on. */
static int camscreen;		/* camera screen is on. */
static int keylockedever;	/* has a key lock command ever been sent? */
/* status */
static uchar sreg[] = {
	0xff,	/* dummy */
	0,		/* SRgain */
	0,		/* SRkeylock */
	Anone,	/* SRauto */
	Pindex,	/* SRpage */
	1		/* SRpoint */
};
/* mem */
static ulong mem[0xFF];
static struct {
	uchar	addr;
	int		n;		/* width of value (8 bits or 16). */
	int		ro;		/* read-only to driver. */
	ushort	init;	/* value after boot. */
} memmap[] = {

	{0x01,	1},	/* Rsmode */
	{0x03,	1},	/* Rslevel */
	{0x04,	1},	/* Rspeakavg */
	{0x05,	1},	/* Rsspeed */
	{0x06,	1},	/* Rsarea */
	{0x0D,	1},	/* Rsmanual */
	{0x0E,	2},	/* Rssyncro */

	{0x20,	1},	/* Rgmode */
	{0x21,	1},	/* Rgmax */
	{0x22,	1},	/* Rgain */

	{0x30,	1},	/* Rwbmode */
	{0x31,	1},	/* RwbRpaint */
	{0x32,	1},	/* RwbBpaint */
	{0x33,	1},	/* Rwbarea */
	{0x39,	2},	/* RwbRgain */
	{0x3A,	2},	/* RwbBgain */
	{0x3B,	1},	/* RwbATWRpaint */
	{0x3C,	1},	/* RwbATWBpaint */
	{0x3D,	1,	1},	/* RwbAWBR */
	{0x3E,	1,	1},	/* RwbAWBB */

	{0x40,	1},	/* Rpgammaenable */
	{0x41,	1},	/* Rpgamma */
	{0x44,	1},	/* RpDTLgain */
	{0x46,	1},	/* Rpped */
	{0x48,	1},	/* RpDNR */

	{0x50,	1},	/* Rmatrix */
	{0x51,	1},	/* RmRhue */
	{0x52,	1},	/* RmRgain */
	{0x53,	1},	/* RmBhue */
	{0x54,	1},	/* RmBgain */
	{0x55,	1},	/* RmYehue */
	{0x56,	1},	/* RmYegain */
	{0x57,	1},	/* RmCyhue */
	{0x58,	1},	/* RmCygain */
	{0x59,	1},	/* RmMghue */
	{0x5A,	1},	/* RmMggain */

	{0x70,	2},	/* Rhphase */

	{0x80,	1},	/* Routput */
	{0x84,	1},	/* Rshading */
	{0x85,	1},	/* Rmanushading */
	{0x86,	1},	/* Rrgbsync */
	{0x88,	1},	/* Rbaudrate */
	{0x89,	1},	/* Rosd */
	{0x90,	1}	/* Rscene */
};

static ulong
hash(uchar p[Packetsz])
{
	ulong h;
	uchar *pp;

	h = 0;
	for(pp = p; p < pp+Packetsz; p++)
		h = 31 * h + *p;
	return h % Nhash;
}

static Resp*
lookup(Packet c, Inst inst)
{
	ulong h;
	Resp *r;

	h = hash(c.b);
	for(r = resptab[h]; r != nil; r = r->next)
		if(pkcmp(c, r->c) == 0)
			return r;
	if(inst != nil){
		if((r = malloc(sizeof *r)) == nil)
			panic("out of memory");
		memmove(r->c.b, c.b, sizeof r->c.b);
		r->inst = inst;
		r->next = resptab[h];
		resptab[h] = r;
	}
	return r;
}

static void
installinsts(void)
{
	size_t i;
	Packet cmd;

	memset(resptab, 0, sizeof resptab);
	for(i = 0; i < Nelem(resps); i++){
		memmove(cmd.b, resps[i].c, sizeof cmd.b);
		lookup(cmd, resps[i].inst);
	}
	dprint("installed %d instructions\n", i);
}

static void
initmem(void)
{
	size_t i;
	ulong *p;
	uchar rate;
	
	rate = memread(Rbaudrate);	/* Baudrate stays over resets. */

	memset(mem, 0, sizeof mem);
	for(i = 0; i < Nelem(memmap); i++){
		p = &mem[memmap[i].addr];
		*p = memmap[i].init & 0xFFFF;
		*p |= (memmap[i].n & 0x03) << 30;	/* Bits 30..31 => width. */
		*p |= (memmap[i].ro & 0x01) << 29;	/* Bit 29 => read-only. */
	}
	dprint("initialised %d memory map entries\n", i);

	memwrite(Rbaudrate, rate);
	memwrite(Rshading, 0x02);	/* Shading off. */
}

static long
memread(ushort addr)
{
	if(addr >= Nelem(mem))
		return -1;
	if(((mem[addr] >> 30) & 0x03) == 0)
		return -1;
	return mem[addr] & 0xFFFF;
}

static int
memwrite(ushort addr, ushort val)
{
	ulong *p;

	if(addr >= Nelem(mem))
		return -1;
	p = &mem[addr];
	if(((*p >> 30) & 0x03) == 0){
		*p = 0;
		*p |= 1 << 29;		/* Map read-only. */
		if((val & 0xFF00) != 0)	/* Set width. */
			*p |= (2 << 30);
		else
			*p |= (1 << 30);
	}else
		*p = (*p & 0xFFFF0000) | (val & 0xFFFF);
	return 0;
}

/* If there are any auto-ops executing, see whether they can finish. */
static void
runauto(void)
{
	if(utime() < autotime)
		return;
	switch(sreg[SRauto]){
	case AWBactive:
		dprint("awb complete\n");
		/* 
		 * AWBnotavail is reserved for when AWB Start causes 
		 * an error, so skip it. 
		 */
		do{
			sreg[SRauto] = rrand(AWBaccept, AWBreject);
		}while(sreg[SRauto] == AWBnotavail);
		break;
	case ABBactive:
		dprint("abb complete\n");
		sreg[SRauto] = rrand(ABBaccept, ABBreject);
		break;
	case ASHDactive:
		dprint("ashd complete\n");
		sreg[SRauto] = rrand(ASHDaccept, ASHDlow);
		break;
	}
}

static int
rrand(int lo, int hi)
{
	return (rand() % (hi - lo)) + lo;
}

static void
xec(void)
{
	Inst fn, inst;
	Resp *r;
	Packet c;

	for(;;){
		c = recvcmd();
		switch(c.b[0]){
		case 0x05:
			fn = r_read;
			break;
		case 0x53:
			fn = r_write;
			break;
		case 0xFF:
			fn = r_statusread;
			break;
		default:
			fn = nil;
			break;
		}
		r = lookup(c, fn);
		if(r == nil)
			inst = r_bad;
		else
			inst = r->inst;
		runauto();
		if(!inst(c))
			break;
	}
}

static int
pkcmp(Packet a, Packet b)
{
	size_t i;

	for(i = 0; i < Packetsz; i++)
		if(a.b[i] != b.b[i])
			return -1;
	return 0;
}

static uchar*
pkchr(uchar *a, uchar b)
{
	while(a++ != 0)
		if(*a == b)
			return a;
	return nil;
}

#define awb		(sreg[SRauto] == AWBactive)
#define abb		(sreg[SRauto] == ABBactive)
#define ashd		(sreg[SRauto] == ASHDactive)
#define colourbar	(sreg[SRpage] == Pcolourbar)

static int
r_bad(Packet c)
{
	uchar valid[] = {0x4B, 0x05, 0x53, 0xFF, 0};
	uchar *p;

	dprint("bad cmd %x %x %x %x\n", c.b[0], c.b[1], c.b[2], c.b[3]);
	p = pkchr(valid, c.b[0]);
	if(p != nil)
		err12();
	else
		err34();
	return 1;
}

static int
r_disp(Packet c)
{
	dprint("disp\n");
	if(awb || abb || ashd){
		errmode();
		return 1;
	}
	ack(c);
	return 1;
}

static int
r_page(Packet c)
{
	dprint("page\n");
	if(awb || abb || ashd){
		errmode();
		return 1;
	}
	if(sreg[SRpage] == Poption)
		sreg[SRpage] = Pcolourbar;	/* Wraps to Poff next time. */
	else
		sreg[SRpage]++;
	ack(c);
	return 1;
}

static int
r_menu(Packet c)
{
	if(awb || abb || ashd || colourbar || camscreen){
		errmode();
		return 1;
	}
	if(c.b[1] == 0x03){
		dprint("menu up\n");
		if(sreg[SRpoint] < 12)
			sreg[SRpoint]++;
		else
			sreg[SRpoint] = 0;
	}else{
		dprint("menu down\n");
		if(sreg[SRpoint] > 0)
			sreg[SRpoint]--;
		else
			sreg[SRpoint] = 12;
	}
	ack(c);
	return 1;
}

static int
r_data(Packet c)
{
	if(awb || abb || ashd || colourbar || camscreen){
		errmode();
		return 1;
	}
	/* TODO */
	if(c.b[1] == 0x05){
		dprint("data up\n");
	}else{
		dprint("data down\n");
	}
	ack(c);
	return 1;
}

static int
r_awb(Packet c)
{
	dprint("awb start\n");
	if(awb || abb || ashd){
		errmode();
		return 1;
	}
	if(memread(Rwbmode) != 0 || colourbar || menuscreen){
		sreg[SRauto] = AWBnotavail;
		errmode();
		return 1;
	}
	sreg[SRauto] = AWBactive;
	autotime = utime() + AWBdur;
	ack(c);
	return 1;
}

static int
r_abb(Packet c)
{
	dprint("abb start\n");
	if(awb || abb || ashd || colourbar || camscreen){
		errmode();
		return 1;
	}
	if(lensopen){
		sreg[SRauto] = ABBlensopen;
		errmode();
		return 1;
	}
	sreg[SRauto] = ABBactive;
	autotime = utime() + ABBdur;
	ack(c);
	return 1;
}

static int
r_ashd(Packet c)
{
	dprint("ashd start\n");
	if(awb || abb || ashd){
		errmode();
		return 1;
	}
	if(memread(Rshading) != 0 || colourbar || menuscreen){
		sreg[SRauto] = ASHDnotavail;
		errmode();
		return 1;
	}
	sreg[SRauto] = ASHDactive;
	autotime = utime() + ASHDdur;
	ack(c);
	return 1;
}

static int
r_reset(Packet c)
{
	dprint("data reset\n");
	if(awb || abb || ashd || colourbar || menuscreen){
		errmode();
		return 1;
	}
	/* Partial reset. */
	initmem();
	ack(c);
	return 1;
}

static int
r_file(Packet c)
{
	dprint("scene file\n");
	if(awb || abb || ashd || colourbar){
		errmode();
		return 1;
	}
	if(memread(Rscene) == 4)
		memwrite(Rscene, 0);
	else
		memwrite(Rscene, memread(Rscene) + 1);
	ack(c);
	return 1;
}

static int
r_keylock(Packet c)
{
	if(c.b[1] == 0x0D){
		dprint("keylock on\n");
		keylockedever = 1;
		sreg[SRkeylock] = 1;
	}else{
		dprint("keylock off\n");
		sreg[SRkeylock] = 0;
	}
	ack(c);
	return 1;
}

static int
r_gain(Packet c)
{
	dprint("gain\n");
	if(memread(Rgmode) >= 2)
		memwrite(Rgmode, 0);
	else
		memwrite(Rgmode, memread(Rgmode) + 1);
	ack(c);
	return 1;
}

static int
r_shutdown(Packet c)
{
	Packet r = {{0x06, 'b', 'y', 'e'}};
	sendresp(r);
	return 0;
}

static int
r_read(Packet c)
{
	uchar n;
	ulong *p;
	Packet r = {{0x06, 0, 0, 0}};
	
	dprint("mem read\n");
	if(c.b[1] >= Nelem(mem)){
		err12();
		return 1;
	}
	p = &mem[c.b[1]];
	n = (*p >> 30) & 0x03;
	if(n == 0){
		/* Address not mapped. */
		err12();
		return 1;
	}
	if(c.b[2] != 0xFF || c.b[3] != 0xFF){
		err34();
		return 1;
	}
	r.b[1] = c.b[1];
	r.b[2] = *p & 0xFF;
	r.b[3] = (*p >> 8) & 0xFF;
	sendresp(r);
	return 1;
}

static int
r_write(Packet c)
{
	uchar n, ro;
	ulong *p;
	Packet r = {{0x06, 0, 0, 0}};
	
	dprint("mem write\n");
	if(awb || abb || ashd){
		errmode();
		return 1;
	}
	if(c.b[1] == Rscene && colourbar){
		errmode();
		return 1;
	}
	if(c.b[1] >= Nelem(mem)){
		err12();
		return 1;
	}
	p = &mem[c.b[1]];
	ro = (*p >> 29) & 0x01;
	n = (*p >> 30) & 0x03;
	if(n == 0 || ro){
		/* Address not mapped, or is read-only. */
		err12();
		return 1;
	}
	if(n == 1)
		*p = (*p  & 0xFFFF0000) | c.b[2];
	else
		*p = (*p & 0xFFFF0000) | (c.b[3] << 8) | c.b[2];
	r.b[1] = c.b[1];
	r.b[2] = c.b[2];
	r.b[3] = c.b[3];
	sendresp(r);
	return 1;
}

static int
r_statusread(Packet c)
{
	Packet r = {{0xFF, 0, 0, 0xFF}};

	if(c.b[1] >= Nelem(sreg) || c.b[1] < 1){
		err12();
		return 1;
	}
	if(c.b[2] != 0xFF || c.b[3] != 0xFF){
		err34();
		return 1;
	}
	dprint("status read\n");
	r.b[1] = c.b[1];
	r.b[2] = sreg[c.b[1]];
	sendresp(r);
	return 1;
}

static void
ack(Packet c)
{
	c.b[0] = 0x06;
	sendresp(c);
}

static void
errmode(void)
{
	Packet r = {{0x15, 0x03, 0xFF, 0xFF}};
	dprint("mode error\n");
	sendresp(r);
}

static void
err12(void)
{
	Packet r = {{0x15, 0x01, 0xFF, 0xFF}};

	dprint("1st or 2nd cmd byte invalid\n");
	sendresp(r);
}

static void
err34(void)
{
	Packet r = {{0x15, 0x02, 0xFF, 0xFF}};
	
	dprint("3rd or 4th cmd byte invalid\n");
	sendresp(r);
}

static void
emulisten(const char *port)
{
	int rate;

	rate = openport(port);
	if(rate == 19200)
		memwrite(Rbaudrate, 0x01);
	xec();
	closeport();
}

static void
usage(void)
{
	fprintf(stderr, "usage: emu [-d] [port]\n");
}

int
main(int argc, char **argv)
{
	int c;
	char *port;

	while(--argc > '\0' && (*++argv)[0] == '-')
		while((c = *++argv[0]))
			switch(c){
			case 'd':
				debug = 1;
				break;
			case 'h':
				usage();
				return 0;
			default:
				fprintf(stderr, "emu: bad option '-%c'\n", c);
				usage();
				return 1;
			}
	if(argc == 0)
		port = defaultport();
	else if(argc == 1)
		port = argv[0];
	else{
		usage();
		return 1;
	}
	srand(utime());
	initmem();
	installinsts();
	emulisten(port);
	return 0;
}
