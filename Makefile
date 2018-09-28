distfiles= \
  Makefile \
  LICENCE \
  README \
  a.h \
  dev.c \
  dev_windows.c \
  ctl.tcl \
  emu.c \
  emu_unix.c \
  emu_windows.c \
  installer.nsi \
  time_unix.c \
  time_windows.c \
  u.c

CFLAGS=-Os -Wall
ver=1.0.0a3

ifeq ($(OS),Windows_NT)
	include Makefile.windows
else
	include Makefile.linux
endif

${emu}: ${emuofiles}
	${LD} ${LDFLAGS} -o ${emu} ${emuofiles} ${emulibs}

${dev}: ${devofiles}
	${LD} ${LDFLAGS} -o ${dev} ${devofiles} ${devlibs}

dist:
	zip ik-${ver}-src.zip ${distfiles}
	tar -c ${distfiles} | gzip >ik-${ver}-src.tar.gz

clean:
	@rm -f ${emu} ${dev} ${ctl} ${emuofiles} ${devofiles}
	@rm -rf ik-ctl.vfs
	@rm -f tclkit$E sdx.kit
	@rm -f ik-${ver}-src.zip ik-${ver}-src.tar.gz
	@rm -f ik-${ver}-win.zip ik-${ver}-win.tar.gz

