UCFLAGS = -O1 -nostdlib -nostdinc

out/ulib.o: lib/ulib.c
	gcc $(UCFLAGS) -c -o $@ $<

out/%: bin/%.c out/ulib.o
	gcc $(UCFLAGS) -c -nostdlib -o $@.o $<
	ld -m elf_x86_64 -nodefaultlibs --omagic -e start -Ttext 0x7 $@.o out/ulib.o -o $@


out:
	mkdir -p out
user: out out/hello
