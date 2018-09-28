/* FIXME */

#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "a.h"

static int tty;	/* fd. */

int
openport(const char *port)
{
	struct termios tio;

	memset(&tio, 0, sizeof tio);
	tio.c_cflag = CS8 | CREAD | CLOCAL;
	tio.c_lflags &= ~(ICANON | ECHO | ISIG);
	tio.c_oflags &= ~OPOST;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;
	tty = open(port, O_RDWR);
	cfsetospeed(&tio, B19200);
	cfsetispeed(&tio, B19200);
	tcsetattr(tty, TCSANOW, &tio);
	return 19200;
}

void
closeport(void)
{
	close(tty);
}

char*
defaultport(void)
{
	#ifdef __linux__
	return "/dev/ttyS0";
	#else
	return "/dev/tty0";
	#endif
}

Packet
recvcmd(void)
{
	Packet c;
	size_t nread;

	for(nread = 0; nread < sizeof c.b;){
		nread += read(tty, c.b+nread, 1);
		usleeps(100*Nanosecond);
	}
	dprint("recv %x %x %x %x\n", c.b[0], c.b[1], c.b[2], c.b[3]);
	//usleeps(50*Millisecond);
	return c;
}

void
sendresp(Packet r)
{
	size_t nwrite;

	dprint("send %x %x %x %x\n", r.b[0], r.b[1], r.b[2], r.b[3]);
	for(nwrite = 0; nwrite < sizeof r.b;){
		nwrite += write(tty, r.b+nwrite, 1);
		usleeps(100*Nanosecond);
	}
}
