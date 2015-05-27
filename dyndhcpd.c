/*
 * (C) 2013-2015 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include "dyndhcpd.h"

const static char optstring[] = "c:hi:v";
const static struct option options_long[] = {
	/* name			has_arg			flag	val */
	{ "config",		required_argument,	NULL,	'c' },
	{ "help",		no_argument,		NULL,	'h' },
	{ "interface",		required_argument,	NULL,	'i' },
	{ "verbose",		no_argument,		NULL,	'v' },
	{ 0, 0, 0, 0 }
};

/*** main ***/
int main(int argc, char ** argv) {
	int i, rc = EXIT_FAILURE, verbose = 0;
	const char * tmp;

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

	char * template = NULL;
	FILE * templatefile;
	const char * templatefilename = NULL;
	char * config = NULL;
	FILE * configfile;
	char * configfilename = NULL;
	size_t fsize, length = 0;

	char * pidfile = NULL, * leasesfile = NULL;

	printf("Starting dyndhcpd/" VERSION " (compiled: " __DATE__ ", " __TIME__ ")\n");

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
			if (templatefilename == NULL)
				templatefilename = CONFIG_TEMPLATE;
			if ((templatefile = fopen(templatefilename, "r")) == NULL) {
				fprintf(stderr, "Failed opening config template for reading.\n");
				goto out;
			}

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

			/* replace strings with real values */
			for (tmp = template; *tmp;) {
				if (*tmp == '_') {
					if (strncmp("__INTERFACE__", tmp, 13) == 0) {
						config = realloc(config, length + strlen(interface) + 1);
						length += sprintf(config + length, interface);
						tmp += 13;
					} else if (strncmp("__VERSION__", tmp, 11) == 0) {
						config = realloc(config, length + strlen(VERSION) + 1);
						length += sprintf(config + length, VERSION);
						tmp += 11;
					} else if (strncmp("__DOMAINNAME__", tmp, 14) == 0) {
						config = realloc(config, length + strlen(domainname) + 1);
						length += sprintf(config + length, domainname);
						tmp += 14;
					} else if (strncmp("__ADDRESS__", tmp, 11) == 0) {
						config = realloc(config, length + strlen(c_address) + 1);
						length += sprintf(config + length, c_address);
						tmp += 11;
					} else if (strncmp("__NETADDRESS__", tmp, 14) == 0) {
						config = realloc(config, length + strlen(c_netaddress) + 1);
						length += sprintf(config + length, c_netaddress);
						tmp += 14;
					} else if (strncmp("__BROADCAST__", tmp, 13) == 0) {
						config = realloc(config, length + strlen(c_broadcast) + 1);
						length += sprintf(config + length, c_broadcast);
						tmp += 13;
					} else if (strncmp("__NETMASK__", tmp, 11) == 0) {
						config = realloc(config, length + strlen(c_netmask) + 1);
						length += sprintf(config + length, c_netmask);
						tmp += 11;
					} else if (strncmp("__MINHOST__", tmp, 11) == 0) {
						config = realloc(config, length + strlen(c_minhost) + 1);
						length += sprintf(config + length, c_minhost);
						tmp += 11;
					} else if (strncmp("__MAXHOST__", tmp, 11) == 0) {
						config = realloc(config, length + strlen(c_maxhost) + 1);
						length += sprintf(config + length, c_maxhost);
						tmp += 11;
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
