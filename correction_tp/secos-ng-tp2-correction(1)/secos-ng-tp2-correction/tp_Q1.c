/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>

void bp_handler() {
}

void bp_trigger() {
}

void tp() {
   // Q1
   idt_reg_t idtr;
   get_idtr(idtr);
   debug("IDT @ 0x%x\n", (unsigned int) idtr.addr);
   // end Q1
}

// expected output:
//   Booting 'SecOS'
// 
// root   (hd0,0)
//  Filesystem type is fat, partition type 0x6
// kernel /kernel.elf
//    [Multiboot-elf, <0x300000:0xc:0x0>, <0x300010:0x0:0x2000>, <0x302010:0x28a0:
// 0xc30>, shtab=0x3062d0, entry=0x303010
// secos-82ee21f-5788141 (c) Airbus
// IDT @ 0x304ce0
