/*
 * (C) 2013-2025 by Christian Hesse <mail@eworm.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
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
