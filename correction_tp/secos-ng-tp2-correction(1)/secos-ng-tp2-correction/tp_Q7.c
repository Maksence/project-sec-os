/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>

void bp_handler() {
    // Q7
    uint32_t val;
    asm volatile ("mov 4(%%ebp), %0":"=r"(val));
    debug("EIP = 0x%x\n", (unsigned int) val);
    // cette valeur est l'adresse dans bp_trigger
    // qui suit l'instruction int3.
    // end Q7
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


// output result:
//   Booting 'SecOS'
// 
// root   (hd0,0)
//  Filesystem type is fat, partition type 0x6
// kernel /kernel.elf
//    [Multiboot-elf, <0x300000:0xc:0x0>, <0x300010:0x0:0x2000>, <0x302010:0x2940:
// 0xc30>, shtab=0x3062d0, entry=0x303010
// secos-82ee21f-5788141 (c) Airbus
// IDT @ 0x304d80
// EIP = 0x303ff0
// ...

// $ objdump -D ./kernel.elf |less
// 00303fec <bp_trigger>:
// 303fec:       55                      push   %ebp
// 303fed:       89 e5                   mov    %esp,%ebp
// 303fef:       cc                      int3   
// 303ff0:       90                      nop <------------------------ ebp-4 = saved EIP!
// 303ff1:       5d                      pop    %ebp
// 303ff2:       c3                      ret    

