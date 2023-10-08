Requirements
============

sudo zypper in make nasm mtools qemu
sudo cp /usr/sbin/mkfs.fat /usr/bin/
wget https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-linux-x64
chmod +x open-watcom-2_0-c-linux-x64
./open-watcom-2_0-c-linux-x64  # install wcc, include 16-bit compilers

Compiling
=========

make

Running
=======

make run

Development. Vscode
===================

rpm --import https://packages.microsoft.com/keys/microsoft.asc
zypper ar https://packages.microsoft.com/yumrepos/vscode vscode
zypper refresh
zypper install code
