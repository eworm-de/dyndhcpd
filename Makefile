# dyndhcpd - Dynamically start DHCP Daemon

CC	:= gcc
MD	:= markdown
INSTALL	:= install
RM	:= rm
CP	:= cp
CFLAGS	+= -O2 -Wall -Werror
VERSION := $(shell git describe --tags --long 2>/dev/null)
# this is just a fallback in case you do not use git but downloaded
# a release tarball...
ifeq ($(VERSION),)
VERSION := 0.0.2
endif

all: dyndhcpd README.html

config.h:
	$(CP) config.def.h config.h

dyndhcpd: dyndhcpd.c config.h
	$(CC) $(CFLAGS) -o dyndhcpd dyndhcpd.c \
		-DVERSION="\"$(VERSION)\""

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
	$(RM) -f *.o *~ dyndhcpd README.html

distclean:
	$(RM) -f *.o *~ dyndhcpd README.html config.h
