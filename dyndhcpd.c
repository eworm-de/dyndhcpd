/*
 * (C) 2013 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * This is an example code skeleton provided by vim-skeleton.
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"

/*** str_replace ***/
char * str_replace(char * original, const char * pattern, const char * replacement) {
	size_t const replen = strlen(replacement);
	size_t const patlen = strlen(pattern);
	size_t const orilen = strlen(original);

	size_t patcnt = 0;
	const char * oriptr;
	const char * patloc;

	/* find how many times the pattern occurs in the original string */
	for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
		patcnt++;

	/* allocate memory for the new string */
	size_t const retlen = orilen + patcnt * (replen - patlen);
	char * const returned = (char *) malloc(sizeof(char) * (retlen + 1));

	if (returned != NULL) {
		/* copy the original string, 
		 * replacing all the instances of the pattern */
		char * retptr = returned;
		for (oriptr = original; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen) {
			size_t const skplen = patloc - oriptr;
			/* copy the section until the occurence of the pattern */
			strncpy(retptr, oriptr, skplen);
			retptr += skplen;
			/* copy the replacement */
			strncpy(retptr, replacement, replen);
			retptr += replen;
		}
		/* copy the rest of the string */
		strcpy(retptr, oriptr);
		free(original);
		return returned;
	}
	return NULL;
}

/*** main ***/
int main(int argc, char ** argv) {
	int i, rc = EXIT_FAILURE;

	struct ifaddrs *ifaddr = NULL, *ifa;
	struct sockaddr_in * s4;
	struct in_addr * v_host, * v_mask;
	struct in_addr s_broadcast, s_netaddress, s_minhost, s_maxhost;
	char c_address[INET_ADDRSTRLEN], c_netmask[INET_ADDRSTRLEN], c_netaddress[INET_ADDRSTRLEN], c_broadcast[INET_ADDRSTRLEN], c_minhost[INET_ADDRSTRLEN], c_maxhost[INET_ADDRSTRLEN];
	char * interface = NULL;

	char hostname[254];
	char * domainname;
	struct hostent *hp;

	size_t fsize;
	char * config = NULL;
	char * filename = NULL;
	FILE * configfile;

	printf("Starting dyndhcpd/" VERSION " (compiled: " __DATE__ ", " __TIME__ ")\n");

	for (i = 1; i < argc; i++) {
		switch ((int)argv[i][0]) {
			case '-':
				switch ((int)argv[i][1]) {
					case 'h':
						fprintf(stderr, "usage: %s [-h] [-iINTERFACE]\n", argv[0]);
						return EXIT_SUCCESS;
					case 'i':
						interface = argv[i] + 2;
						if (strlen(interface) == 0) {
							fprintf(stderr, "No interface given!\n");
							return EXIT_FAILURE;
						}
						break;
					default:
						fprintf(stderr, "unknown option: '%s'\n", argv[i]);
						return EXIT_FAILURE;
				}
				break;
			default:
				fprintf(stderr, "unknown command line argument: '%s'\n", argv[i]);
				return EXIT_FAILURE;
		}
	}

	if (getuid() > 0) {
		fprintf(stderr, "You need to be root!\n");
		goto out;
	}

	gethostname(hostname, 254);
	hp = gethostbyname(hostname);
	if ((domainname = strchr(hp->h_name, '.')) != NULL)
		domainname++;
	else {
		fprintf(stderr, "Could not get domainname, using '" FALLBACKDOMAIN "\n");
		domainname = FALLBACKDOMAIN;
	}

	if (interface == NULL) {
		fprintf(stderr, "No interface given!\n");
		return EXIT_FAILURE;
	}

	if (getifaddrs(&ifaddr) == -1) {
		fprintf(stderr, "getifaddrs() failed.\n");
		return EXIT_FAILURE;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(interface, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr == NULL)
			continue;  
		if (!(ifa->ifa_flags & IFF_UP))
			continue;
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
				
		s4 = (struct sockaddr_in *)ifa->ifa_addr;
		v_host = &s4->sin_addr;

		if (!inet_ntop(ifa->ifa_addr->sa_family, v_host, c_address, INET_ADDRSTRLEN))
			fprintf(stderr, "%s: inet_ntop failed!\n", ifa->ifa_name);

		s4 = (struct sockaddr_in *)ifa->ifa_netmask;
		v_mask = &s4->sin_addr;

		if (!inet_ntop(ifa->ifa_netmask->sa_family, v_mask, c_netmask, INET_ADDRSTRLEN))
			fprintf(stderr, "%s: inet_ntop failed!\n", ifa->ifa_name);

		s_broadcast.s_addr = v_host->s_addr |~ v_mask->s_addr;
		s_netaddress.s_addr = v_host->s_addr & v_mask->s_addr;

		if (ntohl(s_broadcast.s_addr) - ntohl(s_netaddress.s_addr) < 2) {
			fprintf(stderr, "We do not have addresses to serve, need a netmask with 30 bit minimum.\n");
			return EXIT_FAILURE;
		}

		s_minhost.s_addr = htonl(ntohl(s_netaddress.s_addr) + 1);
		s_maxhost.s_addr = htonl(ntohl(s_broadcast.s_addr) - 1);

		if (inet_ntop(AF_INET, &s_broadcast, c_broadcast, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &s_netaddress, c_netaddress, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &s_minhost, c_minhost, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &s_maxhost, c_maxhost, INET_ADDRSTRLEN) != NULL) {
			printf("Interface: %s\n"
					"Domain: %s\n"
					"Host Address: %s\n"
					"Network Address: %s\n"
					"Broadcast: %s\n"
					"Netmask: %s\n"
					"Hosts: %s - %s\n", interface, domainname, c_address, c_netaddress, c_broadcast, c_netmask, c_minhost, c_maxhost);


			/* read the template */
			if ((configfile = fopen(CONFIG_TEMPLATE, "r")) == NULL) {
				fprintf(stderr, "Failed opening config template for reading.\n");
				goto out;
			}
			fseek(configfile, 0, SEEK_END);
			fsize = ftell(configfile);
			fseek(configfile, 0, SEEK_SET);

			config = malloc(fsize + 1);
			if ((fread(config, fsize, 1, configfile)) != 1) {
				fprintf(stderr, "Failed reading config template.\n");
				goto out;
			}
			fclose(configfile);
			config[fsize] = 0;

			if ((config = str_replace(config, "__INTERFACE__", interface)) == NULL)
				goto out;
			if ((config = str_replace(config, "__VERSION__", VERSION)) == NULL)
				goto out;
			if ((config = str_replace(config, "__DOMAINNAME__", domainname)) == NULL)
				goto out;
			if ((config = str_replace(config, "__ADDRESS__", c_address)) == NULL)
				goto out;
			if ((config = str_replace(config, "__NETADDRESS__", c_netaddress)) == NULL)
				goto out;
			if ((config = str_replace(config, "__BROADCAST__", c_broadcast)) == NULL)
				goto out;
			if ((config = str_replace(config, "__NETMASK__", c_netmask)) == NULL)
				goto out;
			if ((config = str_replace(config, "__MINHOST__", c_minhost)) == NULL)
				goto out;
			if ((config = str_replace(config, "__MAXHOST__", c_maxhost)) == NULL)
				goto out;

			filename = malloc(strlen(CONFIG_OUTPUT) + strlen(interface) + 1);
			sprintf(filename, CONFIG_OUTPUT, interface);
			if ((configfile = fopen(filename, "w")) == NULL) {
				fprintf(stderr, "Failed opening config file for writing.\n");
				goto out;
			}
			fputs(config, configfile);
			fclose(configfile);

			execlp("/usr/bin/dhcpd", "dhcpd", "-f", "-d", "-q", "-4", "-cf", filename, interface, NULL);

			rc = EXIT_SUCCESS;
			goto out;
		} else {
			fprintf(stderr, "Failed converting number to string\n");
			goto out;
		}
		
	}

	fprintf(stderr, "Interface not found or no address.\n");

out:
	if (filename != NULL)
		free(filename);
	if (config != NULL)
		free(config);
	if (ifaddr != NULL)
		freeifaddrs(ifaddr);

	return rc;
}

// vim: set syntax=c:
