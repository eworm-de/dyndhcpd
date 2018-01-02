/*
 * (C) 2013-2018 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#ifndef _CONFIG_H
#define _CONFIG_H

/* use this for domain if gethostbyname() fails */
#define FALLBACKDOMAIN "localdomain"

/* pathes for config file, first is read, second is written */
#define CONFIG_TEMPLATE	"/etc/dyndhcpd/dhcpd.conf"
#define CONFIG_OUTPUT	"/run/dhcpd-%s.conf"

/* dhcpd executable */
#define DHCPDFILE	"/usr/bin/dhcpd"

/* pathes for pid and leases file, these are passed to dhcpd */
#define PIDFILE		"/run/dhcpd-%s.pid"
#define LEASESFILE	"/var/lib/dhcp/dhcp-%s.leases"

/* default user */
#define USER		"dhcp"

#define FALLBACKCONFIG \
	"# fallback dhcpd.conf for interface __INTERFACE__\n" \
	"# generated by dyndhcpd/__VERSION__\n" \
	"\n" \
	"authoritative;\n" \
	"option domain-name \"__DOMAINNAME__\";\n" \
	"\n" \
	"subnet __NETADDRESS__ netmask __NETMASK__ {\n" \
	"\toption broadcast-address __BROADCAST__;\n" \
	"\toption routers __ADDRESS__;\n" \
	"\toption domain-name-servers __ADDRESS__;\n" \
	"\trange __MINDHCP__ __MAXDHCP__;\n" \
	"}"

#endif /* _CONFIG_H */

// vim: set syntax=c:
