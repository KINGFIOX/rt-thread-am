/* Data sections (FSymTab, VSymTab, .rti_fn*, UtestTcTab, __am_apps.data*)
   are now included directly in npc-linker.ld's .text output section
   to avoid VMA/LMA alignment mismatch when copying from flash to SDRAM. */

SECTIONS {
  .bss.extra : {
    . = ALIGN(8);
    __am_apps_bss_start = .;
    *(__am_apps.bss*)
    *(__am_apps.sbss*)
    *(__am_apps.scommon*)
    __am_apps_bss_end = .;
    . = ALIGN(8);
  } > dram
}
INSERT BEFORE .bss;
