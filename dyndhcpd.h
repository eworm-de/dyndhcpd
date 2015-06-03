/*
 * (C) 2013-2015 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#ifndef _DYNDHCPD_H
#define _DYNDHCPD_H

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "config.h"
#include "version.h"

struct address {
	struct in_addr i;
        char c[INET_ADDRSTRLEN];
};

/*** replace ***/
int replace(char ** config, size_t *length, const char ** tmp,
		const char * template, const char * value);

/*** main ***/
int main (int argc, char ** argv);

#endif /* _DYNDHCPD_H */

// vim: set syntax=c:
