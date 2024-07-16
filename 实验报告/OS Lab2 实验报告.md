# OS Lab2 实验报告

## 思考题

### Thinking2.1

在编写的 C 程序中，指针变量中存储的地址被视为虚拟地址，还是物理地址？MIPS 汇编程序中 lw 和 sw 指令使用的地址被视为虚拟 地址，还是物理地址？ 

> 在C程序中，指针变量中的都为虚拟地址
>
> lw和sw的值也为虚拟地址

### Thinking 2.2

> • 从可重用性的角度，阐述用宏来实现链表的好处。 

需要实现链表的时候只需要保证链表内有`field`字段，并且`field`属性包含`le_prev`和`le_next`属性就可以，这样就可以在不同的结构体里面都实现链表，可以重复使用。（本实验中的`field`实际上是`pp_link`）

> • 查看实验环境中的 /usr/include/sys/queue.h，了解其中单向链表与循环链表的实现，比较它们与本实验中使用的双向链表，分析三者在插入与删除操作上的性能差异。

单向链表的直接插入头部和删除头部只有`O(1)`时间复杂度，但是如果想插入到指定`pp`的前面，则需要遍历，复杂度为`O(n)`。不过因为节约了一个指针，空间占用会小一点。

双向链表则都是`O(1)`。

循环链表只是将尾部和头部相连，插入和删除的时间没有太大区别。

### Thinking 2.3

>请阅读 include/queue.h 以及 include/pmap.h, 将 Page_list 的结构梳 理清楚，选择正确的展开结构。
>
>```c
>C:
>struct Page_list {
>	struct Page_LIST_ENTEY_t{
>		struct Page {
>			struct Page *le_next;
>			struct Page **le_prev;
> 		} pp_link;
>    		u_short pp_ref;
>    	}* lh_first;
>}
>```

### Thinking 2.4

> • 请阅读上面有关 TLB 的描述，从虚拟内存和多进程操作系统的实现角度，阐述 ASID 的必要性。 

由于在多线程系统中，对于不同进程，相同的虚拟地址各自占用不同的物理地址空间，所以同一虚拟地址通常映射到不同的物理地址。TLB 事实上构建了一个映射 `< VPN, ASID > TLB −−−→ < PFN, N, D, V, G >`。因此TLB中装着不同进程的页表项，ASID用于区别不同进程的页表项。

 没有ASID机制的情况下每次进程切换需要地址空间切换的时候都需要清空TLB。

> • 请阅读 MIPS 4Kc 文档《MIPS32® 4K™ Processor Core Family Software User’s Manual》的 Section 3.3.1 与 Section 3.4，结合 ASID 段的位数，说明 4Kc 中可容纳 不同的地址空间的最大数量。

ASID6位，容纳64个不同进程。

### Thinking 2.5

> • tlb_invalidate 和 tlb_out 的调用关系？ 

`tlb_invalidate(u_int asid, u_long va)`调用`tlb_out((va & ~GENMASK(PGSHIFT, 0)) | (asid & (NASID - 1)))`

> • 请用一句话概括 tlb_invalidate 的作用。

清除TLB中之前的`< VPN, ASID > TLB −−−→ < PFN, N, D, V, G >`的对应关系

>  • 逐行解释 tlb_out 中的汇编代码。

```assembly
LEAF(tlb_out) /* 叶函数 */
.set noreorder
	/* 先保存CP0_ENTRYHI的VPN和ASID存入$t0 */
	mfc0    t0, CP0_ENTRYHI
	/* 再把调用时候传入的参数VPN和ASID写入CP0_ENTRYHI */
	mtc0    a0, CP0_ENTRYHI
	nop
	/* Step 1: Use 'tlbp' to probe TLB entry */
	/* Exercise 2.8: Your code here. (1/2) */
	nop
	/* 根据当前的CP0_ENTRYHI，查找TLB与之对应的表项，并将表项的索引存入CP0_INDEX */
	tlbp	
	nop
	/* Step 2: Fetch the probe result from CP0.Index */
	/* 把tlbp的结果保存到$t1s */
	mfc0    t1, CP0_INDEX
.set reorder
	/* 如果没有这个表项，跳转到NO_SUCH_ENTRY */
	bltz    t1, NO_SUCH_ENTRY
.set noreorder
	/* 清除ENTRYHI ENTRYLO0 ENTRYLO1 */
	mtc0    zero, CP0_ENTRYHI
	mtc0    zero, CP0_ENTRYLO0
	mtc0    zero, CP0_ENTRYLO1
	nop
	/* Step 3: Use 'tlbwi' to write CP0.EntryHi/Lo into TLB at CP0.Index  */
	/* Exercise 2.8: Your code here. (2/2) */
	/* 把为零的CP0_ENTRYHI和CP0_ENTRYLO赋值回表项 */。
	tlbwi
.set reorder

NO_SUCH_ENTRY:
	/* 把原来的VPN和ASID赋值回CP0_ENTRYHI */
	mtc0    t0, CP0_ENTRYHI
	j       ra
END(tlb_out)
```

### Thinking 2.6

>  简单了解并叙述 X86 体系结构中的内存管理机制，比较 X86 和 MIPS 在内存管理上的区别。

**X86 内存管理机制：**

1. **分段（Segmentation）**：X86 使用分段机制来管理内存。在这种模式下，内存被划分为多个段，每个段有不同的权限和特性。常见的段包括代码段、数据段、栈段等。分段机制可以提供一定的内存保护和安全性，但也容易导致内存碎片化。
2. **分页（Paging）**：X86 同时也支持分页机制。在分页模式下，内存被划分为大小固定的页面，通常为4KB。分页机制可以提供更灵活的内存管理，支持虚拟内存和内存映射，以及更细粒度的内存保护。
3. **物理地址扩展（Physical Address Extension，PAE）**：X86 架构还支持物理地址扩展，允许操作系统访问超过4GB的物理内存。

**MIPS 内存管理机制：**

1. **分段和分页的结合**：MIPS 架构通常采用分段和分页的结合方式进行内存管理。与X86相比，MIPS对分段的支持较弱，通常只有一个代码段和一个数据段。
2. **固定大小的页**：MIPS 架构通常使用固定大小的页面进行内存管理，通常为4KB或更大。这种方法简化了内存管理，但可能会导致内存浪费。
3. **TLB（Translation Lookaside Buffer）**：MIPS 使用TLB来加速虚拟地址到物理地址的转换。TLB是一个高速缓存，存储了虚拟地址到物理地址的映射，可以显著提高地址转换的速度。

**区别比较：**

1. **内存管理机制**：X86 使用分段和分页的结合方式，而MIPS主要依赖于分页机制。X86 的分段机制相对更复杂，但也更灵活，而MIPS 的分页机制则更简单直接。
2. **TLB 的使用**：MIPS 使用TLB来加速地址转换，而X86 也有TLB，但其内存管理机制更复杂，TLB的实现也可能会有所不同。
3. **物理地址扩展**：X86 支持PAE，可以访问更大的物理内存，而MIPS 架构通常没有这样的扩展机制。

## 难点分析

这次实验课下的难度就很高，感觉每个函数都很难，一开始不理解各个参数到底对应的虚拟地址还是物理地址，也搞不明白各个变量到底存在哪，到底指代的什么。后来还是一个个`printk`出来才大概明白了，各个变量到底是怎么回事。

全局变量的存放一般都在`0x80400000`之前，这个地址就是在链接脚本中`.end`对应的地址，这样也就可以理解在`page_init()`函数中

```c
void page_init(void) {
	/* Step 1: Initialize page_free_list. */
	/* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
	/* Exercise 2.3: Your code here. (1/4) */
	LIST_INIT(&page_free_list);

	/* Step 2: Align `freemem` up to multiple of PAGE_SIZE. */
	/* Exercise 2.3: Your code here. (2/4) */
	freemem = ROUND(freemem, PAGE_SIZE);

	/* Step 3: Mark all memory below `freemem` as used (set `pp_ref` to 1) */
	/* Exercise 2.3: Your code here. (3/4) */
	u_long usedPages = PPN(PADDR(freemem)); // alloced pages
	// PPN(pa) physical page number ? = pa >> PGSHIFT
	u_long i;
	for (i = 0; i < usedPages; i++) {
		pages[i].pp_ref = 1;
	}

	/* Step 4: Mark the other memory as free. */
	/* Exercise 2.3: Your code here. (4/4) */
	for(; i < npage; i++) {
		pages[i].pp_ref = 0;
		LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}
}
```

为什么要把`usedPages`前面的页的`pp_ref`设置为1，因为已经被全局变量使用了。

之后的`pgdir_walk`也很重要

```c
static int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
	Pde *pgdir_entryp;
	struct Page *pp;

	/* Step 1: Get the corresponding page directory entry. */
	/* Exercise 2.6: Your code here. (1/3) */

	pgdir_entryp = pgdir + PDX(va);

	/* Step 2: If the corresponding page table is not existent (valid) then:
	 *   * If parameter `create` is set, create one. Set the permission bits 'PTE_C_CACHEABLE |
	 *     PTE_V' for this new page in the page directory. If failed to allocate a new page (out
	 *     of memory), return the error.
	 *   * Otherwise, assign NULL to '*ppte' and return 0.
	 */
	/* Exercise 2.6: Your code here. (2/3) */

	if (((*pgdir_entryp) & PTE_V) == 0) {
		if (create == 1) {
			if (page_alloc(&pp) == -E_NO_MEM) {
				*ppte = NULL;
				return -E_NO_MEM;
			} else {
				*pgdir_entryp = page2pa(pp) | PTE_C_CACHEABLE | PTE_V;
				pp -> pp_ref++;
			}
		} else {
			*ppte = NULL;
			return -E_NO_MEM;
		}
	}

	/* Step 3: Assign the kernel virtual address of the page table entry to '*ppte'. */
	/* Exercise 2.6: Your code here. (3/3) */
	// pgdir_entryp -> Pte'pa	PTE_ADDR(*pgdir_entryp)
	// pte'pa -> page'va		KADDR(PTE_ADDR(*pgdir_entryp))
	// offset					KADDR(PTE_ADDR(*pgdir_entryp)) + PTX(va);
	*ppte = (Pte*) KADDR(PTE_ADDR(*pgdir_entryp)) + PTX(va);

	return 0;
}
```

这段代码的大概意思就是，根据给定的页目录和`va`，找到对应的二级页表项，并将其赋值给`*ppte`。这段代码在之后的课上实验中会用到，如果没有理解那课上实验就会花费很多时间重新阅读，甚至重新编写。

最后`page_lookup`也是非常关键的代码

```c
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
	struct Page *pp;
	Pte *pte;

	/* Step 1: Get the page table entry. */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: Check if the page table entry doesn't exist or is not valid. */
	if (pte == NULL || (*pte & PTE_V) == 0) {
		return NULL;
	}

	/* Step 2: Get the corresponding Page struct. */
	/* Hint: Use function `pa2page`, defined in include/pmap.h . */
	pp = pa2page(*pte);
	if (ppte) {
		*ppte = pte;
	}

	return pp;
}
```

这段代码的作用就是找到`va`对应的页控制块`pp`并且返回，并且将对应的二级页表项存入`*ppte`。在课上实验中这个函数也非常重要。

## 实验体会

感觉lab2和lab0~lab1完全不是一个概念，课下实验真的好难，感觉非常复杂，各种概念互相纠缠，难以理解。在这次lab的时候我还是画图才能理解内存的布局，理解各个函数的调用关系和地址的多次转换。许多函数又需要调用前面的函数，非常非常的晦涩。

在完成填空的时候，一定要先阅读已经给出/完成的代码，不要重复造轮子，在这次的课上实验中，遍历空闲页链表的宏其实已经给出在`queue.h`中：

```c
/*
 * Iterate over the elements in the list named "head".
 * During the loop, assign the list elements to the variable "var"
 * and use the LIST_ENTRY structure member "field" as the link field.
 */
#define LIST_FOREACH(var, head, field)                                                             \
	for ((var) = LIST_FIRST((head)); (var); (var) = LIST_NEXT((var), field))

```

在课上实验的时候我还手动实现了一次，浪费了很多时间。

所以在实验的时候一定要仔细阅读代码和注释，理解每个函数的作用（我相信完全没用的函数是不会放在提供的代码里的）
