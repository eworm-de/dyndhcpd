/*
 * (C) 2013-2017 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include "dyndhcpd.h"

const static char optstring[] = "c:hi:vV";
const static struct option options_long[] = {
	/* name			has_arg			flag	val */
	{ "config",		required_argument,	NULL,	'c' },
	{ "help",		no_argument,		NULL,	'h' },
	{ "interface",		required_argument,	NULL,	'i' },
	{ "verbose",		no_argument,		NULL,	'v' },
	{ "version",		no_argument,		NULL,	'V' },
	{ 0, 0, 0, 0 }
};

/*** replace ***/
int replace(char ** config, size_t *length, const char ** tmp,
		const char * template, const char * value) {
	size_t templatelength = strlen(template);

	if (strncmp(template, *tmp, templatelength) == 0) {
		*config = realloc(*config, *length + strlen(value) + 1);
		*length += sprintf(*config + *length, value);
		*tmp += templatelength;
		return 1;
	}
	return 0;
}

/*** main ***/
int main(int argc, char ** argv) {
	int i, rc = EXIT_FAILURE, verbose = 0;
	const char * tmp;

	struct ifaddrs *ifaddr = NULL, *ifa;
	struct network network, dhcp, bootp;
	char * interface = NULL;

	char hostname[HOST_NAME_MAX];
	const char * domainname;
	struct hostent *hp;

	char * template = NULL;
	FILE * templatefile;
	const char * templatefilename = CONFIG_TEMPLATE;
	char * config = NULL;
	FILE * configfile;
	char * configfilename = NULL;
	size_t fsize, length = 0;

	char * pidfile = NULL, * leasesfile = NULL;

	unsigned int version = 0, help = 0;

	/* get command line options */
	while ((i = getopt_long(argc, argv, optstring, options_long, NULL)) != -1)
		switch (i) {
			case 'c':
				templatefilename = optarg;
				if (strlen(templatefilename) == 0) {
					fprintf(stderr, "Requested different config file, but no name given.\n");
					return EXIT_FAILURE;
				}
				break;
			case 'h':
				help++;
				break;
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
			case 'V':
				verbose++;
				version++;
				break;
		}

	if (verbose > 0)
		printf("%s: %s v%s (compiled: " __DATE__ ", " __TIME__ ")\n", argv[0], PROGNAME, VERSION);

	if (help > 0)
		fprintf(stderr, "usage: %s [-c config] [-h] -i INTERFACE [-v] [-V]\n", argv[0]);

	if (version > 0 || help > 0)
		return EXIT_SUCCESS;

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

	/* get the hostname */
	if (gethostname(hostname, HOST_NAME_MAX) == -1) {
		fprintf(stderr, "Failed to get the hostname.\n");
		goto out;
	}

	/* get the domainname */
	hp = gethostbyname(hostname);
	if ((domainname = strchr(hp->h_name, '.')) != NULL)
		domainname++;
	else {
		fprintf(stderr, "Could not get domainname, using '" FALLBACKDOMAIN "'\n");
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

		/* get variables in place for address and netmask */
		memcpy(&network.address.i, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, sizeof(struct in_addr));
		memcpy(&network.netmask.i, &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr, sizeof(struct in_addr));

		/* check if subnet has enough addresses */
		if ((network.netmask.i.s_addr & 0x0f000000) > 0) {
			fprintf(stderr, "We do not have addresses to serve, need a netmask with 28 bit minimum.\n");
			goto out;
		}

		/* calculate broadcast and netaddress */
		network.broadcast.i.s_addr = network.address.i.s_addr | ~network.netmask.i.s_addr;
		network.netaddress.i.s_addr = network.address.i.s_addr & network.netmask.i.s_addr;

		/* calculate for dhcp subnet */
		dhcp.netmask.i.s_addr = htonl(ntohl(network.netmask.i.s_addr) >> 1 | (1 << 31));
		dhcp.address.i.s_addr = network.address.i.s_addr ^ (dhcp.netmask.i.s_addr ^ network.netmask.i.s_addr);
		dhcp.broadcast.i.s_addr = dhcp.address.i.s_addr | ~dhcp.netmask.i.s_addr;
		dhcp.netaddress.i.s_addr = dhcp.address.i.s_addr & dhcp.netmask.i.s_addr;
		dhcp.minhost.i.s_addr = htonl(ntohl(dhcp.netaddress.i.s_addr) + 1);
		dhcp.maxhost.i.s_addr = htonl(ntohl(dhcp.broadcast.i.s_addr) - 1);

		/* calculate for pxe subnet */
		bootp.netmask.i.s_addr = htonl(ntohl(dhcp.netmask.i.s_addr) >> 1 | (1 << 31));
		bootp.address.i.s_addr = network.address.i.s_addr ^ (bootp.netmask.i.s_addr ^ dhcp.netmask.i.s_addr);
		bootp.broadcast.i.s_addr = bootp.address.i.s_addr | ~bootp.netmask.i.s_addr;
		bootp.netaddress.i.s_addr = bootp.address.i.s_addr & bootp.netmask.i.s_addr;
		bootp.minhost.i.s_addr = htonl(ntohl(bootp.netaddress.i.s_addr) + 1);
		bootp.maxhost.i.s_addr = htonl(ntohl(bootp.broadcast.i.s_addr) - 1);

		/* convert addresses from binary to text form */
		if (inet_ntop(AF_INET, &network.address.i, network.address.c, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &network.netmask.i, network.netmask.c, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &network.broadcast.i, network.broadcast.c, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &network.netaddress.i, network.netaddress.c, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &dhcp.minhost.i, dhcp.minhost.c, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &dhcp.maxhost.i, dhcp.maxhost.c, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &bootp.minhost.i, bootp.minhost.c, INET_ADDRSTRLEN) != NULL &&
				inet_ntop(AF_INET, &bootp.maxhost.i, bootp.maxhost.c, INET_ADDRSTRLEN) != NULL) {
			/* print information */
			if (verbose) {
				int pad = strlen(dhcp.minhost.c) > strlen(bootp.minhost.c) ?
					strlen(dhcp.minhost.c) : strlen(bootp.minhost.c);
				printf( "Interface:    %s\n"
					"Domain:       %s\n"
					"Host Address: %s\n"
					"Network:      %s\n"
					"Broadcast:    %s\n"
					"Netmask:      %s\n"
					"Hosts DHCP:   %-*s - %s\n"
					"Hosts BOOTP:  %-*s - %s\n",
						interface, domainname,
						network.address.c, network.netaddress.c,
						network.broadcast.c, network.netmask.c,
						pad, dhcp.minhost.c, dhcp.maxhost.c,
						pad, bootp.minhost.c, bootp.maxhost.c);
			}

			/* open the template for reading */
			if ((templatefile = fopen(templatefilename, "r")) != NULL) {
				/* seek to the and so we know the file size */
				fseek(templatefile, 0, SEEK_END);
				fsize = ftell(templatefile);
				fseek(templatefile, 0, SEEK_SET);

				/* allocate memory and read file */
				template = malloc(fsize + 1);
				if ((fread(template, fsize, 1, templatefile)) != 1) {
					fprintf(stderr, "Failed reading config template.\n");
					goto out;
				}
				fclose(templatefile);
				template[fsize] = 0;
			} else {
				fprintf(stderr, "Failed opening config template for reading.\n"
						"Using fallback to built in defaults, functionality is limited.\n");
				template = strdup(FALLBACKCONFIG);
			}

			/* replace strings with real values */
			for (tmp = template; *tmp;) {
				if (*tmp == '_') {
					if (replace(&config, &length, &tmp, "__INTERFACE__", interface) ||
						replace(&config, &length, &tmp, "__VERSION__", VERSION) ||
						replace(&config, &length, &tmp, "__HOSTNAME__", hostname) ||
						replace(&config, &length, &tmp, "__DOMAINNAME__", domainname) ||
						replace(&config, &length, &tmp, "__ADDRESS__", network.address.c) ||
						replace(&config, &length, &tmp, "__NETADDRESS__", network.netaddress.c) ||
						replace(&config, &length, &tmp, "__BROADCAST__", network.broadcast.c) ||
						replace(&config, &length, &tmp, "__NETMASK__", network.netmask.c) ||
						replace(&config, &length, &tmp, "__MINDHCP__", dhcp.minhost.c) ||
						replace(&config, &length, &tmp, "__MAXDHCP__", dhcp.maxhost.c) ||
						replace(&config, &length, &tmp, "__MINBOOTP__", bootp.minhost.c) ||
						replace(&config, &length, &tmp, "__MAXBOOTP__", bootp.maxhost.c)) {
						/* do nothing, work has been done */
					} else {
						config = realloc(config, length + 1);
						config[length++] = *tmp++;
					}
				} else {
					config = realloc(config, length + 1);
					config[length++] = *tmp++;
				}
			}
			config = realloc(config, length + 1);
			config[length++] = 0;

			/* get new filename and open file for writing */
			configfilename = malloc(strlen(CONFIG_OUTPUT) + strlen(interface) + 1);
			sprintf(configfilename, CONFIG_OUTPUT, interface);
			if ((configfile = fopen(configfilename, "w")) == NULL) {
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
				printf("Running: dhcpd -f -q -4 -pf %s -lf %s -cf %s %s\n",
					pidfile, leasesfile, configfilename, interface);
			rc = execlp(DHCPDFILE, "dhcpd", "-f", "-q", "-4",
				"-pf", pidfile, "-lf", leasesfile, "-cf", configfilename, interface, NULL);

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
	if (configfilename != NULL)
		free(configfilename);
	if (config != NULL)
		free(config);
	if (template != NULL)
		free(template);
	if (ifaddr != NULL)
		freeifaddrs(ifaddr);

	return rc;
}

// vim: set syntax=c:
