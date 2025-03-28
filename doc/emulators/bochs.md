# Using BOCHS

While we recommend using QEMU, sometimes it is nice to use the native debugging
support in [BOCHS](http://bochs.sourceforge.net/). We've used BOCHS successfully with version 2.6.8. You must have
a version of BOCHS that is built with x86_64 support, which does not seem to be the
default in a lot of package repos. We had to build it manually. You probably also
want to enable the native debugger.

Here is a BOCHS config file (`~/.bochsrc`) that we used successfully:

```
ata0-master: type=cdrom, path=nautilus.iso, status=inserted
boot: cdrom
com1: enabled=1, mode=file, dev=serial.out
cpu: count=2
cpuid: level=6, mmx=1, level=6, x86_64=1, 1g_pages=1
megs: 2048
```

