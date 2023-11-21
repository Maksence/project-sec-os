/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>

void bp_handler() {
   asm volatile ("pusha"); // Q8
   debug("#BP handling\n");
   uint32_t eip;
   asm volatile ("mov 4(%%ebp), %0":"=r"(eip));
   debug("EIP = %x\n", (unsigned int) eip);
   asm volatile ("popa"); // Q9
   asm volatile ("leave; iret");
}

void bp_trigger() {
    // Q10
    asm volatile ("int3");
    debug("after bp triggered\n");
    // end Q10
}

void tp() {
    idt_reg_t idtr;
    get_idtr(idtr);
    debug("IDT @ 0x%x\n", (unsigned int) idtr.addr);

    int_desc_t *bp_dsc = &idtr.desc[3];
    bp_dsc->offset_1 = (uint16_t)((uint32_t)bp_handler);
    bp_dsc->offset_2 = (uint16_t)(((uint32_t)bp_handler)>>16);

    bp_trigger();
}

// output:

//   Booting 'SecOS'
// 
// root   (hd0,0)
//  Filesystem type is fat, partition type 0x6
// kernel /kernel.elf
//    [Multiboot-elf, <0x300000:0xc:0x0>, <0x300010:0x0:0x2000>, <0x302010:0x2980:
// 0xc30>, shtab=0x3062d0, entry=0x303010
// secos-82ee21f-5788141 (c) Airbus
// IDT @ 0x304dc0
// #BP handling
// EIP = 304007
// after bp triggered
// halted !
