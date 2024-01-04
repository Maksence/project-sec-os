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

#define RING0_CODE  1
#define RING0_DATA  2
#define RING3_CODE  3
#define RING3_DATA  4
#define TSS_IDX  5

//#define TYPE_DATA_RW SEG_DESC_DATA_RW
//SEG_DESC_DATA_RW
//#define TYPE_CODE_RX SEG_DESC_DATA_RX
//SEG_DESC_CODE_XR
#define FLAG_SEGMENT_PRESENT 1
#define FLAG_DEFAULT_AVL 1
#define FLAG_DEFAULT_LONGMODE 0
#define FLAG_DEFAULT_LENGTH 1
#define FLAG_DEFAULT_GRANULARITY 1
#define FLAG_DEFAULT_DESCRIPTOR_TYPE 1
#define FLAG_PRIVILEGE_R0 0
#define FLAG_PRIVILEGE_R3 3


seg_desc_t my_gdt[GDT_ENTRY_COUNT];
gdt_reg_t my_gdtr;
tss_t      TSS;

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

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

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



void print_selectors(){
    uint16_t ds = get_ds();
	uint16_t es = get_es();
	uint16_t fs = get_fs();
	uint16_t gs = get_gs();
	debug("ds: 0x%x ", ds);
	debug("es: 0x%x ", es);
	debug("fs: 0x%x ", fs);
	debug("gs: 0x%x ", gs);
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

void init_gdt_segs(){
    // Init GDT
    // First element of the GDT is always empty
    my_gdt[0].raw = 0ULL;
    // Initialize GDT segments for kernel code and data segments in flatmode
    init_segment(&my_gdt[1], 0x0000, 0xffff, SEG_DESC_CODE_XR, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R0, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
    init_segment(&my_gdt[2], 0x0000, 0xffff, SEG_DESC_DATA_RW, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R0, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
    // Initialize GDT segments for user code and data segments in flatmode
    init_segment(&my_gdt[3], 0x00, 0xffff, SEG_DESC_CODE_XR, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R3, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
    init_segment(&my_gdt[4], 0x00, 0xffff, SEG_DESC_DATA_RW, FLAG_DEFAULT_DESCRIPTOR_TYPE, FLAG_PRIVILEGE_R3, FLAG_SEGMENT_PRESENT, FLAG_DEFAULT_AVL, FLAG_DEFAULT_LONGMODE, FLAG_DEFAULT_LENGTH, FLAG_DEFAULT_GRANULARITY);
   
	// Load the GDT by changing the adress and limit in gdtr
    gdt_reg_t my_gdtr;  

    my_gdtr.addr = (long unsigned int)my_gdt;
    my_gdtr.limit = sizeof(my_gdt) - 1;
    set_gdtr(my_gdtr);
    
    // Check GDT content    
    get_gdtr(my_gdtr);
    debug("GDT addr:  0x%x ", (unsigned int) my_gdtr.addr);
    debug("limit: %d\n", my_gdtr.limit);
    print_gdt_content(my_gdtr);
    
    // Init registers on kernel segments
    set_cs(gdt_krn_seg_sel(RING0_CODE));
    set_ss(gdt_krn_seg_sel(RING0_DATA));
    set_ds(gdt_krn_seg_sel(RING0_DATA));
    set_es(gdt_krn_seg_sel(RING0_DATA));
    set_fs(gdt_krn_seg_sel(RING0_DATA));
    set_gs(gdt_krn_seg_sel(RING0_DATA));
            
    print_selectors();
    debug("\n AAA\n");
    /*
    TSS.s0.esp = get_ebp();
    TSS.s0.ss  = gdt_krn_seg_sel(RING0_DATA);
    tss_dsc(&my_gdt[TSS_IDX], (offset_t)&TSS);
    set_tr(gdt_krn_seg_sel(TSS_IDX));
    */
	// END INIT GDT
    
}


#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

void tp() {
    init_gdt_segs();
    debug("\n BBB \n");
    //Switch to user mode
    set_ds(gdt_krn_seg_sel(RING3_DATA));
    set_es(gdt_krn_seg_sel(RING3_DATA));
    set_fs(gdt_krn_seg_sel(RING3_DATA));
    set_gs(gdt_krn_seg_sel(RING3_DATA));
    debug("\n CCC \n");

    TSS.s0.esp = get_ebp();
    TSS.s0.ss  = gdt_krn_seg_sel(RING0_DATA);
    tss_dsc(&my_gdt[TSS_IDX], (offset_t)&TSS);
    debug("\n DDD \n");
    set_tr(gdt_krn_seg_sel(TSS_IDX));
    debug("\n TSS setup \n");
    //print_selectors();
       
    asm volatile (
    "push %0    \n" // ss
    "push %%ebp \n" // esp
    "pushf      \n" // eflags
    "push %1    \n" // cs
    "push %2    \n" // eip
    // end Q2
    // Q3
    "iret"
    ::
        "i"(gdt_krn_seg_sel(RING3_DATA)),
        "i"(gdt_krn_seg_sel(RING3_CODE)),
        "r"(&userland)
    ); 



    debug("\n >>>>> end of tp <<<<<\n");
}

