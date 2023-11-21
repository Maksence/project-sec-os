/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>

void bp_handler() {
    // Q2
    debug("#BP handling\n");
    // end Q2
}

void bp_trigger() {
    // Q4
    asm volatile ("int3");
    // end Q4
}

void tp() {
    // Q1
    idt_reg_t idtr;
    get_idtr(idtr);
    debug("IDT @ 0x%x\n", (unsigned int) idtr.addr);
    // end Q1

    // Q3
    int_desc_t *bp_dsc = &idtr.desc[3];
    bp_dsc->offset_1 = (uint16_t)((uint32_t)bp_handler);
    bp_dsc->offset_2 = (uint16_t)(((uint32_t)bp_handler)>>16);
    // end Q3

    // Q4
    bp_trigger();
    // end Q4
}


// output result, see gdb_script_Q4 for explanation
//   Booting 'SecOS'
// 
// root   (hd0,0)
//  Filesystem type is fat, partition type 0x6
// kernel /kernel.elf
//    [Multiboot-elf, <0x300000:0xc:0x0>, <0x300010:0x0:0x2000>, <0x302010:0x2920:
// 0xc30>, shtab=0x3062d0, entry=0x303010
// secos-82ee21f-5788141 (c) Airbus
// IDT @ 0x304d60
// #BP handling
// 
// IDT event
//  . int    #6
//  . error  0xffffffff
//  . cs:eip 0x8:0x4c
//  . ss:esp 0x304076:0x301fe8
//  . eflags 0x2
// 
// - GPR
// eax     : 0xe
// ecx     : 0x304961
// edx     : 0xd
// ebx     : 0x30
// esp     : 0x301fb4
// ebp     : 0x8
// esi     : 0x2bfc2
// edi     : 0x2bfc3
// 
// Exception: Invalid Opcode
// cr0 = 0x11
// cr4 = 0x0
// 
// -= Stack Trace =-
// 0xf000ff53
// fatal exception !
// QEMU: Terminated
