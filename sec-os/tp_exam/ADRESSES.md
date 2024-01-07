/*
| 0x10 0000      | Multiboot Header         |
| 0x10 0010      | Kernel stack             |
| 0x10 2010      | Kernel                   |
| 0x20 0000      | Kernel page directory    |
| 0x20 1000      | Kernel page table        |
| 0x30 0000      | User1 page directory     |
| 0x30 1000      | User1 page table         |
| 0x37 0000      | User2 page directory     |
| 0x37 1000      | User2 page table         |
| 0x50 0000      | Shared memory            |

*/