/*
 * (C) 2013-2020 by Christian Hesse <mail@eworm.de>
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

#ifndef _DYNDHCPD_H
#define _DYNDHCPD_H

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <limits.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "config.h"
#include "version.h"

#define PROGNAME	"dyndhcpd"

struct address {
	struct in_addr i;
        char c[INET_ADDRSTRLEN];
};

struct network {
	struct address address;
	struct address netaddress;
	struct address netmask;
	struct address broadcast;
	struct address minhost;
	struct address maxhost;
};

/*** replace ***/
int replace(char ** config, size_t *length, const char ** tmp,
		const char * template, const char * value);

/*** main ***/
int main (int argc, char ** argv);

#endif /* _DYNDHCPD_H */

// vim: set syntax=c:
