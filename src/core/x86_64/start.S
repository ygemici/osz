.section .text
.global _start
.extern main

/**********************************************************************
 *                     OS/Z Loader entry point                        *
 **********************************************************************

    Registers
        rax     magic 'BOOTBOOT' (0x544f4f42544f4f42)
        rbx     virtual address of bootparams
        rcx     virtual address of environment
        rdx     virtual address of framebuffer
*/
_start:
    jmp    main

