# dyndhcpd - Dynamically start DHCP Daemon

CC	:= gcc
MD	:= markdown
INSTALL	:= install
RM	:= rm
CP	:= cp
CFLAGS	+= -O2 -Wall -Werror
# this is just a fallback in case you do not use git but downloaded
# a release tarball...
VERSION := 0.0.5

all: dyndhcpd README.html

config.h:
	$(CP) config.def.h config.h

version.h: $(wildcard .git/HEAD .git/index .git/refs/tags/*) Makefile
	echo "#ifndef VERSION" > $@
	echo "#define VERSION \"$(shell git describe --tags --long 2>/dev/null || echo ${VERSION})\"" >> $@
	echo "#endif" >> $@

dyndhcpd: dyndhcpd.c config.h version.h
	$(CC) $(CFLAGS) -o dyndhcpd dyndhcpd.c

README.html: README.md
	$(MD) README.md > README.html

install: install-bin install-doc

install-bin: dyndhcpd
	$(INSTALL) -D -m0755 dyndhcpd $(DESTDIR)/usr/bin/dyndhcpd
	$(INSTALL) -D -m0644 dyndhcpd@.service $(DESTDIR)/usr/lib/systemd/system/dyndhcpd@.service
	$(INSTALL) -D -m0644 dhcpd.conf $(DESTDIR)/etc/dyndhcpd/dhcpd.conf

install-doc: README.html
	$(INSTALL) -D -m0644 README.md $(DESTDIR)/usr/share/doc/dyndhcpd/README.md
	$(INSTALL) -D -m0644 README.html $(DESTDIR)/usr/share/doc/dyndhcpd/README.html

clean:
	$(RM) -f *.o *~ dyndhcpd README.html version.h

distclean:
	$(RM) -f *.o *~ dyndhcpd README.html version.h config.h

release:
	git archive --format=tar.xz --prefix=dyndhcpd-$(VERSION)/ $(VERSION) > dyndhcpd-$(VERSION).tar.xz
	gpg -ab dyndhcpd-$(VERSION).tar.xz
