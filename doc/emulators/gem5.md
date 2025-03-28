
## Using Gem5

You can configure and build Nautilus for execution in the [Gem5
architectural simulator](http://gem5.org).  Note that Gem5 is very
slow.  Simulated time is 2-3 orders of magnitude slower than
real-time.  If you care about interaction, and not simulation
accuracy, configure Nautilus to override the APIC timing calibration
results, a suboption under the Gem5 target architecture.  Once you
have built the kernel for the Gem5 target architecture, you can copy
`nautilus.bin` to `~gem5/binaries`, and run it using Gem5's example full
system configuration (`~gem5/configs/example/fs.py`), like this (for two
cpus):

```Shell
$> cd ~gem5
$> build/X86/gem5.opt -d run.out configs/example/fs.py -n 2
```

Nautilus on Gem5 follows Gem5's boot model for Linux.  If you don't
want to change anything, just symlink `binaries/nautilus.bin` as the
linux kernel executable the example config expects.  Alternatively,
you can modify the config like this, or do something similar in your
own config:

```
     test_sys = makeLinuxX86System(...)
+++  test_sys.kernel = binary('nautilus.bin')
```

Once Gem5 is running, you can debug Nautilus in the following
Gem5-standard ways:

```Shell
$> telnet localhost 3456  # access serial0 / com1
```

```GDB
gdb binaries/nautilus.bin
(gdb) target remote localhost:7000 # attach debugger to cpu 0
(gdb) set architecture i386:x86-64
(gdb) ...
```

Note that if you want to interact with Nautilus running on Gem5, you
will need to use the virtual console on a char device (`serial0`) to
do so.   If you don't want to interact, please see the `autoexec.bat`
startup script feature in `src/arch/gem5/init.c`.

