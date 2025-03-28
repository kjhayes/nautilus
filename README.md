![Nautilus Logo](https://lh5.googleusercontent.com/8BkFSH-06MvfV9hqSk3D5VJQWPabgfMrlkZOcd6unP2AWYZi9ZOc5sgFtXMhyAHRPHJoMtv87jxwE9214Hx2YqmcFppPnYgpTvyau1wwwhHUee5YEn5Sl0to4LNFMg9D-Q=w1280 "Nautilus Logo")
[![Build Status](https://travis-ci.com/HExSA-Lab/nautilus.svg?branch=master)](https://travis-ci.com/HExSA-Lab/nautilus)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/17390/badge.svg)](https://scan.coverity.com/projects/hexsa-lab-nautilus)
[![CodeFactor](https://www.codefactor.io/repository/github/hexsa-lab/nautilus/badge)](https://www.codefactor.io/repository/github/hexsa-lab/nautilus)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/HExSA-Lab/nautilus.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/HExSA-Lab/nautilus/alerts/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# Nautilus
Nautilus is an example of an Aerokernel, a very thin kernel-layer exposed
(much like Unikernel) directly to a runtime system and/or application.
An Aerokernel does not, by default, have a user-mode! There are several reasons for this,
simplicity and performance among the most important. Furthermore, there are no heavy-weight
processes---only threads, all of which share an address space. Therefore, Nautilus is also an
example of a single address-space OS (SASOS). The *runtime* can implement user-mode features
or address space isolation if this is required for its execution model.

## Table of Contents

- [Background](#background)
- [Prerequisites](#prerequisites)
- [Hardware Support](#hardware-support)
- [Building](#building)
- [Running](#running)
- [Rapid Development](#rapid-development)
- [RISC-V Development](#risc-v-development)
- [ARM64 Development](#arm64-development)
- [Resources](#resources)
- [Maintainers](#maintainers)
- [License](#license)
- [Acknowledgements](#acknowledgements)


## Background
We call the combination of an Aerokernel and the runtime/application using it
a Hybrid Runtime (HRT), in that it is both a runtime *and* a kernel, especially regarding its
ability to use the full machine and determine the proper abstractions to the raw hardware
(if the runtime developer sees a mismatch with his/her needs and the Aerokernel mechanisms,
they can be overridden).

If stronger isolation or more complete POSIX/Linux compatibility is required, it is useful
to run the HRT in the context of a Hybrid Virtual Machine. An HVM allows a virtual machine
to split the (virtual) hardware resources among a regular OS (ROS) and an HRT. The HRT portion of the
HVM can then be seen as a kind of software accelerator. Note that because of the simplicity
of the hardware abstractions in a typical HRT, virtualization overheads are much, much less
significant than in, e.g. a Linux guest.

## Prerequisites

- `clang/llvm 9.0+` (https://releases.llvm.org/download.html)
- `grub` version >= ~2.02
- `xorriso` (for creating ISO images)
- `qemu` or `bochs` (for testing and debugging)
- `NOELLE` (https://github.com/scampanoni/noelle)
- `wllvm or gclang` (https://github.com/travitch/whole-program-llvm) (https://github.com/SRI-CSL/gllvm)

## Hardware Support

- x86_64 machines (AMD and Intel)
- RISCV64 (SiFive)
- ARM64 (Pine64)
- Intel Xeon Phi, both KNC and KNL using [Philix](http://philix.halek.co) for easy booting
- As a Hybrid Virtual Machine (HVM) in the [Palacios VMM](http://v3vee.org/palacios)

Nautilus can also run as a virtual machine under QEMU, BOCHS, KVM, and in a simulated
environment using [Gem5](http://gem5.org/Main_Page)

## Building

First, configure Nautilus by running either
`make menuconfig` or `make defconfig`. The latter
generates a default configuration for you. The former
allows you to customize your kernel build.

Some default config files have been provided in `./setups/*`
and can be loaded by running `make SETUPS-SUBDIR/defconfig`.
For example, `make x64/defconfig`, `make arm64/defconfig`, and `make riscv/defconfig`
will load a config made for the QEMU virtual machine targeting the x64, arm64 and riscv64
architectures respectively.

## Running

To quickly start a QEMU virtual machine based upon the current config,
you can run the command `make qemu`.
The command `make qemu-gdb-PORT` will start the same virtual machine running
a GDB server over TCP on the specified port.

More details on running Nautilus with different emulators/virtual machines/simulators such as QEMU, BOCHS, or Gem5 can be found in `./docs/emulators/`.

## Rapid Development

If you'd like to get started quickly with development, a good way is to use 
[Vagrant](https://www.vagrantup.com/). We've provided a `Vagrantfile` in
the top-level Nautilus directory for provisioning a Vagrant VM which has
pretty much everything you need to develop and run Nautilus. This setup
currently only works for VMWare Fusion/Desktop (which requires the paid Vagrant
VMWare provider). We hope to get this working for VirtualBox, and perhaps AWS
soon. If you already have Vagrant installed, to get started you can do the
following from the top-level Nautilus directory:

```Shell
$> vagrant up
```

This will run for several minutes and provision a VM with all the required
packages. It will automatically clone the latest version of Nautilus and build
it. To connect to the VM, you can ssh into it, and immediately start running
Nautilus. There is a demo put in the VM's nautilus directory which will boot
Nautilus in QEMU with a virtual console on a serial port and the QEMU monitor in
another `tmux` pane:

```Shell
$> vagrant ssh
[vagrant@localhost] cd nautilus
[vagrant@localhost] . ./demo
```

## RISC-V Development

If you'd like to check out building and running Nautilus in an emulated RISC-V
environment using QEMU, we've provided some steps below you may follow to get
started. Note that this is an experimental and incomplete port from x86_64 to
RISC-V.

The first step would be to install a compatiable version of QEMU to emulate the
RISC-V machine. We've tested this port on version 5.0.0:

```Shell
$> git clone https://github.com/qemu/qemu
$> cd qemu
$> git checkout v5.0.0
$> ./configure --target-list=riscv64-softmmu
$> make -j $(nproc)
$> sudo make install
```

This will install a custom version of QEMU that you should then be able to call
using `qemu-system-riscv64`. Next, you'll need to install the [RISC-V GNU
Compiler Toolchain](https://github.com/riscv/riscv-gnu-toolchain). We've used
the Newlib cross-compiler:

```Shell
$> sudo mkdir -p /opt/riscv
$> git clone https://github.com/riscv/riscv-gnu-toolchain
$> cd riscv-gnu-toolchain
$> ./configure --prefix=/opt/riscv
$> make
```

This will run for several minutes and build the entire RISC-V Newlib cross-compiler.
Once complete, add `/opt/riscv/bin` to your `PATH` and you should
now be able to call `riscv64-unknown-elf-gcc` and its cousins.

Next, you'll need to configure with `make menuconfig`. Under Platform and Target
Selection, choose `RISC-V 64-bit Host` and under Build, set Toolchain Root to
`/opt/riscv/riscv64-unknown-elf/bin`. There is also an example config that you
may use with the following command:

```Shell
$> cp configs/cs446-s21-riscv-config .config
```

You should then be able to build and run Nautilus for RISC-V. Try this:

```Shell
$> ARCH=riscv CROSS_COMPILE=riscv64-unknown-elf- make -j
$> qemu-system-riscv64 -kernel nautilus.bin \
                       -m 256M \
                       -smp 1 \
                       -machine virt \
                       -bios none \
                       -nographic \
                       -gdb tcp::1234 \
```

## ARM64 Development

To get Nautilus running in an emulated ARM64 environment we will first need a full ARM64 cross compiler toolchain, a script for this purpose can be found at `/scripts/toolchains/build-aarch64.sh`.
Running this script will build a full GNU compiler toolchain and install it at `/opt/toolchains/aarch64/bin`, this does not include `libgcc` though.
Because you will also need to compile U-Boot (https://github.com/u-boot/u-boot) which requires `libgcc`, you will either need to compile it yourself or find a pre-built toolchain through a package manager.

Once you have a working compiler toolchain, you will need to set the `CROSS_COMPILE` prefix variable in the root Makefile accordingly.
And you will also need to build U-Boot for your platform and set the `UBOOT_BIN` variable to the U-Boot binary.

Then run:
```Shell
$> make menuconfig
```

And set the "Platform and Target" to "Generic ARM64 host", save the config and run:

```Shell
$> make defconfig
$> make qemu-flash
$> make qemu -j
```

This will compile Nautilus for ARM64 and present you with the U-Boot shell.
Run the following commands to configure U-Boot to automatically boot Nautilus.

```Shell
u-boot> setenv bootcmd "bootflow scan -lb; bootm start 0x40400000 - 0x40000000; bootm loados; bootm go"
u-boot> saveenv
u-boot> boot
```

These commands will be saved to `./setup/arm64/flash.img` so U-Boot will automatically boot Nautilus from then on.

## Resources

You can find publications related to Nautilus and HRTs/HVMs at
http://halek.co, http://pdinda.org, http://interweaving.org,
and the lab websites below.

Our labs:

<img src="http://cs.iit.edu/~khale/images/hexsa-logo.png" height=100/>

[HExSA Lab](http://hexsa.halek.co) at [IIT](https://www.iit.edu)

<img src="http://cs.iit.edu/~khale/nautilus/img/prescience.png" height=100/>

[Prescience Lab](http://www.presciencelab.org) at [Northwestern](https://www.northwestern.edu)

## Maintainers

### Nautilus
Primary development is done by [Kyle Hale](http://halek.co) and [Peter
Dinda](http://pdinda.org).

However, many people contribute to the development
and maintenance of Nautilus. Please see [this
page](http://cs.iit.edu/~khale/nautilus/) as well as comments in the headers
and the commit logs for details.

### CARAT CAKE
Primary development done by:
[Brian Suchy](http://briansuchy.com), [Souradip Ghosh](https://souradipghosh.com/), [Drew Kersnar](https://www.linkedin.com/in/dakersnar/), [Siyuan Chai](https://schai.me/), [Aaron Nelson](https://www.linkedin.com/in/a-r-n/), [Zhen Huang](https://www.linkedin.com/in/zhen-huang-9706/), [Michael Cuevas](https://mcuevas.org/), [Alex Bernat](https://github.com/alexbernat), [Gaurav Chaudhary](https://www.linkedin.com/in/gauravchaudhary1993/), [Nikos Hardavellas](https://users.cs.northwestern.edu/~hardav/), [Simone Campanoni](https://users.cs.northwestern.edu/~simonec/#gsc.tab=0), and [Peter Dinda](http://pdinda.org/)

## License
[![MIT License](http://seawisphunter.com/minibuffer/api/MIT-License-transparent.png)](https://github.com/HExSA-Lab/nautilus/blob/master/LICENSE.txt)

## Acknowledgements

<img align="left" src="https://www.nsf.gov/images/logos/NSF_4-Color_bitmap_Logo.png" height=100/>
<img align="left" src="https://ucrtoday.ucr.edu/wp-content/uploads/2018/06/DOE-logo.png" height=100/>
<img src="https://upload.wikimedia.org/wikipedia/commons/3/32/Sandia_National_Laboratories_logo.svg" height=100/>


Nautilus was made possible by support from the United States [National Science
Foundation](https://nsf.gov) (NSF) via grants [CCF-1533560](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1533560), [CRI-1730689](https://nsf.gov/awardsearch/showAward?AWD_ID=1730689&HistoricalAwards=false), [REU-1757964](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1757964), [CNS-1718252](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1718252&HistoricalAwards=false),
CNS-0709168, [CNS-1763743](https://www.nsf.gov/awardsearch/showAward?AWD_ID=0709168), and [CNS-1763612](https://www.nsf.gov/awardsearch/showAward?AWD_ID=1763612&HistoricalAwards=false), the [Department of Energy](https://www.energy.gov/) (DOE) via
grant DE-SC0005343, and [Sandia National Laboratories](https://www.sandia.gov/) through the [Hobbes
Project](https://xstack.sandia.gov/hobbes/), which was funded by the [2013 Exascale Operating and Runtime Systems
Program](https://science.energy.gov/~/media/grants/pdf/lab-announcements/2013/LAB_13-02.pdf) under the [Office of Advanced Scientific Computing Research](https://science.energy.gov/ascr) in the [DOE
Office of Science](https://science.energy.gov/). Sandia National Laboratories is a multi-program laboratory
managed and operated by Sandia Corporation, a wholly owned subsidiary of
Lockheed Martin Corporation, for the U.S. Department of Energy's [National
Nuclear Security Administration](https://www.energy.gov/nnsa/national-nuclear-security-administration) under contract [DE-AC04-94AL85000](https://govtribe.com/award/federal-contract-award/definitive-contract-deac0494al85000).

[Kyle C. Hale](http://halek.co) © 2018
