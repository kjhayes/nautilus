# Nautilus Buildsystem

## Architecture

Depending on which architecture is specified in the current configuration, a different sub-directory of `scripts/arch` will be set as the value of `ARCH_SCRIPTS_DIR` in the root directory.

Within `ARCH_SCRIPTS_DIR`, the root Makefile will look for and include the `config.mk` file which can specify additional Makefile variables and append to existing variables such as `COMMON_FLAGS` and `CFLAGS` to affect how the kernel is built.

The root Makefile will also look for and include the `rules.mk` file, which is used to add additional rules to the root Makefile which are architecture specific, such as `isoimage` on x64, or `uImage` on ARM64 and RISC-V.

## Toolchain

Very similarly to how architectures can define their own `config.mk` and `rules.mk` files in a subdirectory of `scripts/arch`, toolchains can specify their own `config.mk` and `rules.mk` in `scripts/toolchain`.

These function almost identically to the architecture specific Makefiles, but it is expected that `config.mk` at least provides a definition of toolchain variables such as `CC`, `LD`, `CPP`, `CXX`, `OBJCOPY`, etc. which will be used during the build process.

## Build

`scripts/build.mk` is where all of the actual compilation rules are found for the kernel, including dealing with include file dependencies.
To begin, when `build.mk` is invoked on another directory, it begins by trying to include the local `Makefile` if one exists, expecting that `Makefile` will populate the `obj-y` variable with object files and subdirectories which will need to be built.
The local `Makefile` is also able to add additional rules as dependencies of the `default` rule, which will force them to be run when building the local directory.

`obj-y` needs to consist only of filenames ending in `.o` or `/`. The `.o` object files will be compiled from the corresponding `.c` file, generating a `.cmd` file in the process to keep track of include dependencies.
The files ending in `/` will have `builtin.o` appended to them, causing `build.mk` to recursively invoke itself on the subdirectory in order to generate that subdirectory's `builtin.o` file.
Then all of the `.o` files are linked together generating `builtin.o`.

## KConfig

`scripts/kconfig.mk` makes a subset of KConfig commands available such as `make menuconfig` and `make defconfig` through a set of python scripts.
When looking for `defconfig` files, `kconfig.mk` will search through subdirectories of the `setups` directory, looking for any file path matching the pattern `*defconfig`, so for example,
if the rule `make arm64/defconfig` is run, it will match to the file `nautilus/setups/arm64/defconfig`, and load the config found there.

`scripts/kconfig.mk` also handles generating the `autoconf.h` which is force included to make all of the `NAUT_CONFIG_` macro definitions available in the kernel source.
These variables are also made available all of the Makefiles included in the buildsystem, except for `kconfig.mk` before a `.config` file is generated at least once.

## Clean Rules

All files found recursively within the `scripts` directory which are named `clean.mk` will be included in the root Makefile regardless of any configuration options.
These files are used to specify addtional rules to be added to the `CLEAN_RULES` variable.
All such rules will then be invoked when `make clean` is run, to remove all generated files.

`clean.mk` files are included regardless of configuration so that even if the configuration changes, no generated files will stop being removed by `make clean`. For example, if an x64 kernel is built, generating `nautilus.iso`, which is removed by a clean rule in `scripts/arch/x64/clean.mk`, then swapping to an ARM64 config and running `make clean` should still remove `nautilus.iso`.

## Extra Rules

### Kconfig Fuzzing

To generate a "random" configuration based on the current `.config` the command `make fuzzconfig` is provided.
This will go through and generate a new configuration with randomly set options.

Certain options should not be randomly set however (for example: NAUT_CONFIG_RISCV_KERNEL_LINK_ADDR cannot be set randomly due to alignment constraints)
and so Nautilus has the custom Kconfig attribute `option no_fuzz` which will stop `fuzzconfig` from changing the variables value.

### QEMU

If the variable `QEMU` is defined, then the `scripts/extra/qemu.mk` file will be included, defining a few useful rules such as `make qemu`, which will try to start a virtual machine with the current config, `make qemu-gdb` which is the same as `make qemu` but it will won't start the virtual machine immediately, instead opening a gdbserver on TCP port 1234 to debug the kernel.

`make qemu-gdb-N` can also be invoked for some port number `N` in order to open the gdbserver on a different TCP port.

If `NAUT_CONFIG_USE_FDT` is selected for the architecture, it will also expose rules for extracting the device tree blob: `qemu-dtb` and the device tree source: `qemu-dts` for the virtual machine.

### Objdump

If the toolchain defines `OBJDUMP` then the `scripts/extra/objdump.mk` file will be included, providing the `make asm` rule, to generate the assembly source file `nautilus.asm`.

