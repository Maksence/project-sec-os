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
}
