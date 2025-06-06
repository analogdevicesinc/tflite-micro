/* This linker script generated from xt-genldscripts.tpp for LSP C:\Users\SLandge\cces\3.0.0\EHP_BSP\drivers\adc\Audio_Talkthrough_I2S\EV-SOMCRR\SC835\sharcfx\AD_TalkT_I2S_EZ_SC835_SHARCFX_Core1\system\startup_lsp\lsp */
/* Linker Script for default link */
MEMORY
{
  L2_ram_fx_seg :                     	org = 0x20020000, len = 0x1DE000
  L2_boot_noload_seg :                	org = 0x201FE000, len = 0x2000
  L1_data_seg :                       	org = 0x2F780000, len = 0x80000
  L1_code_seg :                       	org = 0x2F800000, len = 0x10000
  L3_ram_fx_seg :                     	org = 0x80000000, len = 0x10000000
}

PHDRS
{
  L2_ram_arm_phdr PT_LOAD;
  L2_ram_fx_phdr PT_LOAD;
  L2_ram_fx_bss_phdr PT_LOAD;
  L2_boot_noload_phdr PT_LOAD;
  L2_boot_code_0_phdr PT_LOAD;
  L2_boot_code_1_phdr PT_LOAD;
  L1_data_mp_phdr PT_LOAD;
  L1_code_mp_phdr PT_LOAD;
  L2_boot_code_arm_phdr PT_LOAD;
  L1_code_arm_phdr PT_LOAD;
  L1_data_arm_phdr PT_LOAD;
  L1_data_phdr PT_LOAD;
  L1_data_bss_phdr PT_LOAD;
  L1_code_phdr PT_LOAD;
  System_mmr_phdr PT_LOAD;
  SPI_data_phdr PT_LOAD;
  L3_ram_fx_phdr PT_LOAD;
  L3_ram_fx_bss_phdr PT_LOAD;
  L3_ram_arm_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetHandler)


/*  Memory boundary addresses:  */
_memmap_mem_L2_sram_start = 0x20000000;
_memmap_mem_L2_sram_end   = 0x20200000;
_memmap_mem_L2_boot_rom_start = 0x20200000;
_memmap_mem_L2_boot_rom_end   = 0x20220000;
_memmap_mem_L1_dram_mp_start = 0x28240000;
_memmap_mem_L1_dram_mp_end   = 0x282c0000;
_memmap_mem_L1_iram_mp_start = 0x282c0000;
_memmap_mem_L1_iram_mp_end   = 0x282d0000;
_memmap_mem_L2_boot_rom_arm_start = 0x28a40000;
_memmap_mem_L2_boot_rom_arm_end   = 0x28a50000;
_memmap_mem_L1_iram_arm_start = 0x28a80000;
_memmap_mem_L1_iram_arm_end   = 0x28a90000;
_memmap_mem_L1_dram_arm_start = 0x28ac0000;
_memmap_mem_L1_dram_arm_end   = 0x28ae0000;
_memmap_mem_L1_dram_start = 0x2f780000;
_memmap_mem_L1_dram_end   = 0x2f800000;
_memmap_mem_L1_iram_start = 0x2f800000;
_memmap_mem_L1_iram_end   = 0x2f810000;
_memmap_mem_Sys_mmr_start = 0x30000000;
_memmap_mem_Sys_mmr_end   = 0x40000000;
_memmap_mem_SPI_start = 0x60000000;
_memmap_mem_SPI_end   = 0x80000000;
_memmap_mem_L3_sdram_start = 0x80000000;
_memmap_mem_L3_sdram_end   = 0xc0000000;

/*  Memory segment boundary addresses:  */
_memmap_seg_L2_ram_fx_start = 0x20020000;
_memmap_seg_L2_ram_fx_max   = 0x201fe000;
_memmap_seg_L2_boot_noload_start = 0x201fe000;
_memmap_seg_L2_boot_noload_max   = 0x20200000;
_memmap_seg_L1_data_start = 0x2f780000;
_memmap_seg_L1_data_max   = 0x2f800000;
_memmap_seg_L1_code_start = 0x2f800000;
_memmap_seg_L1_code_max   = 0x2f810000;
_memmap_seg_L3_ram_fx_start = 0x80000000;
_memmap_seg_L3_ram_fx_max   = 0x90000000;

_rom_store_table = 0;
PROVIDE(_memmap_vecbase_reset = 0x2f800000);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x00111010;
_memmap_cacheattr_wt_base = 0x00333030;
_memmap_cacheattr_bp_base = 0x00444040;
_memmap_cacheattr_unused_mask = 0xFF000F0F;
_memmap_cacheattr_wb_trapnull = 0x44111410;
_memmap_cacheattr_wba_trapnull = 0x44111410;
_memmap_cacheattr_wbna_trapnull = 0x44222420;
_memmap_cacheattr_wt_trapnull = 0x44333430;
_memmap_cacheattr_bp_trapnull = 0x44444440;
_memmap_cacheattr_wb_strict = 0x00111010;
_memmap_cacheattr_wt_strict = 0x00333030;
_memmap_cacheattr_bp_strict = 0x00444040;
_memmap_cacheattr_wb_allvalid = 0x44111414;
_memmap_cacheattr_wt_allvalid = 0x44333434;
_memmap_cacheattr_bp_allvalid = 0x44444444;
_memmap_region_map = 0x0000003A;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{


  .L2_boot.noload (NOLOAD) : ALIGN(4)
  {
    _L2_boot_noload_start = ABSOLUTE(.);
    *(.L2_boot.noload)
    . = ALIGN (4);
    _L2_boot_noload_end = ABSOLUTE(.);
    _memmap_seg_L2_boot_noload_end = ALIGN(0x8);
  } >L2_boot_noload_seg :L2_boot_noload_phdr

  _memmap_mem_L2_sram_max = ABSOLUTE(.);


  _memmap_mem_L2_boot_rom_max = ABSOLUTE(.);

  _memmap_mem_L1_dram_mp_max = ABSOLUTE(.);

  _memmap_mem_L1_iram_mp_max = ABSOLUTE(.);

  _memmap_mem_L2_boot_rom_arm_max = ABSOLUTE(.);

  _memmap_mem_L1_iram_arm_max = ABSOLUTE(.);

  _memmap_mem_L1_dram_arm_max = ABSOLUTE(.);

  .L1.rodata : ALIGN(4)
  {
    _L1_rodata_start = ABSOLUTE(.);
    *(.L1.rodata)
    . = ALIGN (4);
    _L1_rodata_end = ABSOLUTE(.);
  } >L1_data_seg :L1_data_phdr

  .dram0.rodata : ALIGN(4)
  {
    _dram0_rodata_start = ABSOLUTE(.);
    *(.dram0.rodata)
    *(.dram.rodata)
    . = ALIGN (4);
    _dram0_rodata_end = ABSOLUTE(.);
  } >L1_data_seg :L1_data_phdr

  .L1.data : ALIGN(4)
  {
    _L1_data_start = ABSOLUTE(.);
    *(.L1.data)
    . = ALIGN (4);
    _L1_data_end = ABSOLUTE(.);
  } >L1_data_seg :L1_data_phdr

  .dram0.data : ALIGN(4)
  {
    _dram0_data_start = ABSOLUTE(.);
    *(.dram0.data)
    *(.dram.data)
    . = ALIGN (4);
    _dram0_data_end = ABSOLUTE(.);
  } >L1_data_seg :L1_data_phdr

  .L1.noload (NOLOAD) : ALIGN(4)
  {
    _L1_noload_start = ABSOLUTE(.);
    *(.L1.noload)
    . = ALIGN (4);
    _L1_noload_end = ABSOLUTE(.);
  } >L1_data_seg :L1_data_phdr

  .L1.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _L1_bss_start = ABSOLUTE(.);
    *(.L1.bss)
    *(.dram0.bss)
    . = ALIGN (8);
    _L1_bss_end = ABSOLUTE(.);
    _end = ALIGN(0x8);
    PROVIDE(end = ALIGN(0x8));
    _stack_sentry = ALIGN(0x8);
    _memmap_seg_L1_data_end = ALIGN(0x8);
  } >L1_data_seg :L1_data_bss_phdr

  PROVIDE(__stack = 0x2f800000);
  _heap_sentry = 0x2f800000;
  _isb_default = 0x2f7ff840;
  _memmap_mem_L1_dram_max = ABSOLUTE(.);

  .DispatchVector.text : ALIGN(4)
  {
    _DispatchVector_text_start = ABSOLUTE(.);
    KEEP (*(.DispatchVector.text))
    . = ALIGN (4);
    _DispatchVector_text_end = ABSOLUTE(.);
  } >L1_code_seg :L1_code_phdr

  .ResetHandler.text : ALIGN(4)
  {
    _ResetHandler_text_start = ABSOLUTE(.);
    *(.ResetHandler.literal .ResetHandler.text)
    . = ALIGN (4);
    _ResetHandler_text_end = ABSOLUTE(.);
  } >L1_code_seg :L1_code_phdr

  .DispatchHandler.text : ALIGN(4)
  {
    _DispatchHandler_text_start = ABSOLUTE(.);
    *(.DispatchHandler.literal .DispatchHandler.text)
    . = ALIGN (4);
    _DispatchHandler_text_end = ABSOLUTE(.);
  } >L1_code_seg :L1_code_phdr

  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    *(.ResetVector.text)
    . = ALIGN (4);
    _ResetVector_text_end = ABSOLUTE(.);
  } >L1_code_seg :L1_code_phdr

  .L1.text : ALIGN(4)
  {
    _L1_text_start = ABSOLUTE(.);
    *(.L1.literal .L1.text)
    . = ALIGN (4);
    _L1_text_end = ABSOLUTE(.);
  } >L1_code_seg :L1_code_phdr

  .iram0.text : ALIGN(4)
  {
    _iram0_text_start = ABSOLUTE(.);
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    . = ALIGN (4);
    _iram0_text_end = ABSOLUTE(.);
    _memmap_seg_L1_code_end = ALIGN(0x8);
  } >L1_code_seg :L1_code_phdr

  _memmap_mem_L1_iram_max = ABSOLUTE(.);

  .L2.rodata : ALIGN(4)
  {
    _L2_rodata_start = ABSOLUTE(.);
    *(.L2.rodata)
    . = ALIGN (4);
    _L2_rodata_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .fft_twiddles.rodata : ALIGN(4)
  {
    _fft_twiddles_rodata_start = ABSOLUTE(.);
    *(.fft_twiddles.rodata)
    . = ALIGN (4);
    _fft_twiddles_rodata_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .sram0.rodata : ALIGN(4)
  {
    _sram0_rodata_start = ABSOLUTE(.);
    *(.sram0.rodata)
    . = ALIGN (4);
    _sram0_rodata_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .clib.rodata : ALIGN(4)
  {
    _clib_rodata_start = ABSOLUTE(.);
    *(.clib.rodata)
    . = ALIGN (4);
    _clib_rodata_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .rtos.rodata : ALIGN(4)
  {
    _rtos_rodata_start = ABSOLUTE(.);
    *(.rtos.rodata)
    . = ALIGN (4);
    _rtos_rodata_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(SORT(.rodata.sort.*))
    KEEP (*(SORT(.rodata.keepsort.*) .rodata.keep.*))
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    PROVIDE (__eh_frame_start = .);
    KEEP (*(.eh_frame))
    PROVIDE (__eh_frame_end = .);
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_L1_bss_start)
    LONG(_L1_bss_end)
    LONG(_bss_start)
    LONG(_bss_end)
    LONG(_L3_bss_start)
    LONG(_L3_bss_end)
    _bss_table_end = ABSOLUTE(.);
    . = ALIGN (4);
    _rodata_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .L2.text : ALIGN(4)
  {
    _L2_text_start = ABSOLUTE(.);
    *(.L2.literal .L2.text)
    . = ALIGN (4);
    _L2_text_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *(.literal.sort.* SORT(.text.sort.*))
    KEEP (*(.literal.keepsort.* SORT(.text.keepsort.*) .literal.keep.* .text.keep.* .literal.*personality* .text.*personality*))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    . = ALIGN (4);
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .clib.text : ALIGN(4)
  {
    _clib_text_start = ABSOLUTE(.);
    *(.clib.literal .clib.text)
    . = ALIGN (4);
    _clib_text_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .rtos.text : ALIGN(4)
  {
    _rtos_text_start = ABSOLUTE(.);
    *(.rtos.literal .rtos.text)
    . = ALIGN (4);
    _rtos_text_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .sram0.text : ALIGN(4)
  {
    _sram0_text_start = ABSOLUTE(.);
    *(.sram0.literal .sram0.text)
    . = ALIGN (4);
    _sram0_text_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .clib.data : ALIGN(4)
  {
    _clib_data_start = ABSOLUTE(.);
    *(.clib.data)
    . = ALIGN (4);
    _clib_data_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .clib.percpu.data : ALIGN(4)
  {
    _clib_percpu_data_start = ABSOLUTE(.);
    *(.clib.percpu.data)
    . = ALIGN (4);
    _clib_percpu_data_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .rtos.data : ALIGN(4)
  {
    _rtos_data_start = ABSOLUTE(.);
    *(.rtos.data)
    . = ALIGN (4);
    _rtos_data_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .rtos.percpu.data : ALIGN(4)
  {
    _rtos_percpu_data_start = ABSOLUTE(.);
    *(.rtos.percpu.data)
    . = ALIGN (4);
    _rtos_percpu_data_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .L2.data : ALIGN(4)
  {
    _L2_data_start = ABSOLUTE(.);
    *(.L2.data)
    . = ALIGN (4);
    _L2_data_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .sram0.data : ALIGN(4)
  {
    _sram0_data_start = ABSOLUTE(.);
    *(.sram0.data)
    . = ALIGN (4);
    _sram0_data_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .data : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(SORT(.data.sort.*))
    KEEP (*(SORT(.data.keepsort.*) .data.keep.*))
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))
    *(__llvm_prf_cnts)
    *(__llvm_prf_data)
    *(__llvm_prf_vnds)
    . = ALIGN (4);
    _data_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .L2.noload (NOLOAD) : ALIGN(4)
  {
    _L2_noload_start = ABSOLUTE(.);
    *(.L2.noload)
    . = ALIGN (4);
    _L2_noload_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .noload (NOLOAD) : ALIGN(4)
  {
    _noload_start = ABSOLUTE(.);
    *(.noload)
    . = ALIGN (4);
    _noload_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  __llvm_prf_names : ALIGN(4)
  {
    __llvm_prf_names_start = ABSOLUTE(.);
    *(__llvm_prf_names)
    . = ALIGN (4);
    __llvm_prf_names_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .note.gnu.build-id : ALIGN(4)
  {
    _note_gnu_build-id_start = ABSOLUTE(.);
    *(.note.gnu.build-id)
    . = ALIGN (4);
    _note_gnu_build-id_end = ABSOLUTE(.);
  } >L2_ram_fx_seg :L2_ram_fx_phdr

  .bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(SORT(.bss.sort.*))
    KEEP (*(SORT(.bss.keepsort.*) .bss.keep.*))
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *(.clib.bss)
    *(.clib.percpu.bss)
    *(.rtos.bss)
    *(.rtos.percpu.bss)
    *(.L2.bss)
    *(.sram0.bss)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _memmap_seg_L2_ram_fx_end = ALIGN(0x8);
  } >L2_ram_fx_seg :L2_ram_fx_bss_phdr

  _memmap_mem_L2_sram_max = ABSOLUTE(.);

  _memmap_mem_Sys_mmr_max = ABSOLUTE(.);

  _memmap_mem_SPI_max = ABSOLUTE(.);

  .L3.rodata : ALIGN(4)
  {
    _L3_rodata_start = ABSOLUTE(.);
    *(.L3.rodata)
    . = ALIGN (4);
    _L3_rodata_end = ABSOLUTE(.);
  } >L3_ram_fx_seg :L3_ram_fx_phdr

  .L3.text : ALIGN(4)
  {
    _L3_text_start = ABSOLUTE(.);
    *(.L3.literal .L3.text)
    . = ALIGN (4);
    _L3_text_end = ABSOLUTE(.);
  } >L3_ram_fx_seg :L3_ram_fx_phdr

  .L3.data : ALIGN(4)
  {
    _L3_data_start = ABSOLUTE(.);
    *(.L3.data)
    . = ALIGN (4);
    _L3_data_end = ABSOLUTE(.);
  } >L3_ram_fx_seg :L3_ram_fx_phdr

  .L3.noload (NOLOAD) : ALIGN(4)
  {
    _L3_noload_start = ABSOLUTE(.);
    *(.L3.noload)
    . = ALIGN (4);
    _L3_noload_end = ABSOLUTE(.);
  } >L3_ram_fx_seg :L3_ram_fx_phdr

  .L3.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _L3_bss_start = ABSOLUTE(.);
    *(.L3.bss)
    . = ALIGN (8);
    _L3_bss_end = ABSOLUTE(.);
    _memmap_seg_L3_ram_fx_end = ALIGN(0x8);
  } >L3_ram_fx_seg :L3_ram_fx_bss_phdr



  _memmap_mem_L3_sdram_max = ABSOLUTE(.);

  .debug  0 :  { *(.debug) }
  .line  0 :  { *(.line) }
  .debug_srcinfo  0 :  { *(.debug_srcinfo) }
  .debug_sfnames  0 :  { *(.debug_sfnames) }
  .debug_aranges  0 :  { *(.debug_aranges) }
  .debug_ranges   0 :  { *(.debug_ranges) }
  .debug_pubnames  0 :  { *(.debug_pubnames) }
  .debug_info  0 :  { *(.debug_info) }
  .debug_abbrev  0 :  { *(.debug_abbrev) }
  .debug_line  0 :  { *(.debug_line) }
  .debug_frame  0 :  { *(.debug_frame) }
  .debug_str  0 :  { *(.debug_str) }
  .debug_loc  0 :  { *(.debug_loc) }
  .debug_macinfo  0 :  { *(.debug_macinfo) }
  .debug_weaknames  0 :  { *(.debug_weaknames) }
  .debug_funcnames  0 :  { *(.debug_funcnames) }
  .debug_typenames  0 :  { *(.debug_typenames) }
  .debug_varnames  0 :  { *(.debug_varnames) }
  .debug.xt.map 0 :  { *(.debug.xt.map) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    *(.xt.prop)
    *(.xt.prop.*)
    *(.gnu.linkonce.prop.*)
  }
  .xt.lit 0 :
  {
    *(.xt.lit)
    *(.xt.lit.*)
    *(.gnu.linkonce.p.*)
  }
  .xtensa.info 0 :
  {
    *(.xtensa.info)
  }
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
  .comment 0 :
  {
    KEEP(*(.comment))
  }
  .note.GNU-stack 0 :
  {
    *(.note.GNU-stack)
  }
}

