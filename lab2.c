/*
对于给定的页目录 pgdir，统计其包含的所有二级页表中满足以下条件的页表项：

页表项有效；
页表项映射的物理地址为给定的 Page *pp 对应的物理地址；
页表项的权限包含给定的权限 perm_mask。
*/
u_int page_perm_stat(Pde *pgdir, struct Page *pp, u_int perm_mask) {
	int count = 0;//统计满足条件的页表项的数量
	Pde *pde;
	Pte *pte;
	for (int i = 0; i < 1024; i++) {
		pde = pgdir + i;
		if(!(*pde & PTE_V)) { //当前页目录是否有效
			continue;
		}
	
		for (int j = 0; j< 1024;j++ ){
			pte = (Pte*)KADDR(PTE_ADDR(*pde)) + j;
			if (!(*pte & PTE_V)) { ////当前页表是否有效
				continue;
			}
			if (((perm_mask | (*pte))== (*pte)) 
                && (((u_long)(page2pa(pp)))>>12) == (((u_long)(PTE_ADDR(*pte)))>>12))
				count++;
            /*该层if判断条件等价于
            (perm_mask & (*pte))== perm_mask
            (page2pa(pp) == PTE_ADDR(*pte))
            */
		}
    }
	return count;
}	

// Interface for 'Passive Swap Out'
struct Page *swap_alloc(Pde *pgdir, u_int asid) {
	// Step 1: Ensure free page
	if (LIST_EMPTY(&page_free_swapable_list)) {
		/* Your Code Here (1/3) */
		u_char  *disk_swap = disk_alloc();
		u_long da = (u_long)disk_swap;
		struct Page *p = pa2page(SWAP_PAGE_BASE);//这里策略是只换0x3900000处的页
		for (u_long i = 0; i < 1024; i++) { //改变所有页表中映射到0x3900000的页表项
			Pde *pde = pgdir + i;
			if ((*pde) & PTE_V) {
				for (u_long j = 0; j < 1024; j++) {
					Pte *pte = (Pte*)KADDR(PTE_ADDR(*pde)) + j;
					if (((*pte) & PTE_V) && (PTE_ADDR(*pte) == SWAP_PAGE_BASE) ) {
						(*pte) = ((da / BY2PG) << 12) | ((*pte) & 0xfff);
						//上式作用等价于(*pte) = PTE_ADDR(da) | ((*pte) & 0xfff);保留后面的所有12位offset
						(*pte) = ((*pte) | PTE_SWP)  & (~PTE_V);
						tlb_invalidate(asid, (i << 22) | (j << 12) ); //tlb_invalidate(asid, va);
					}
				}
			}
		}
		memcpy((void *)da, (void *)page2kva(p), BY2PG); 
		//这里没有再删掉页控制块p对应的内容，是因为跳出该if之后，会执行剩下的语句，其中倒数第2句会帮助情空
		LIST_INSERT_HEAD(&page_free_swapable_list, p, pp_link);
	}

	// Step 2: Get a free page and clear it
	struct Page *pp = LIST_FIRST(&page_free_swapable_list);
	LIST_REMOVE(pp, pp_link);
	memset((void *)page2kva(pp), 0, BY2PG);

	return pp;
}

// Interfaces for 'Active Swap In'
static int is_swapped(Pde *pgdir, u_long va) {
	/* Your Code Here (2/3) */
	Pde *pde = pgdir + PDX(va);
	if (*pde & PTE_V) {
		Pte *pte = (Pte*)KADDR(PTE_ADDR(*pde)) + PTX(va);
		if ((*pte & PTE_SWP) && !(*pte & PTE_V)) {
			return 1;
		}
	}
	return 0;
}

static void swap(Pde *pgdir, u_int asid, u_long va) {
	/* Your Code Here (3/3) */
	struct Page *pp = swap_alloc(pgdir, asid); //可用于交换的内存块的页控制块
	u_long da = PTE_ADDR(*((Pte*)KADDR(PTE_ADDR(*(pgdir + PDX(va)))) + PTX(va))); //外存地址
	memcpy((void *)page2kva(pp), (void *)da, BY2PG);

	for (u_long i = 0; i < 1024; i++) { //所有页表项记录的映射在外存的页表项改到新申请下来的swap的内存地址
		Pde *pde = pgdir + i;
		if (*pde & PTE_V) {
			for (u_long j = 0; j < 1024; j++) {
				Pte *pte = (Pte*)KADDR(PTE_ADDR(*pde)) + j;
				if (!(*pte & PTE_V) && (*pte & PTE_SWP) && (PTE_ADDR(*pte) == da)) {
					//以下三句话含义均可类比于 swap_alloc
					(*pte) = ((page2pa(pp) / BY2PG) << 12) | ((*pte) & 0xfff);
					(*pte) = ((*pte) & ~PTE_SWP) | PTE_V;
					tlb_invalidate(asid, (i << 22) | (j << 12) );
				}
			}
		}
	}
	disk_free((u_char *)da);
	return;
}

/*
tlbr：以 Index 寄存器中的值为索引,读出 TLB 中对应的表项到 EntryHi 与 EntryLo。

tlbwi：以 Index 寄存器中的值为索引,将此时 EntryHi 与 EntryLo 的值写到索引指定的 TLB 表项中。

tlbwr：将 EntryHi 与 EntryLo 的数据随机写到一个 TLB 表项中（此处使用 Random 寄存器来“随机”指定表项，Random 寄存器本质上是一个不停运行的循环计数器）。

tlbp：根据 EntryHi 中的 Key（包含 VPN 与 ASID），查找 TLB 中与之对应的表项，并将表项的索引存入 Index 寄存器（若未找到匹配项，则 Index 最高位被置 1）。

内核软件操作TLB的流程：1.填写CP0寄存器；2.使用TLB相关指令

用户态下访存受限，我理解的是不能访问kseg1（用于访问外设）和kseg2（仅内核使用），也不可以访问kseg0。

在用户空间访问时，虚拟地址到物理地址的转化均通过TLB进行

7.3TLB重填流程
TLB重填发生在CPU发出虚拟地址，想要在TLB中查找，但没有找到，之后程序会将对应的物理地址重填入TLB

1.确定此时的一级页表基地址：mCONTEXT中存储了当前进程一级页表基地址位于 kseg0 的虚拟地址  方便确定一级页表的物理地址

2.从 BadVaddr 中取出引发 TLB 缺失的虚拟地址, 并确定其对应的一级页表偏移量（高 10 位）

3.根据一级页表偏移量, 从一级页表中取出对应的表项：此时取出的表项由二级页表基地址的物理地址与权限位组成；

4.判定权限位: 若权限位显示该表项无效（无 PTE_V ）, 则调用 page_out ，随后回到第一步

5.确定引发 TLB 缺失的虚拟地址对应的二级页表偏移量（中间 10 位），与先前取得的二级页表基地址的物理地址共同确认二级页表项的物理地址

6.将二级页表项物理地址转为位于 kseg0 的虚拟地址（高位补 1），随后页表中取出对应的表项：此时取出的表项由物理地址与权限位组成；

7.判定权限位: 若权限位显示该表项无效（无 PTE_V），则调用 page_out ，随后回到第一步；（PTE_COW 为写时复制权限位，将在 lab4 中用到，此时将所有页表项该位视为 0 即可）

8.将物理地址存入 EntryLo , 并调用 tlbwr 将此时的 EntryHi 与 EntryLo 写入到 TLB 中（EntryHi 中保留了虚拟地址相关信息）。
*/