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
VERSION := 0.1.8

all: dyndhcpd README.html

config.h:
	$(CP) config.def.h config.h

version.h: $(wildcard .git/HEAD .git/index .git/refs/tags/*) Makefile
	printf "#ifndef VERSION\n#define VERSION \"%s\"\n#endif\n" $(shell git describe --long 2>/dev/null || echo ${VERSION}) > $@

dyndhcpd: dyndhcpd.c dyndhcpd.h config.h version.h
	$(CC) dyndhcpd.c $(CFLAGS) $(LDFLAGS) -o dyndhcpd

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
	gpg --armor --detach-sign --comment dyndhcpd-$(VERSION).tar.xz dyndhcpd-$(VERSION).tar.xz
	git notes --ref=refs/notes/signatures/tar add -C $$(git archive --format=tar --prefix=dyndhcpd-$(VERSION)/ $(VERSION) | gpg --armor --detach-sign --comment dyndhcpd-$(VERSION).tar | git hash-object -w --stdin) $(VERSION)
