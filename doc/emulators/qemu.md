# Using QEMU

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso -m 2048
```

Nautilus has multicore support, so this will also work just fine:

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso -m 2048 -smp 4
```

You should see Nautilus boot up on all 4 cores.

Nautilus is a NUMA-aware Aerokernel. To see this in action, try (with a sufficiently new
version of QEMU):

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso \
                      -m 8G \
                      -numa node,nodeid=0,cpus=0-1 \
                      -numa node,nodeid=1,cpus=2-3 \
                      -smp 4,sockets=2,cores=2,threads=1
```

Nautilus supports debugging over the serial port. This is useful if you want to
debug a physical machine remotely. All prints after the serial port has been
initialized will be redirected to COM1. To use this, find the SERIAL_REDIRECT
entry and enable it in `make menuconfig`. You can now run like this:

```Shell
$> qemu-system-x86_64 -cdrom nautilus.iso -m 2G -serial stdio
```

Sometimes it is useful to interact with the Nautilus root shell via serial port,
e.g. when you're running under QEMU on a system that does not have a windowing
system. You'll want to first put a character device on the serial port by
rebuilding Nautilus after selecting the *Place a virtual console interface on a character device* option.
Then, after Nautilus boots (making sure you enabled the `-serial stdio` option
in QEMU) you'll see a virtual console at your terminal. You can get to the root
shell by getting to the terminal list with `\``3`. You can then select the root
shell, and you will be able to run shell commands and see output. If you want to
see more kernel output, you can use serial redirection and serial mirroring in
your config.

If you'd like to use Nautilus networking with QEMU, you should use a TUN/TAP
interface. First, you can run the following on your host machine:

```Shell
$> sudo tunctl -d tap0
$> sudo tunctl -t tap0
$> sudo ifconfig tap0 up 10.10.10.2 netmask 255.255.255.0
```

Then you can use the tap interface with QEMU as follows. This particular
invocation attaches both a virtual e1000 fast ethernet card and a virtio
network interface:

```Shell
$> sudo qemu-system-x86_64 -smp 2 \
                           -m 2048 \
                           -vga std \
                           -serial stdio \
                           -cdrom nautilus.iso \
                           -netdev tap,ifname=tap0,script=no,id=net0 \
                               -device virtio-net,netdev=net0 \
                           -netdev user,id=net1 \
                               -device e1000,netdev=net1 \
                           -drive if=none,id=hd0,format=raw,file=nautilus.iso \
                               -device virtio-blk,drive=hd0
```

