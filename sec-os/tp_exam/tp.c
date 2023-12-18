/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <string.h>

/*| 0x10 0000  | Multiboot Header     |
| 0x10 0010   |   Kernel stack    |
| 0x10 2010      | Kernel        |*/


#define KERNEL_STACK_START 0x100010
#define KERNEL_STACK_SIZE 0x2000
#define KERNEL_START 0x102010
#define GDT_ENTRY_COUNT 5
#define TYPE_DATA_RW 3
#define TYPE_CODE_RX 11
#define FLAG_SEGMENT_PRESENT 1
#define FLAG_DEFAULT_AVL 1
#define FLAG_DEFAULT_LONGMODE 0
#define FLAG_DEFAULT_LENGTH 1
#define FLAG_DEFAULT_GRANULARITY 1
#define FLAG_DEFAULT_DESCRIPTOR_TYPE 1
#define FLAG_PRIVILEGE_R0 0
#define FLAG_PRIVILEGE_R3 3

void userland() {
    //Fonction test pour vérifier que le passage en mode userland fonctionne
   asm volatile ("mov %eax, %cr0");
}

/*** Pour l'instant, les fonctions sont prises des corrections du tp
Dans un premier temps, on définit les variables qui nous seront utiles.
Ensuite, on va créer une fonction qui va nous permettre de gérer des segments de codes, et de les initialiser.
Enfin, on va créer une fonction qui va nous permettre de créer une gdt initialement, et d'instancier des segments en son sein.
Dans un premier temps, on se contentera de 3 entrées dans la GDT pour le kernel: une pour le code, une pour les données, et une pour le TSS (TSS qu'on utilisera dans la partie tâches)
**/

/*On a pas besoin de beaucoups des informations ici. 
On initialise les segments en ring0
Quels bases et limites donner à ces segments ?
-> On se met en mode flat donc on commence à 0 et on finit à 0xffffffff
*/
#define gdt_flat_dsc(_entry, _ring, _type) \
  {                                        \
    (_entry)->raw = 0;                     \
    (_entry)->limit_1 = 0xffff;            \
    (_entry)->limit_2 = 0xf;               \
    (_entry)->type = _type;                \
    (_entry)->dpl = _ring;                 \
    (_entry)->d = 1;                       \
    (_entry)->g = 1;                       \
    (_entry)->s = 1;                       \
    (_entry)->p = 1;                       \
  }


void init_segment(seg_desc_t* seg, uint32_t base, uint32_t limit, uint8_t type, uint8_t s, uint8_t dpl, uint8_t p, uint8_t avl, uint8_t l, uint8_t d, uint8_t g) {
    seg->base_1 = base & 0xFFFF;
    seg->base_2 = (base >> 16) & 0xFF;
    seg->base_3 = (base >> 24) & 0xFF;
    seg->limit_1 = limit & 0xFFFF;
    seg->limit_2 = (limit >> 16) & 0xF;
    seg->type = type;
    seg->s = s;
    seg->dpl = dpl;
    seg->p = p;
    seg->avl = avl;
    seg->l = l;
    seg->d = d;
    seg->g = g;

}

void init_gdt(gdt_reg_t* gdtr, seg_desc_t* gdt, uint16_t size) {
    gdtr->addr = (uint32_t)gdt;
    gdtr->limit = size - 1;
}

void print_gdt_content(gdt_reg_t gdtr_ptr) {
    seg_desc_t* gdt_ptr;
    gdt_ptr = (seg_desc_t*)(gdtr_ptr.addr);
    int i=0;
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit)) {
        uint32_t start = gdt_ptr->base_3<<24 | gdt_ptr->base_2<<16 | gdt_ptr->base_1;
        uint32_t end;
        if (gdt_ptr->g) {
            end = start + ( (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1) <<12) + 4095;
        } else {
            end = start + (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1);
        }
        debug("%d ", i);
        debug("[0x%x ", start);
        debug("- 0x%x] ", end);
        debug("seg_t: 0x%x ", gdt_ptr->type);
        debug("desc_t: %d ", gdt_ptr->s);
        debug("priv: %d ", gdt_ptr->dpl);
        debug("present: %d ", gdt_ptr->p);
        debug("avl: %d ", gdt_ptr->avl);
        debug("longmode: %d ", gdt_ptr->l);
        debug("default: %d ", gdt_ptr->d);
        debug("gran: %d ", gdt_ptr->g);
        debug("\n");
        gdt_ptr++;
        i++;
    }
}

void tp() {
    gdt_reg_t gdtr_ptr;
    get_gdtr(gdtr_ptr);
	/*
	debug("GDT DE BASE \n");
    debug("GDT addr:  0x%x ", (unsigned int) gdtr_ptr.addr);
    debug("limit: %d\n", gdtr_ptr.limit);
    // res Q1
    // GDT addr:  0x8f8c limit: 39
    // end Q1
	*/
    // Q2 
    //print_gdt_content(gdtr_ptr);
    seg_desc_t my_gdt[GDT_ENTRY_COUNT];

    // Initialize GDT segments for kernel code and data segments in flatmode
    init_segment(&my_gdt[1], 0x0000, 0xffff, TYPE_CODE_RX, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R0, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
    init_segment(&my_gdt[2], 0x0000, 0xffff, TYPE_DATA_RW, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R0, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
    // Initialize GDT segments for user code and data segments in flatmode
    init_segment(&my_gdt[3], 0x00, 0xffff, TYPE_CODE_RX, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R3, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
    init_segment(&my_gdt[4], 0x00, 0xffff, TYPE_DATA_RW, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R3, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
    // end Q5
	// TODO
    gdt_reg_t my_gdtr;
    my_gdtr.addr = (long unsigned int)my_gdt;
    my_gdtr.limit = sizeof(my_gdt) - 1;
    set_gdtr(my_gdtr);
    // TODO
    // end Q6

    // Q7

    get_gdtr(my_gdtr);
    debug("GDT addr:  0x%x ", (unsigned int) my_gdtr.addr);
    debug("limit: %d\n", my_gdtr.limit);
    print_gdt_content(my_gdtr);
	/**
	uint16_t ds = get_ds();
	uint16_t es = get_es();
	uint16_t fs = get_fs();
	uint16_t gs = get_gs();
	debug("ds: 0x%x ", ds);
	debug("es: 0x%x ", es);
	debug("fs: 0x%x ", fs);
	debug("gs: 0x%x ", gs);
	*/
}

/*Cette fonction est inutile mais me sert de référence si j'ai besoin de revoir rapidement les flags*/
void donothing() {
	// Q1
    // GDTR and GDT configured by GRUB
    gdt_reg_t gdtr_ptr;
    get_gdtr(gdtr_ptr);
    debug("GDT addr:  0x%x ", (unsigned int) gdtr_ptr.addr);
    debug("limit: %d\n", gdtr_ptr.limit);
    // res Q1
    // GDT addr:  0x8f8c limit: 39
    // end Q1

    // Q2 
    print_gdt_content(gdtr_ptr);
	debug("LIMITE \n");
	debug("limit: %d\n", gdtr_ptr.limit);
	debug("limit: %d\n", gdtr_ptr.limit);
	debug("limit: %d\n", gdtr_ptr.limit);
    // res Q2
    /*
    0 [0x0 - 0xfff0] seg_t: 00000000000000000000000000000000 desc_t: 0 priv: 0 present: 0 avl: 0 longmode: 0 default: 0 gran: 0 
    1 [0x0 - 0xffffffff] seg_t: 00000000000000000000000000001011 desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 1 gran: 1 
    2 [0x0 - 0xffffffff] seg_t: 00000000000000000000000000000011 desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 1 gran: 1 
    3 [0x0 - 0xffff] seg_t: 00000000000000000000000000001111 desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 0 gran: 0 
    4 [0x0 - 0xffff] seg_t: 00000000000000000000000000000011 desc_t: 1 priv: 0 present: 1 avl: 0 longmode: 0 default: 0 gran: 0 
    -----------------------------------------------------------------------------------------------
    */ 
    //end Q2


    // Q4 
    // ségrégation de grub en mode flat...
    // end Q4

    // Q5
    seg_desc_t my_gdt[7];
    my_gdt[0].raw = 0ULL;
    my_gdt[1].limit_1 = 0xffff;   //:16;     /* bits 00-15 of the segment limit */
    my_gdt[1].base_1 = 0x0000;    //:16;     /* bits 00-15 of the base address */
    my_gdt[1].base_2 = 0x00;      //:8;      /* bits 16-23 of the base address */
    my_gdt[1].type = 11;//Code,RX //:4;      /* segment type */
    my_gdt[1].s = 1;              //:1;      /* descriptor type */
    my_gdt[1].dpl = 0; //ring0    //:2;      /* descriptor privilege level */
    my_gdt[1].p = 1;              //:1;      /* segment present flag */
    my_gdt[1].limit_2 = 0xf;      //:4;      /* bits 16-19 of the segment limit */
    my_gdt[1].avl = 1;            //:1;      /* available for fun and profit */
    my_gdt[1].l = 0; //32bits     //:1;      /* longmode */
    my_gdt[1].d = 1;              //:1;      /* default length, depend on seg type */
    my_gdt[1].g = 1;              //:1;      /* granularity */
    my_gdt[1].base_3 = 0x00;      //:8;      /* bits 24-31 of the base address */
    my_gdt[2].limit_1 = 0xffff;   //:16;     /* bits 00-15 of the segment limit */
    my_gdt[2].base_1 = 0x0000;    //:16;     /* bits 00-15 of the base address */
    my_gdt[2].base_2 = 0x00;      //:8;      /* bits 16-23 of the base address */
    my_gdt[2].type = 3; //data,RW //:4;      /* segment type */
    my_gdt[2].s = 1;              //:1;      /* descriptor type */
    my_gdt[2].dpl = 0; //ring0    //:2;      /* descriptor privilege level */
    my_gdt[2].p = 1;              //:1;      /* segment present flag */
    my_gdt[2].limit_2 = 0xf;      //:4;      /* bits 16-19 of the segment limit */
    my_gdt[2].avl = 1;            //:1;      /* available for fun and profit */
    my_gdt[2].l = 0; // 32 bits   //:1;      /* longmode */
    my_gdt[2].d = 1;              //:1;      /* default length, depend on seg type */
    my_gdt[2].g = 1;              //:1;      /* granularity */
    my_gdt[2].base_3 = 0x00;      //:8;      /* bits 24-31 of the base address */
    // end Q5

    // Q6
    gdt_reg_t my_gdtr;
    my_gdtr.addr = (long unsigned int)my_gdt;
    my_gdtr.limit = sizeof(my_gdt) - 1;
    set_gdtr(my_gdtr);
    // TODO
    // end Q6

    // Q7
    get_gdtr(my_gdtr);
    debug("GDT addr:  0x%x ", (unsigned int) my_gdtr.addr);
    debug("limit: %d\n", my_gdtr.limit);
    print_gdt_content(my_gdtr);
    // end Q7


}
