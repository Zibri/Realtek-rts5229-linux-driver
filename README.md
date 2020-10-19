General Information
===================

Linux driver for Realtek PCI-Express card reader chip.


Build Steps
===========

1) Clone this repo
2) Copy the whole folder into /usr/src/rts5229-1.07/
3) sudo dkms install -m rts5229 -v 1.07
4) Copy blacklist-rts5229.conf to /etc/modprobe.d/ (Optional)
5) sudo update-initramfs -u
6) sudo dkms autoinstall
7) reboot your computer

Note: Root privilege is required in step 2, 3 and 4

If you wanna unload the module after suspend, run the following command:

```bash
$ echo SUSPEND_MODULES="rts5229" | sudo tee -a /etc/pm/config.d/modules
```

All procedure:

```bash
git clone https://github.com/Zibri/Realtek-rts5229-linux-driver.git
sudo mkdir /usr/src/rts5229-1.07
sudo cp Realtek-rts5229-linux-driver/* /usr/src/rts5229-1.07
cd /usr/src/rts5229-1.07
sudo su
> dkms install -m rts5229 -v 1.07
> exit
sudo mkinitcpio -p linux
sudo dkms autoinstall
reboot
```
