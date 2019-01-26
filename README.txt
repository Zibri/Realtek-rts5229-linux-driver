General Information
===================

Linux driver for Realtek PCI-Express card reader chip.


Build Steps
===========

1) Clone this repo
2) Copy the whole folder into /usr/src/rts5229-1.07/
3) dkms install -m rts5229 -v 1.07
4) Copy blacklist-rts5229.conf to /etc/modprobe.d/
5) reboot your computer

Note: Root privilege is required in step 2, 3 and 4
