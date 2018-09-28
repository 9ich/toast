#include <string.h>
#include <windows.h>
#include "a.h"

static HANDLE handle;	/* port */

int
openport(const char *port)
{
	DCB dcb;
	HANDLE h;

	h = CreateFile(port, GENERIC_READ | GENERIC_WRITE, 0, 0, 
		OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if(h == INVALID_HANDLE_VALUE)
		panic("cannot open port %s");
	if(!GetCommState(h, &dcb)){
		CloseHandle(h);
		panic("cannot get comm state of %s", port);
	}
	dcb.BaudRate = CBR_19200;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	if(!SetCommState(h, &dcb)){
		dprint("cannot set %s to 19200 baud, trying 9600\n", port);
		dcb.BaudRate = CBR_9600;
		if(!SetCommState(h, &dcb)){
			CloseHandle(h);
			panic("cannot set valid baud rate on %s", port);
		}
	}
	dprint("using port %s at %d baud\n", port, dcb.BaudRate);
	handle = h;
	return dcb.BaudRate;
}

void
closeport(void)
{
	if(handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);
}

char*
defaultport(void)
{
	return "COM1";
}

Packet
recvresp(void)
{
	Packet k;
	DWORD nread;
	HANDLE h;

	h = handle;
	memset(k.b, 0, sizeof k.b);
	for(nread = 0; nread < sizeof k.b;){
		OVERLAPPED o;
		DWORD n;

		memset(&o, 0, sizeof o);
		o.hEvent = CreateEvent(nil, TRUE, FALSE, nil);
		if(o.hEvent == nil){
			CloseHandle(h);
			panic("cannot create async recv event (%d)", GetLastError());
		}
		if(ReadFile(h, k.b+nread, 1, nil, &o)){
			if(!GetOverlappedResult(h, &o, &n, FALSE))
				continue;
		}else{
			if(GetLastError() != ERROR_IO_PENDING){
				CloseHandle(h);
				panic("read error (%d)", GetLastError());
			}
			if(WaitForSingleObject(o.hEvent, INFINITE) != WAIT_OBJECT_0){
				CloseHandle(h);
				panic("recv wait error (%d)", GetLastError());
			}
			if(!GetOverlappedResult(h, &o, &n, FALSE))
				continue;
		}
		nread += n;
		usleeps(100*Nanosecond);
	}
	dprint("recv %x %x %x %x\n", k.b[0], k.b[1], k.b[2], k.b[3]);
	usleeps(50*Millisecond);
	return k;
}

void
sendcmd(Packet cmd)
{
	DWORD nwrite;
	HANDLE h;

	dprint("send %x %x %x %x\n", cmd.b[0], cmd.b[1], cmd.b[2], cmd.b[3]);
	h = handle;
	for(nwrite = 0; nwrite < Packetsz;){
		OVERLAPPED o;
		DWORD n;

		memset(&o, 0, sizeof o);
		o.hEvent = CreateEvent(nil, TRUE, FALSE, nil);
		if(o.hEvent == nil){
			CloseHandle(h);
			panic("cannot create async send event (%d)", GetLastError());
		}
		usleeps(100*Nanosecond);
		if(WriteFile(h, cmd.b+nwrite, 1, nil, &o)){
			if(!GetOverlappedResult(h, &o, &n, FALSE))
				continue;
		}else{
			if(GetLastError() != ERROR_IO_PENDING){
				CloseHandle(h);
				panic("write error (%d)", GetLastError());
			}
			if(WaitForSingleObject(o.hEvent, INFINITE) != WAIT_OBJECT_0){
				CloseHandle(h);
				panic("send wait error (%d)", GetLastError());
			}
			if(!GetOverlappedResult(h, &o, &n, FALSE))
				continue;
		}
		nwrite += n;
	}
	usleeps(50*Millisecond);
}

