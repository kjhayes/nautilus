/*
 * We compile this to an empty object file to avoid complaints for the toolchain if
 * the build system tries to compile a builtin.o without any input files
 * (Used to use /dev/null but ld.lld runs into ABI issues for RISCV)
 */
