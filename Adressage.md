| Adress     | Content   |
| --------   | -------   |
| 0x10 0000  | Multiboot Header     |
| 0x10 0010   |   Kernel stack    |
| 0x10 2010      | Kernel        |

MBH = Multi Boot Header, l’endroit où on met pleins d’infos au lancement par grub. Permet à grub de lancer notre OS (pas la peine de se préocupper du contenu normalement)
Kernel Stack
On charge le programme à 0x100000, on ajoute le .mbh sur 4, on aligne à 0x10 pour le .stack qui fait 0x2000 et on arrive à 0x102010
