Intro
=====

Hi, this is my fiendly pacman-os. It's mainly based on two sources
* https://www.youtube.com/watch?v=9t-SPC7Tczc&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN
* https://github.com/YoctoForBeaglebone/pacman4console
* https://github.com/floooh/pacman.c.git


Requirements for the older versions with the real-mode
======================================================

'''
sudo zypper in make nasm mtools qemu gcc
sudo cp /usr/sbin/mkfs.fat /usr/bin/
wget https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/open-watcom-2_0-c-linux-x64
chmod +x open-watcom-2_0-c-linux-x64
./open-watcom-2_0-c-linux-x64  # install wcc, include 16-bit compilers
'''


Requirements for the newer versions with the protected mode (master branch)
===========================================================================

1) Install
'''
sudo zypper in make nasm mtools qemu gcc
sudo cp /usr/sbin/mkfs.fat /usr/bin/     # when working as non-root

sudo zypper in -y git makeinfo gcc-c++ gmp-devel mpfr-devel mpc-devel
sudo zypper in -y nasm mtools qemu gcc
'''

2) configure environment variables
'''
export TARGET=i686-elf
export PREFIX="$HOME/Workspace/Toolchain/i686-elf"
export PATH="$PREFIX/bin:$PATH"
export TOOLCHAIN=$PREFIX
'''
into ~/.bashrc

3) compile gcc

'''
cd
mkdir Workspace && cd $_
git clone https://github.com/aleksei-burlakov/pacman-os.git
git clone https://github.com/nanobyte-dev/nanobyte_os.git
git clone https://github.com/nanobyte-dev/nanobyte_experiments

mkdir Toolchain && cd $_
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz
wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz
tar -zxvf gcc-13.2.0.tar.gz
tar -xf binutils-2.41.tar.xz
mkdir binutils-build && cd $_
../binutils-2.41/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
make
make install
cd ..
mkdir gcc-build && cd $_
../gcc-13.2.0/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
'''

Compiling
=========

'''
cd pacman-os
make
'''

Running
=======

'''
make run
'''

Development. Vscode
===================

If you need visual studio do

'''
rpm --import https://packages.microsoft.com/keys/microsoft.asc
zypper ar https://packages.microsoft.com/yumrepos/vscode vscode
zypper refresh
zypper install code
'''
