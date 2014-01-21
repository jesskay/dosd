# dosd - dynamic menu
# See LICENSE file for copyright and license details.

include config.mk

SRC = dosd.c draw.c
OBJ = ${SRC:.c=.o}

all: options dosd

options:
	@echo dosd build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC -c $<
	@${CC} -c $< ${CFLAGS}

${OBJ}: config.mk draw.h

dosd: dosd.o draw.o
	@echo CC -o $@
	@${CC} -o $@ dosd.o draw.o ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f dosd ${OBJ} dosd-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p dosd-${VERSION}
	@cp Makefile config.mk draw.h ${SRC} dosd-${VERSION}
	@tar -cf dosd-${VERSION}.tar dosd-${VERSION}
	@gzip dosd-${VERSION}.tar
	@rm -rf dosd-${VERSION}

install: all
	@echo installing executables to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f dosd ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dosd

uninstall:
	@echo removing executables from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/dosd

.PHONY: all options clean dist install uninstall
