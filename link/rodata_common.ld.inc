
  .rodata ALIGN(0x1000) : {
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(0x1000);
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r*)
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

  .modules ALIGN(0x1000) : {
    __start_modules = .;
    *(.module.*);
    __stop_modules = .;
  }

  .aspace_impls ALIGN(0x1000) : {
      __start_aspace_impls = .;
      *(.aspace_impls*);
      __stop_aspace_impls = .;
  }
    
  .alloc_impls ALIGN(0x1000) : {
      __start_alloc_impls = .;
      *(.alloc_impls*);
      __stop_alloc_impls = .;
  }

