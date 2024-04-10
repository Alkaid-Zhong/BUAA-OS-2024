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

