/*
 * (C) 2013-2014 by Christian Hesse <mail@eworm.de>
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
#include "version.h"

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
	int i, rc = EXIT_FAILURE, verbose = 0;

	struct ifaddrs *ifaddr = NULL, *ifa;
	struct sockaddr_in * s4;
	struct in_addr * v_host, * v_mask;
	struct in_addr s_broadcast, s_netaddress, s_minhost, s_maxhost;
	char c_address[INET_ADDRSTRLEN], c_netmask[INET_ADDRSTRLEN],
	     c_netaddress[INET_ADDRSTRLEN], c_broadcast[INET_ADDRSTRLEN],
	     c_minhost[INET_ADDRSTRLEN], c_maxhost[INET_ADDRSTRLEN];
	char * interface = NULL;

	char hostname[254];
	char * domainname;
	struct hostent *hp;

	char * config_template = NULL;
	size_t fsize;
	char * config = NULL;
	char * filename = NULL;
	FILE * configfile;
	char * pidfile = NULL, * leasesfile = NULL;

	printf("Starting dyndhcpd/" VERSION " (compiled: " __DATE__ ", " __TIME__ ")\n");

	/* get command line options */
	while ((i = getopt(argc, argv, "c:hi:v")) != -1)
		switch (i) {
			case 'c':
				config_template = optarg;
				if (strlen(config_template) == 0) {
					fprintf(stderr, "Requested different config file, but no name given.\n");
					return EXIT_FAILURE;
				}
				break;
			case 'h':
				fprintf(stderr, "usage: %s [-c config] [-h] -i INTERFACE [-v]\n", argv[0]);
				return EXIT_SUCCESS;
			case 'i':
				interface = optarg;
				if (strlen(interface) == 0) {
					fprintf(stderr, "No interface given!\n");
					return EXIT_FAILURE;
				}
				break;
			case 'v':
				verbose++;
				break;
		}

	/* bail if we are not root */
	if (getuid() > 0) {
		fprintf(stderr, "You need to be root!\n");
		goto out;
	}

	/* check if dhcpd exists and is executable */
	if (access(DHCPDFILE, X_OK) == -1) {
		fprintf(stderr, "The dhcp daemon is not execatable!\n");
		goto out;
	}

	/* get the domainname */
	gethostname(hostname, 254);
	hp = gethostbyname(hostname);
	if ((domainname = strchr(hp->h_name, '.')) != NULL)
		domainname++;
	else {
		fprintf(stderr, "Could not get domainname, using '" FALLBACKDOMAIN "\n");
		domainname = FALLBACKDOMAIN;
	}

	/* give an error if we do not have an interface */
	if (interface == NULL) {
		fprintf(stderr, "No interface given!\n");
		return EXIT_FAILURE;
	}

	/* initialize ifaddr */
	if (getifaddrs(&ifaddr) == -1) {
		fprintf(stderr, "getifaddrs() failed.\n");
		return EXIT_FAILURE;
	}

	/* to find the correct interface */
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		/* next iteration if requirements do not match */
		if (strcmp(interface, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr == NULL)
			continue;
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;

		/* check if the device is up */
		if (!(ifa->ifa_flags & IFF_UP)) {
			fprintf(stderr, "Interface %s is down.\n", interface);
			goto out;
		}

		/* check if the device is connected / cable is plugged in
		 * do warn here, but do not fail */
		if (!(ifa->ifa_flags & IFF_RUNNING))
			fprintf(stderr, "Warning: Interface %s is not connected.\n", interface);

		/* get variables in place for address */
		s4 = (struct sockaddr_in *)ifa->ifa_addr;
		v_host = &s4->sin_addr;

		/* convert address from binary to text form */
		if (!inet_ntop(ifa->ifa_addr->sa_family, v_host, c_address, INET_ADDRSTRLEN))
			fprintf(stderr, "%s: inet_ntop failed!\n", ifa->ifa_name);

		/* get variables in place for netmask */
		s4 = (struct sockaddr_in *)ifa->ifa_netmask;
		v_mask = &s4->sin_addr;

		/* convert netmask from binary to text form */
		if (!inet_ntop(ifa->ifa_netmask->sa_family, v_mask, c_netmask, INET_ADDRSTRLEN))
			fprintf(stderr, "%s: inet_ntop failed!\n", ifa->ifa_name);

		/* calculate broadcast and net address */
		s_broadcast.s_addr = v_host->s_addr |~ v_mask->s_addr;
		s_netaddress.s_addr = v_host->s_addr & v_mask->s_addr;

		/* check if subnet has enough addresses */
		if (ntohl(s_broadcast.s_addr) - ntohl(s_netaddress.s_addr) < 2) {
			fprintf(stderr, "We do not have addresses to serve, need a netmask with 30 bit minimum.\n");
			goto out;
		}

		/* calculate min and max host */
		s_minhost.s_addr = htonl(ntohl(s_netaddress.s_addr) + 1);
		s_maxhost.s_addr = htonl(ntohl(s_broadcast.s_addr) - 1);

		/* convert missing addresses from binary to text form */
		if (inet_ntop(AF_INET, &s_broadcast, c_broadcast, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &s_netaddress, c_netaddress, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &s_minhost, c_minhost, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &s_maxhost, c_maxhost, INET_ADDRSTRLEN) != NULL) {
			/* print information */
			if (verbose)
				printf("Interface: %s\n"
					"Domain: %s\n"
					"Host Address: %s\n"
					"Network Address: %s\n"
					"Broadcast: %s\n"
					"Netmask: %s\n"
					"Hosts: %s - %s\n", interface, domainname, c_address, c_netaddress, c_broadcast, c_netmask, c_minhost, c_maxhost);

			/* open the template for reading */
			if (config_template == NULL)
				config_template = CONFIG_TEMPLATE;
			if ((configfile = fopen(config_template, "r")) == NULL) {
				fprintf(stderr, "Failed opening config template for reading.\n");
				goto out;
			}

			/* seek to the and so we know the file size */
			fseek(configfile, 0, SEEK_END);
			fsize = ftell(configfile);
			fseek(configfile, 0, SEEK_SET);

			/* allocate memory and read file */
			config = malloc(fsize + 1);
			if ((fread(config, fsize, 1, configfile)) != 1) {
				fprintf(stderr, "Failed reading config template.\n");
				goto out;
			}
			fclose(configfile);
			config[fsize] = 0;

			/* replace strings with real values */
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

			/* get new filename and open file for writing */
			filename = malloc(strlen(CONFIG_OUTPUT) + strlen(interface) + 1);
			sprintf(filename, CONFIG_OUTPUT, interface);
			if ((configfile = fopen(filename, "w")) == NULL) {
				fprintf(stderr, "Failed opening config file for writing.\n");
				goto out;
			}

			/* actually write the final configuration to file and close it */
			fputs(config, configfile);
			fclose(configfile);

			/* get names for pid and leases file */
			pidfile = malloc(strlen(PIDFILE) + strlen(interface) + 1);
			sprintf(pidfile, PIDFILE, interface);
			leasesfile = malloc(strlen(LEASESFILE) + strlen(interface) + 1);
			sprintf(leasesfile, LEASESFILE, interface);

			/* check if leases file exists, create it if it does not */
			if (access(leasesfile, R_OK) == -1) {
				if (verbose)
					printf("Creating leases file %s.\n", leasesfile);
				fclose(fopen(leasesfile, "w"));
			}

			/* execute dhcp daemon, replace the current process
			 * dyndhcpd is cleared from memory here and code below is not execuded if
			 * everything goes well */
			if (verbose > 1)
				printf("Running: dhcpd -f -d -q -4 -pf %s -lf %s -cf %s %s\n",
					pidfile, leasesfile, filename, interface);
			rc = execlp(DHCPDFILE, "dhcpd", "-f", "-d", "-q", "-4",
				"-pf", pidfile, "-lf", leasesfile, "-cf", filename, interface, NULL);

			fprintf(stderr, "The dhcp daemon failed to execute.\n");

			goto out;
		} else {
			/* failed to convert addresses from binary to string */
			fprintf(stderr, "Failed converting number to string\n");
			goto out;
		}
	}

	/* we did not find an interface to work with */
	fprintf(stderr, "Interface not found, link down or no address.\n");

out:
	/* free memory */
	if (leasesfile != NULL)
		free(leasesfile);
	if (pidfile != NULL)
		free(pidfile);
	if (filename != NULL)
		free(filename);
	if (config != NULL)
		free(config);
	if (ifaddr != NULL)
		freeifaddrs(ifaddr);

	return rc;
}

// vim: set syntax=c:
