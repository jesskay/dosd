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
	@cp LICENSE Makefile README config.mk dosd.1 draw.h dosd_cat ${SRC} dosd-${VERSION}
	@tar -cf dosd-${VERSION}.tar dosd-${VERSION}
	@gzip dosd-${VERSION}.tar
	@rm -rf dosd-${VERSION}

install: all
	@echo installing executables to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f dosd ${DESTDIR}${PREFIX}/bin
	@cp -f dosd_cat ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dosd
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dosd_cat
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < dosd.1 > ${DESTDIR}${MANPREFIX}/man1/dosd.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/dosd.1

uninstall:
	@echo removing executables from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/dosd
	@rm -f ${DESTDIR}${PREFIX}/bin/dosd_cat
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/dosd.1

.PHONY: all options clean dist install uninstall
