# dyndhcpd - Dynamically start DHCP Daemon

# commands
CC	:= gcc
MD	:= markdown
INSTALL	:= install
RM	:= rm
CP	:= cp

# flags
CFLAGS	+= -std=c11 -O2 -fPIC -Wall -Werror
LDFLAGS	+= -Wl,-z,now -Wl,-z,relro -pie

# this is just a fallback in case you do not use git but downloaded
# a release tarball...
VERSION := 0.1.5

all: dyndhcpd README.html

config.h:
	$(CP) config.def.h config.h

version.h: $(wildcard .git/HEAD .git/index .git/refs/tags/*) Makefile
	echo "#ifndef VERSION" > $@
	echo "#define VERSION \"$(shell git describe --tags --long 2>/dev/null || echo ${VERSION})\"" >> $@
	echo "#endif" >> $@

dyndhcpd: dyndhcpd.c dyndhcpd.h config.h version.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o dyndhcpd dyndhcpd.c

README.html: README.md
	$(MD) README.md > README.html

install: install-bin install-doc

install-bin: dyndhcpd
	$(INSTALL) -D -m0755 dyndhcpd $(DESTDIR)/usr/bin/dyndhcpd
	$(INSTALL) -D -m0644 config/dhcpd.conf $(DESTDIR)/etc/dyndhcpd/dhcpd.conf
	$(INSTALL) -D -m0644 config/ipxe-options.conf $(DESTDIR)/etc/dyndhcpd/ipxe-options.conf
	$(INSTALL) -D -m0644 systemd/dyndhcpd@.service $(DESTDIR)/usr/lib/systemd/system/dyndhcpd@.service

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
