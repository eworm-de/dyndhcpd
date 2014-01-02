/*
 * (C) 2013-2014 by Christian Hesse <mail@eworm.de>
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
#define CONFIG_OUTPUT	"/tmp/dhcpd_%s.conf"

#endif /* _CONFIG_H */

// vim: set syntax=c:
