/* GPLv2 (c) Airbus */
#include <debug.h>

void tp() {
	// TODO
	gdt_reg_t gdtr_ptr;
	get_gdtr(gdtr_ptr);
	debug("GDT addr:  0x%x ", (unsigned int) gdtr_ptr.addr);
	debug("limit: %d\n", gdtr_ptr.limit);

	print_gdt_content(gdtr_ptr);

	uint16_t ds = get_ds();
	uint16_t es = get_es();
	uint16_t fs = get_fs();
	uint16_t gs = get_gs();
	debug("ds: 0x%x ", ds);
	debug("es: 0x%x ", es);
	debug("fs: 0x%x ", fs);
	debug("gs: 0x%x ", gs);
}
