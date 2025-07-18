dyndhcpd
========

[![GitHub stars](https://img.shields.io/github/stars/eworm-de/dyndhcpd?logo=GitHub&style=flat&color=red)](https://github.com/eworm-de/dyndhcpd/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/eworm-de/dyndhcpd?logo=GitHub&style=flat&color=green)](https://github.com/eworm-de/dyndhcpd/network)
[![GitHub watchers](https://img.shields.io/github/watchers/eworm-de/dyndhcpd?logo=GitHub&style=flat&color=blue)](https://github.com/eworm-de/dyndhcpd/watchers)

start DHCP daemon that dynamically creates configuration based on
assigned IP address

*Use at your own risk*, pay attention to
[license and warranty](#license-and-warranty), and
[disclaimer on external links](#disclaimer-on-external-links)!

Requirements
------------

To compile and run `dyndhcpd` you need:

* [ISC DHCPD ↗️](https://www.isc.org/software/dhcp)
* [markdown ↗️](https://daringfireball.net/projects/markdown/) (HTML documentation)

Build and install
-----------------

Building and installing is very easy. Just run:

    make

followed by:

    make install

This will place an executable at `/usr/bin/dyndhcpd`, template configuration
will go to `/etc/dyndhcpd/`, documentation can be found in
`/usr/share/doc/dyndhcpd/`. Additionally a systemd unit file is installed to
`/usr/lib/systemd/system/dyndhcpd@.service`.

Usage
-----

Just run `dyndhcpd -i <interface>` or start a systemd unit with
`systemctl start dyndhcpd@<interface>`.

License and warranty
--------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
[GNU General Public License](COPYING.md) for more details.

Disclaimer on external links
----------------------------

Our website contains links to the websites of third parties ("external
links"). As the content of these websites is not under our control, we
cannot assume any liability for such external content. In all cases, the
provider of information of the linked websites is liable for the content
and accuracy of the information provided. At the point in time when the
links were placed, no infringements of the law were recognisable to us.
As soon as an infringement of the law becomes known to us, we will
immediately remove the link in question.

> 💡️ **Hint**: All external links are marked with an arrow pointing
> diagonally in an up-right (or north-east) direction (↗️).

### Upstream

URL:
[GitHub.com](https://github.com/eworm-de/dyndhcpd#dyndhcpd)

Mirror:
[eworm.de](https://git.eworm.de/cgit.cgi/dyndhcpd/)
[GitLab.com](https://gitlab.com/eworm-de/dyndhcpd#dyndhcpd)

---
[⬆️ Go back to top](#top)
