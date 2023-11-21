/* GPLv2 (c) Airbus */
#include <debug.h>
#include <intr.h>

void bp_handler() {
    // Q2
    debug("#BP handling\n");
    // end Q2
}

void bp_trigger() {
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
}
