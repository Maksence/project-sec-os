/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <string.h>
#include <pagemem.h>
#include <cr.h>
/*
| 0x10 0000      | Multiboot Header         |
| 0x10 0010      |   Kernel stack           |
| 0x10 2010      | Kernel                   |
| 0x20 0000      | Kernel page directory    |
| 0x20 1000      | Kernel page table        |
| 0x30 0000      | User1 page directory     |
| 0x30 1000      | User1 page table         |
| 0x37 0000      | User2 page directory     |
| 0x37 1000      | User2 page table         |
*/


#define KERNEL_STACK_START 0x100010
#define KERNEL_STACK_SIZE 0x2000
#define KERNEL_START 0x102010
#define GDT_ENTRY_COUNT 8

#define RING0_CODE 1
#define RING0_DATA 2
#define RING3_CODE 3
#define RING3_DATA 4
#define TSS_IDX 5

#define FLAG_SEGMENT_PRESENT 1
#define FLAG_DEFAULT_AVL 1
#define FLAG_DEFAULT_LONGMODE 0
#define FLAG_DEFAULT_LENGTH 1
#define FLAG_DEFAULT_GRANULARITY 1
#define FLAG_DEFAULT_DESCRIPTOR_TYPE 1
#define FLAG_PRIVILEGE_R0 0
#define FLAG_PRIVILEGE_R3 3

#define KERNEL_PGD 0x200000
#define KERNEL_PTB 0x201000
#define USER1_PGD 0x300000
#define USER1_PTB 0x301000
#define USER2_PGD 0x370000
#define USER2_PTB 0x371000
#define OFFSET_PTB 0x1000
#define SHARED_MEM_ADDRS 0x500000


seg_desc_t my_gdt[GDT_ENTRY_COUNT];
gdt_reg_t my_gdtr;
tss_t TSS;

void userland()
{

    // asm volatile ("mov %eax, %cr0");
    debug("\n >>>>> userland <<<<<\n");
}

void user1(){
    while(1){
        debug("\n >>>>> user1 <<<<<\n");
    }
}

void user2(){
    while(1){
        debug("\n >>>>> user2 <<<<<\n");
    }
}

/*
On initialise les segments en ring0
-> On se met en mode flat donc on commence à 0 et on finit à 0xffffffff
*/

void init_segment(seg_desc_t *seg, uint32_t base, uint32_t limit, uint8_t type, uint8_t s, uint8_t dpl, uint8_t p, uint8_t avl, uint8_t l, uint8_t d, uint8_t g)
{
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

void print_selectors()
{
    uint16_t ds = get_ds();
    uint16_t es = get_es();
    uint16_t fs = get_fs();
    uint16_t gs = get_gs();
    debug("ds: 0x%x ", ds);
    debug("es: 0x%x ", es);
    debug("fs: 0x%x ", fs);
    debug("gs: 0x%x ", gs);
}

void print_gdt_content(gdt_reg_t gdtr_ptr)
{
    seg_desc_t *gdt_ptr;
    gdt_ptr = (seg_desc_t *)(gdtr_ptr.addr);
    int i = 0;
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit))
    {
        uint32_t start = gdt_ptr->base_3 << 24 | gdt_ptr->base_2 << 16 | gdt_ptr->base_1;
        uint32_t end;
        if (gdt_ptr->g)
        {
            end = start + ((gdt_ptr->limit_2 << 16 | gdt_ptr->limit_1) << 12) + 4095;
        }
        else
        {
            end = start + (gdt_ptr->limit_2 << 16 | gdt_ptr->limit_1);
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

void init_gdt_segs()
{
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
    debug("GDT addr:  0x%x ", (unsigned int)my_gdtr.addr);
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
}

#define tss_dsc(_dSc_, _tSs_)                    \
    ({                                           \
        raw32_t addr = {.raw = _tSs_};           \
        (_dSc_)->raw = sizeof(tss_t);            \
        (_dSc_)->base_1 = addr.wlow;             \
        (_dSc_)->base_2 = addr._whigh.blow;      \
        (_dSc_)->base_3 = addr._whigh.bhigh;     \
        (_dSc_)->type = SEG_DESC_SYS_TSS_AVL_32; \
        (_dSc_)->p = 1;                          \
    })

void setup_mem_mapping(pde32_t *pgd, int first_ptb_offset, int task_num)
{
    debug("INITIALISATION DE LA PAGINATION\n");
    memset((void *)pgd, 0, PAGE_SIZE);

    pte32_t *ptb = (pte32_t *)((int)pgd + first_ptb_offset);

    // Initialize the first 1024 entries of the page table
    debug("addrs PTB[0] = %d\n", ptb[0].addr);
    for (int i = 0; i < 1024; i++)
    {
        pg_set_entry(&ptb[i], PG_KRN | PG_RW, i + task_num * 1024);
    }
    debug("PTB[1] = %d\n", ptb[1].raw);  

    // Initialize the first entry of the page directory
    pg_set_entry(&pgd[0], PG_KRN | PG_RW, page_nr(ptb));
    debug("PGD = %p\n", pgd);

    // Initialize the first entry of the shared page table
    pte32_t *ptb_shared = (pte32_t *)((int)pgd + first_ptb_offset + 4*1024);
    pg_set_entry(&ptb_shared[0], PG_KRN | PG_RW, page_nr(SHARED_MEM_ADDRS));
    pg_set_entry(&pgd[1], PG_KRN | PG_RW, page_nr(ptb_shared));
    debug("SHARED MEMORY ADDRESS = %d\n", ptb_shared[0].addr);
}

void enable_pagination()
{
    // Enabling paging
    uint32_t cr0 = get_cr0();
    set_cr0(cr0 | CR0_PG);
}

void tp()
{
    init_gdt_segs();
    // Initalize TSS segment
    set_ds(gdt_krn_seg_sel(RING3_DATA));
    set_es(gdt_krn_seg_sel(RING3_DATA));
    set_fs(gdt_krn_seg_sel(RING3_DATA));
    set_gs(gdt_krn_seg_sel(RING3_DATA));
    TSS.s0.esp = get_ebp();
    TSS.s0.ss = gdt_krn_seg_sel(RING0_DATA);
    tss_dsc(&my_gdt[TSS_IDX], (offset_t)&TSS);
    set_tr(gdt_krn_seg_sel(TSS_IDX));
    debug("\n TSS was setup \n");
    
    // Initialize page tables for paging
    setup_mem_mapping((pde32_t *)KERNEL_PGD, OFFSET_PTB, 0);
    setup_mem_mapping((pde32_t *) USER1_PGD, OFFSET_PTB, 1);
    setup_mem_mapping((pde32_t *) USER2_PGD, OFFSET_PTB, 2);

    set_cr3(KERNEL_PGD);
    enable_pagination();
    debug("\n >>>>> Tâches effectuées: <<<<<\n -mise en place segmentation et pagination \n -création d'une PGD par tâches et une pour le kernel. \n -mise en place d'un espace mémoire commun aux tâches non testé. \n");
    debug("\n >>>>> end of tp <<<<<\n");
}
