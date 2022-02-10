/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */

_stext = ORIGIN(FLASH);
_Min_Heap_Size = 0 ;	/* required amount of heap  */
_Min_Stack_Size = 0x4000 ;	/* required amount of stack */
_estack = ORIGIN(RAM) + _Min_Stack_Size + _Min_Heap_Size;	/* start of "RAM" Ram type memory */
_sstack = ORIGIN(RAM); /* start of "RAM" Ram type memory */
__ram_end = ORIGIN(RAM) + LENGTH(RAM);
/* Memories definition */
MEMORY
{
    RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 640K
    FLASH    (rx)    : ORIGIN = 0x1010000,   LENGTH = 2944K
}

/* Sections */
SECTIONS
{
    /* The startup code into "FLASH" Rom type memory */
    .app_info :
    {
        . = ALIGN(512);
        KEEP(*(.app_info)) /* Startup code */
        . = ALIGN(512);
    } >FLASH
    .isr_vector :
    {
        . = ALIGN(4);
        __isr_start_address = .;
        KEEP(*(.isr_vector)) /* Startup code */
        . = ALIGN(4);
    } >FLASH

    /* The program code and other data into "FLASH" Rom type memory */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.glue_7)         /* glue arm to thumb code */
        *(.glue_7t)        /* glue thumb to arm code */
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;        /* define a global symbols at end of code */
    } >FLASH

    /* Constant data into "FLASH" Rom type memory */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
        . = ALIGN(4);
    } >FLASH

    .ARM.extab   : {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } >FLASH

    .ARM : {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } >FLASH

    .preinit_array     :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } >FLASH

    .preinit_fun_array :
    {
        . = ALIGN(4);
        __preinit_fun_array_start = .;
        KEEP (*(SORT(.preinit_fun_array.*)))
        KEEP (*(.preinit_fun_array*))
        __preinit_fun_array_end = .;
        . = ALIGN(4);
    } > FLASH
    
    .init_fun_array :
    {
        . = ALIGN(4);
        __init_fun_array_start = .;
        KEEP (*(SORT(.init_fun_array.*)))
        KEEP (*(.init_fun_array*))
        __init_fun_array_end = .;
        . = ALIGN(4);
    } > FLASH
    
    .task_fun_array :
    {
        . = ALIGN(4);
        __task_fun_array_start = .;
        KEEP (*(SORT(.task_fun_array.*)))
        KEEP (*(.task_fun_array*))
        __task_fun_array_end = .;
        . = ALIGN(4);
    } > FLASH

    /* User_heap_stack section, used to check that there is enough "RAM" Ram  type memory left */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM
    /* Initialized data sections into "RAM" Ram type memory */
    /* Used by the startup to initialize data */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */
        *(.RamFunc)        /* .RamFunc sections */
        *(.RamFunc*)       /* .RamFunc* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end */

    } >RAM AT> FLASH

    /* Uninitialized data section into "RAM" Ram type memory */
    . = ALIGN(4);
    .bss :
    {
        /* This is used by the startup in order to initialize the .bss section */
        _sbss = .;         /* define a global symbol at bss start */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;         /* define a global symbol at bss end */
        __bss_end__ = _ebss;
    } >RAM
    . = ALIGN(16);
    __os_heap_start = .;
    /* Remove information from the compiler libraries */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
