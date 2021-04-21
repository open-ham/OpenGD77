/*
 * Generated Linker script file for MK22FN512xxx12
 */

MEMORY
{
  /* Define each memory region */
  PROGRAM_FLASH (rx) : ORIGIN = 0x4000, LENGTH = 0x7B800 /* 0x00004000-0x007f7ff (alias Flash) */  
  SRAM_UPPER (rwx) : ORIGIN = 0x20000000, LENGTH = 0x10000 /* 64K bytes (alias RAM) */  
/*  SRAM_LOWER_BOTTOM (rwx) : ORIGIN = 0x1fff0000, LENGTH = 0x6b60 */ /* (alias RAM2_BOTTOM) */
/*  SRAM_LOWER_AMBE (rwx) : ORIGIN = 0x1fff6b60, LENGTH = 0x2228 */ /* (alias RAM2_AMBE) */
/*  SRAM_LOWER_TOP (rwx) : ORIGIN = 0x1fff8d88, LENGTH = 0x7278 */ /* (alias RAM2_TOP) */
   SRAM_LOWER (rwx) : ORIGIN = 0x1fff0000, LENGTH = 0x10000  /* (alias RAM2) */  
}

  /* Define a symbol for the top of each memory region */
  __base_PROGRAM_FLASH = 0x4000 ; /* PROGRAM_FLASH */  
  __base_Flash = 0x4000 ; /* Flash */  
  __top_PROGRAM_FLASH = 0x4000 + 0x7B800 ; /* leave out bootloader and CPU-UUID area*/  
  __top_Flash = 0x4000 + 0x7B800 ; /* leave out bootloader and CPU-UUID area*/  
  __base_SRAM_UPPER = 0x20000000  ; /* SRAM_UPPER */  
  __base_RAM = 0x20000000 ; /* RAM */  
  __top_SRAM_UPPER = 0x20000000 + 0x10000 ; /* 64K bytes */  
  __top_RAM = 0x20000000 + 0x10000 ; /* 64K bytes */  
  
  __base_SRAM_LOWER = 0x1fff0000  ; /* SRAM_LOWER */  
  __base_RAM2 = 0x1fff0000 ; /* RAM2 */  
  __top_SRAM_LOWER = 0x1fff0000 + 0x10000 ; /* 64K bytes */  
  __top_RAM2 = 0x1fff0000 + 0x10000 ; /* 64K bytes */  
