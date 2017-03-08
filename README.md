dyndhcpd
========

start DHCP daemon that dynamically creates configuration based on
assigned IP address

Requirements
------------

To compile and run `dyndhcpd` you need:

* [ISC DHCPD](https://www.isc.org/software/dhcp)
* [markdown](http://daringfireball.net/projects/markdown/) (HTML documentation)

Build and install
-----------------

Building and installing is very easy. Just run:

> make

followed by:

> make install

This will place an executable at `/usr/bin/dyndhcpd`, template configuration
will go to `/etc/dyndhcpd/`, documentation can be found in
`/usr/share/doc/dyndhcpd/`. Additionally a systemd unit file is installed to
`/usr/lib/systemd/system/dyndhcpd@.service`.

Usage
-----

Just run `dyndhcpd -i <interface>` or start a systemd unit with
`systemctl start dyndhcpd@<interface>`.

### Upstream

URL: [GitHub.com](https://github.com/eworm-de/dyndhcpd)  
Mirror: [eworm.de](https://git.eworm.de/cgit.cgi/dyndhcpd/)
