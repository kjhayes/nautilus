/* vim: tabstop=4 shiftwidth=4 noexpandtab
 */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64",
              "elf64-x86-64")

ENTRY(start)

SECTIONS
{

	. = 0x10000;

	.init           :
	{
		KEEP (*(SORT_NONE(.init)))
	}
	.plt            : { *(.plt) *(.iplt) }
	.plt.bnd        : { *(.plt.bnd) }
	.text           :
	{
		*(.text)
	}

  .rodata ALIGN(4K) : {
    *(.rodata)
  }
	.data : { *(.bss) *(.data) }
}

