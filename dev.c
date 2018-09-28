#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "a.h"

enum {
	Nhash=	16
};

typedef int (*Cmdfunc)(char**);
typedef struct Cmd Cmd;

struct Cmd {
	char	*s;
	Cmdfunc	fn;
	Cmd		*next;
};

extern int		openport(const char*);
extern void		closeport(void);
extern char*	defaultport(void);
extern void		sendcmd(Packet);
extern Packet	recvresp(void);
static ulong	hash(const char*);
static Cmd*		lookup(const char*, Cmdfunc);
static int		splitcmd(char*, char**, size_t);
static char*	verify(Packet, Packet);
static int		pkcmp(Packet, Packet);
static char*	errtostr(Packet);
static int		printff(const char *, ...);
static void		installcmds(void);
static char*	grabline(FILE*, char*, size_t);
static void		xec(FILE*);
static void		devrun(const char*);
static void		usage(void);

static int		c_disp(char**);
static int		c_page(char**);
static int		c_menu(char**);
static int		c_data(char**);
static int		c_awb(char**);
static int		c_abb(char**);
static int		c_ashd(char**);
static int		c_preset(char**);
static int		c_file(char**);
static int		c_keylock(char**);
static int		c_gain(char**);
static int		c_statusread(char**);
static int		c_read(char**);
static int		c_write(char**);
static int		c_exit(char**);

static Cmd *ctab[Nhash];
static struct {
	const char	*s;
	Cmdfunc		fn;
	const char	*help;
} cmds[] = {
	{"disp",	c_disp},
	{"page",	c_page},
	{"menu",	c_menu,			"[up|down]"},
	{"data",	c_data,			"[up|down]"},
	{"awb",		c_awb},
	{"abb",		c_abb},
	{"ashd",	c_ashd},
	{"preset",	c_preset},
	{"file",	c_file},
	{"keylock",	c_keylock,		"[on|off]"},
	{"gain",	c_gain},
	{"status",	c_statusread,	"[addr]"},
	{"read",	c_read,			"[addr]"},
	{"write",	c_write,		"[addr] [val]"},
	{"exit", c_exit}
};

static ulong
hash(const char *s)
{
	ulong h;

	h = 0;
	while(*s++ != '\0')
		h = 31 * h + *s;
	return h % Nhash;
}

static Cmd*
lookup(const char *s, Cmdfunc f)
{
	ulong h;
	Cmd *p;

	h = hash(s);
	for(p = ctab[h]; p != nil; p = p->next)
		if(strcmp(s, p->s) == 0)
			return p;
	if(f != nil){
		if((p = malloc(sizeof *p)) == nil)
			panic("out of memory");
		p->s = stringdup(s);
		p->fn = f;
		p->next = ctab[h];
		ctab[h] = p;
	}
	return p;
}

static void
installcmds(void)
{
	size_t i;

	for(i = 0; i < Nelem(cmds); i++)
		lookup(cmds[i].s, cmds[i].fn);
	dprint("installed %d commands\n", i);
}

static int
splitcmd(char *str, char **a, size_t n)
{
	size_t i;
	char *s, *tok;

	if((s = str) == nil || *str == '\0' || *str == '\n' || *str == '\r')
		return -1;
	i = 0;
	while(i < n-1 && (tok = stringsep(&s, " ")) != nil)
		a[i++] = tok;
	a[i] = nil;
	return i;
}

static char*
verify(Packet cmd, Packet r)
{
	switch(cmd.b[0]){
	case 0x4B:{
		Packet want = {{0x06, cmd.b[1], 0xFF, 0xFF}};

		if(pkcmp(r, want) != 0)
			return errtostr(r);
		break;
	}
	case 0x05:
		if(r.b[0] != 0x06 || r.b[1] != cmd.b[1])
			return errtostr(r);
		break;
	case 0x53:{
		Packet want = {{0x06, cmd.b[1], cmd.b[2], cmd.b[3]}};

		if(pkcmp(r, want) != 0)
			return errtostr(r);
		break;
	}
	case 0xFF:
		if(r.b[0] != 0xFF || r.b[1] != cmd.b[1] || r.b[3] != 0xFF)
			return errtostr(r);
		break;
	default:
		return nil;
	}
	return nil;
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

static char*
errtostr(Packet err)
{
	if(err.b[0] != 0x15 || err.b[2] != 0xFF || err.b[3] != 0xFF)
		return nil;
	switch(err.b[1]){
	case 0x01:
		return "1st or 2nd command byte invalid";
	case 0x02:
		return "3rd or 4th command byte invalid";
	case 0x03:
		return "device cannot execute command in current mode";
	case 0x04:
		return "communication error";
	case 0x05:
		return "communication timeout";
	}
	return "unknown error";
}

static int
printff(const char *fmt, ...)
{
	int n;
	va_list varg;

	if(fmt == nil)
		return -1;
	va_start(varg, fmt);
	n = vfprintf(stdout, fmt, varg);
	va_end(varg);
	fflush(stdout);
	return n;
}

static char*
grabline(FILE *f, char *buf, size_t n)
{
	int c;
	size_t i;

	if(f == nil || buf == nil)
		return nil;
	for(i = 0; i < n-1; i++){
		c = fgetc(f);
		if(c == EOF || c == '\n')
			break;
		if(c != '\r')
			buf[i] = c;
	}
	buf[i] = '\0';
	if(i < 1 && c == EOF)
		buf = nil;
	return buf;
}

static int
c_disp(char **args)
{
	Packet k = {{0x4B, 0x01, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	err = verify(k, recvresp());
	if(err != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_page(char **args)
{
	Packet k = {{0x4B, 0x02, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	err = verify(k, recvresp());
	if(err != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_menu(char **args)
{
	Packet up = {{0x4B, 0x03, 0xFF, 0xFF}};
	Packet down = {{0x4B, 0x04, 0xFF, 0xFF}};
	char *err;

	if(args[1] == nil){
		printff("missing argument\n");
		return 1;
	}
	if(strcmp(args[1], "up") == 0){
		sendcmd(up);
		if((err = verify(up, recvresp())) != nil)
			printff("%s\n", err);
		return 1;
	}
	if(strcmp(args[1], "down") == 0){
		sendcmd(down);
		if((err = verify(down, recvresp())) != nil)
			printff("%s\n", err);
		return 1;
	}
	printff("invalid argument '%s'\n", args[1]);
	return 1;
}

static int
c_data(char **args)
{
	Packet up = {{0x4B, 0x05, 0xFF, 0xFF}};
	Packet down = {{0x4B, 0x06, 0xFF, 0xFF}};
	char *err;

	if(args[1] == nil){
		printff("missing argument\n");
		return 1;
	}
	if(strcmp(args[1], "up") == 0){
		sendcmd(up);
		if((err = verify(up, recvresp())) != nil)
			printff("%s\n", err);
		return 1;
	}
	if(strcmp(args[1], "down") == 0){
		sendcmd(down);
		if((err = verify(down, recvresp())) != nil)
			printff("%s\n", err);
		return 1;
	}
	printff("invalid argument '%s'\n", args[1]);
	return 1;
}

static int
c_awb(char **args)
{
	Packet k = {{0x4B, 0x07, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_abb(char **args)
{
	Packet k = {{0x4B, 0x08, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_ashd(char **args)
{
	Packet k = {{0x4B, 0x09, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_preset(char **args)
{
	Packet k = {{0x4B, 0x0A, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_file(char **args)
{
	Packet k = {{0x4B, 0x0B, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_keylock(char **args)
{
	Packet on = {{0x4B, 0x0D, 0xFF, 0xFF}};
	Packet off = {{0x4B, 0x0E, 0xFF, 0xFF}};
	char *err;

	if(args[1] == nil){
		printff("missing argument\n");
		return 1;
	}
	if(strcmp(args[1], "on") == 0){
		sendcmd(on);
		if((err = verify(on, recvresp())) != nil)
			printff("%s\n", err);
		return 1;
	}
	if(strcmp(args[1], "off") == 0){
		sendcmd(off);
		if((err = verify(off, recvresp())) != nil)
			printff("%s\n", err);
		return 1;
	}
	printff("invalid argument '%s'\n", args[1]);
	return 1;
}

static int
c_gain(char **args)
{
	Packet k = {{0x4B, 0x11, 0xFF, 0xFF}};
	char *err;

	(void)args;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_statusread(char **args)
{
	Packet k = {{0xFF, 0, 0xFF, 0xFF}};
	int addr;
	char *err;

	if(args[1] == nil){
		printff("missing argument\n");
		return 1;
	}
	if(sscanf(args[1], "%x", &addr) < 1){
		printff("invalid argument -- expected hexadecimal address\n");
		return 1;
	}
	k.b[1] = addr & 0xFF;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_read(char **args)
{
	Packet k = {{0x05, 0, 0xFF, 0xFF}};
	Packet r;
	int addr, val;
	char *err;

	if(args[1] == nil){
		printff("missing argument\n");
		return 1;
	}
	if(sscanf(args[1], "%x", &addr) < 1){
		printff("invalid argument -- expected hexadecimal address\n");
		return 1;
	}
	k.b[1] = addr & 0xFF;
	sendcmd(k);
	r = recvresp();
	if((err = verify(k, r)) != nil)
		printff("%s\n", err);
	val = r.b[2] | (r.b[3] << 8);
	printff("%x\n", val);
	return 1;
}

static int
c_write(char **args)
{
	Packet k = {{0x53, 0, 0, 0}};
	char *err;
	int n, addr, val;

	for(n = 0; (args+1)[n] != nil; n++)
		;
	if(n < 2){
		printff("missing arguments\n");
		return 1;
	}
	if(sscanf(args[1], "%x", &addr) < 1){
		printff("invalid argument -- expected hexadecimal address\n");
		return 1;
	}
	if(sscanf(args[2], "%x", &val) < 1){
		printff("invalid argument -- expected hexadecimal value\n");
		return 1;
	}
	k.b[1] = addr & 0xFF;
	k.b[2] = val & 0xFF;
	k.b[3] = (val >> 8) & 0xFF;
	sendcmd(k);
	if((err = verify(k, recvresp())) != nil)
		printff("%s\n", err);
	return 1;
}

static int
c_exit(char **args)
{
	return 0;
}

static void
xec(FILE *f)
{
	while(!feof(f)){
		Cmd *c;
		char buf[1024];
		char *cb[32];

		memset(buf, 0, sizeof buf);
		if(grabline(f, buf, sizeof buf) == nil)
			break;
		if(splitcmd(buf, cb, Nelem(cb)) < 1)
			continue;
		c = lookup(cb[0], nil);
		if(c == nil){
			fprintf(stderr, "unknown command '%s'\n", cb[0]);
			continue;
		}
		if(!c->fn(cb))
			break;
	}
}

static void
devrun(const char *port)
{
	openport(port);
	xec(stdin);
	closeport();
}

static void
usage(void)
{
	fprintf(stderr, "usage: ik-dev [-d] [-h] [port]\n");
}

static void
help(void)
{
	size_t i;

	printf("commands:\n");
	for(i = 0; i < Nelem(cmds); i++){
		printf("  %s", cmds[i].s);
		if(cmds[i].help != nil)
			printf(" %s", cmds[i].help);
		printf("\n");
	}
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
				help();
				return 0;
			default:
				fprintf(stderr, "bad option '-%c'\n", c);
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
	installcmds();
	devrun(port);
	return 0;
}
