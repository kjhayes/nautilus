OUTPUT_ARCH( "aarch64" )
ENTRY( nautilus_entry )
SECTIONS
{
  . = 0x40400000;
  PROVIDE(kernel_start = .);
  _loadStart = .;
  .text ALIGN(0x1000) : {
    _textStart = .;
    *(.text.boot*)
    *(.text* .text.*)
    . = ALIGN(0x1000);
    _textEnd = .;
    etext = .;
    . = ALIGN(0x1000);
  }
  .data ALIGN(0x10000) : {
    _dataStart = .;
    *(.sdata .sdata.*)
    *(.data .data.*)
    *(.tdata .tdata.*)
    _dataEnd = .;
  }
  .got ALIGN(0x1000) : {
      *(.got*)
      *(.gnu.linkconce.got*)
  }
  _rodataStart = .;
  .rodata ALIGN(0x1000) : {
    *(.srodata .srodata.*)
    . = ALIGN(0x1000);
    *(.rodata .rodata.*)
  }
  .cmdline_flags ALIGN(0x1000) : {
    __start_flags = .;
    *(.cmdline_flags*);
    __stop_flags = .;
  }
  .tests ALIGN(0x1000) : {
    __start_tests = .;
    *(.tests*);
    __stop_tests = .;
  }
  .shell_cmds ALIGN(0x1000) : {
    __start_shell_cmds = .;
    KEEP(*(SORT_BY_NAME(.shell_cmd.*)));
    __stop_shell_cmds = .;
  }
  _rodataEnd = .;
  _loadEnd = .;
  .bss ALIGN(0x1000) : {
    . = ALIGN(16);
    _bssStart = .;
    *(.sbss .sbss.*)
    . = ALIGN(16);
    *(.bss .bss.*);
    . = ALIGN(16);
    *(.tbss .tbss*);
    _bssEnd = .;
  }
  PROVIDE(kernel_end = .);
}
