# OS Lab3 实验报告

## 思考题

### Thinking 3.1

> Thinking 3.1 请结合 MOS 中的页目录自映射应用解释代码中 
>
> `e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_V `的含义。

`UVPT`：用户页表的起始处的内核虚拟地址

`PDX(UVPT)`：`UVPT`地址在页目录中是第几个（`pgdir[PDX(UVPT)]`）

`e->env_pgdir[PDX(UVPT)]`：当前进程内存中`UVPT`地址的页目录项

`e->env_pgdir`：进程 e 的页目录的内核**虚拟地址**

`PADDR(e->env_pgdir)`：进程 e 的页目录的**物理地址**

`PADDR(e->env_pgdir) | PTE_V`：页目录的物理基地址，加上权限位

整体：当前进程内存中UVPT地址的页目录项 = 页目录的物理基地址，加上权限位

### Thinking 3.2

> elf_load_seg 以函数指针的形式，接受外部自定义的回调函数 map_page。 请你找到与之相关的 data 这一参数在此处的来源，并思考它的作用。没有这个参数可不可以？为什么？ 

```c
// elfloader.c
int elf_load_seg(Elf32_Phdr *ph, const void *bin, elf_mapper_t map_page, void *data);

// env.c
static int load_icode_mapper(
    void *data, 
    u_long va, 
    size_t offset, 
    u_int perm,
    const void *src, 
    size_t len
) {
	struct Env *env = (struct Env *)data;
	...
	return page_insert(env->env_pgdir, env->env_asid, p, va, perm);
}

static void load_icode(struct Env *e, const void *binary, size_t size) {
 	//...
    panic_on(elf_load_seg(ph, binary + ph->p_offset, load_icode_mapper, e));
    //...
}
```

data 的来源是`load_icode`传入的进程控制块指针

调用顺序：`load_icode -> elf_load_seg -> load_icode_mapper`

如果没有`data`，`load_icode_mapper`就不能知道当前进程空间的页目录基地址和asid，无法`page_insert`，所以必须要有这个参数。

### Thinking 3.3

> 结合 elf_load_seg 的参数和实现，考虑该函数需要处理哪些页面加载的情况。

```c
int elf_load_seg(Elf32_Phdr *ph, const void *bin, elf_mapper_t map_page, void *data) {
	u_long va = ph->p_vaddr;
	size_t bin_size = ph->p_filesz;
	size_t sgsize = ph->p_memsz;
	u_int perm = PTE_V;
    // 先修改后面的权限位
	if (ph->p_flags & PF_W) {
		perm |= PTE_D;
	}

	int r;
	size_t i;
	u_long offset = va - ROUNDDOWN(va, PAGE_SIZE);
    // 如果va没有页面对其，就将多余的地址记为offset
    // 并且将PAGE_SIZE - offset的binary数据，写入对应页的对应地址
	if (offset != 0) {
		if ((r = map_page(data, va, offset, perm, bin,
				  MIN(bin_size, PAGE_SIZE - offset))) != 0) {
			return r;
		}
	}

	/* Step 1: load all content of bin into memory. */
    // 将段内页映射到物理空间
	for (i = offset ? MIN(bin_size, PAGE_SIZE - offset) : 0; i < bin_size; i += PAGE_SIZE) {
		if ((r = map_page(data, va + i, 0, perm, bin + i, MIN(bin_size - i, PAGE_SIZE))) !=
		    0) {
			return r;
		}
	}

	/* Step 2: alloc pages to reach `sgsize` when `bin_size` < `sgsize`. */
    // 把多余的空间用0填充满
	while (i < sgsize) {
		if ((r = map_page(data, va + i, 0, perm, NULL, MIN(sgsize - i, PAGE_SIZE))) != 0) {
			return r;
		}
		i += PAGE_SIZE;
	}
	return 0;
}
```

### Thinking 3.4

> 这里的 env_tf.cp0_epc 字段指示了进程恢复运行时 PC 应恢复到的位置。我们要运行的进 程的代码段预先被载入到了内存中，且程序入口为 e_entry，当我们运行进程时，CPU 将自动 从 PC 所指的位置开始执行二进制码。
>
> 你认为这里的 env_tf.cp0_epc 存储的是物理地址还是虚拟地址?

虚拟地址

### Thinking 3.5

> 试找出 0、1、2、3 号异常处理函数的具体实现位置。8 号异常（系统调用） 涉及的 do_syscall() 函数将在 Lab4 中实现。

```c
// traps.c

// 先定义了一些handler
extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);

// 用于分发不用的ExcCode
void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

// 用于处理未知（其实是未定义处理函数）的ExcCode
void do_reserved(struct Trapframe *tf) {
	print_tf(tf);
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}
```

```asm
# genex.S
# 其他的异常处理函数定义在这里
NESTED(handle_int, TF_SIZE, zero)
	mfc0    t0, CP0_CAUSE
	mfc0    t2, CP0_STATUS
	and     t0, t2
	andi    t1, t0, STATUS_IM7
	bnez    t1, timer_irq
timer_irq:
	li      a0, 0
	j       schedule
END(handle_int

BUILD_HANDLER tlb do_tlb_refill

#if !defined(LAB) || LAB >= 4
BUILD_HANDLER mod do_tlb_mod
BUILD_HANDLER sys do_syscall
#endif

BUILD_HANDLER reserved do_reserved

```

### Thinking 3.6

> 阅读 entry.S、genex.S 和 env_asm.S 这几个文件，并尝试说出时钟中断在哪些时候开启，在哪些时候关闭。

```asm
# 在 entry.S 中
exc_gen_entry: # 进入异常处理
	mfc0    t0, CP0_STATUS
	and     t0, t0, ~(STATUS_UM | STATUS_EXL | STATUS_IE) 
	mtc0    t0, CP0_STATUS # 禁止中断
/* Exercise 3.9: Your code here. */
	mfc0 	t0, CP0_CAUSE
	andi 	t0, 0x7c
	lw		t0, exception_handlers(t0)
	jr 		t0
	
# 在 env_asm.S 中，env_pop_tf函数负责reset时钟
LEAF(env_pop_tf)
.set reorder
.set at
	mtc0    a1, CP0_ENTRYHI
	move    sp, a0
	RESET_KCLOCK
	j       ret_from_exception
END(env_pop_tf)

# 在 genex.S 中
FEXPORT(ret_from_exception)
	RESTORE_ALL 
	eret # 打开中断
```

### Thinking 3.7

>  阅读相关代码，思考操作系统是怎么根据时钟中断切换进程的。 

在进程运行过程中，若时钟中断产生，则会触发MIPS中断，系统将PC指向 `0x800000080`，跳转到` .text.exc_gen_entry`代码段，进行异常分发；

```asm
NESTED(handle_int, TF_SIZE, zero)
	mfc0    t0, CP0_CAUSE
	mfc0    t2, CP0_STATUS
	and     t0, t2
	andi    t1, t0, STATUS_IM7
	bnez    t1, timer_irq
timer_irq:
	li      a0, 0
	j       schedule
END(handle_int)
```

由于是中断，会被分发到`handle_int`，从而进入`schedule`，进行调度

在`schedule`函数中

```c
void schedule(int yield) {
	static int count = 0; // remaining time slices of current env
	struct Env *e = curenv; // 先获取当前进程的进程控制块
	count--; // 可用时间片数量减一
	if ((yield != 0) // yield指定必须切换
        || (count == 0) // 时间片归零
        || (e == NULL) // 当前没有进程
        || (e->env_status != ENV_RUNNABLE) // 当前进程已被阻塞
       ) { // 以上四种条件就需要切换进程
		if (e != NULL && e->env_status == ENV_RUNNABLE) { // 当前有正在运行的进程
			TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
			TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
		} // 将其插入调度队列的尾部
		panic_on(TAILQ_EMPTY(&env_sched_list));
		e = TAILQ_FIRST(&env_sched_list); // 从队头取一个进程
		count = e->env_pri; // 时间片设置为优先级
		env_run(e);
	} else { // 不切换
		env_run(e);
	}
}
```

## 难点分析

本次lab的最大难点我认为就是进程的地址空间。

首先要认识到每个进程在其认知中，自己都是独占内存空间的。

在此基础上再去理解各个函数。

在此之前，指导书中写道：

>  虚拟地址 ULIM 是 kseg0 与 kuseg 的分界线，是系统给用户进程分配的最高地址。ULIM 以上的地方，kseg0 和 kseg1 两部分内存的访问不经过 TLB，这部分内存由内核管 理、所有进程共享。在 MOS 操作系统特意将一些内核的数据暴露到用户空间，使得进程不需要切换到内核态就能访问，这是 MOS 特有的设计。

```c
static int env_setup_vm(struct Env *e) { // 这个函数就是来分配进程用户空间的
	struct Page *p;
	try(page_alloc(&p));
	p->pp_ref++;
	e->env_pgdir = (Pde*) page2kva(p);
	// 由于可以直接访问一部分内核数据，因此可以直接拷贝页目录
	memcpy(e->env_pgdir + PDX(UTOP), base_pgdir + PDX(UTOP),
	       sizeof(Pde) * (PDX(UVPT) - PDX(UTOP)));
	// 也可以直接修改页目录
	e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_V;
	return 0;
}
```

还有就是要理解一下中断的处理方式和分发方式，这点在前面写过了，如果没有读懂这段代码extra部分就完蛋了。

其他的部分整体没有lab2难，可能是lab2已经接触过了**控制块**和队列宏这两个概念，所以lab3在操作的时候会更加熟悉。

## 实验体会

本次的课上实验exam部分由于读题不仔细把`clocks`理解为了进程开始到被调度用的时间间隔，但实际上是进程总共运行的时间总和。导致在课上实验的时候花费了一个多小时来找问题，但大多是无用功，直到后来增加了提示在看懂题目表达的意思是什么。所以在完成实验的时候要好好读题，想明白意思再开始写。而且我还天真的以为只需要写env_stat就能完成，其实要在sched.c里面修改e->scheds

```c
void schedule(int yield) {
	static int count = 0; // remaining time slices of current env
	struct Env *e = curenv;
	count--;
	if ((yield != 0) || (count == 0) || (e == NULL) || (e->env_status != ENV_RUNNABLE)) {
		if (e != NULL && e->env_status == ENV_RUNNABLE) {
			TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
			TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
		}
		panic_on(TAILQ_EMPTY(&env_sched_list));
		e = TAILQ_FIRST(&env_sched_list);
		count = e->env_pri;
		e->env_scheds++; // 写在这里
		env_run(e);
	} else {
		env_run(e);
	}
}

```

env_stat反而没有那么难写

```c
void env_stat(struct Env *e, u_int *pri, u_int *scheds, u_int *runs, u_int *clocks) {
	*pri = e->env_pri;
	*scheds = e->env_scheds;
	*runs = e->env_runs;
	if (*runs == 0) {
		e->env_clocks = 0;
	} else {
		e->env_clocks += ((struct Trapframe *)KSTACKTOP - 1)->cp0_count;
	}
	*clocks = e->env_clocks;
}
```

在extra部分，重点句**rs所指向的地址对应的值**，应该是`*tf->reg[rs]`

```c
void do_ri(struct Trapframe *tf) {
    u_long va = tf->cp0_epc;
    Pte *pte;
    page_lookup(curenv->env_pgdir, va, &pte);
    u_long pa = PTE_ADDR(*pte) | (va & 0xfff);
    u_long kva = KADDR(pa);

    int *instr = (int*) kva;
    int opCode = (*instr) >> 26;
    int _rs = ((*instr) >> 21) & 0x1f;
    int _rt = ((*instr) >> 16) & 0x1f;
    int _rd = ((*instr) >> 11) & 0x1f;
    int spc = ((*instr) >> 6) & 0x1f;
    int funcCode = (*instr) & 0x3f;

    if (opCode == 0 && funcCode == 0x3f) { //pmaxud
        u_int rs = tf->regs[_rs];
        u_int rt = tf->regs[_rt];
        u_int rd = 0;
        int i;
        for(i = 0; i < 32; i += 8) {
            u_int rs_i = rs & (0xff << i);
            u_int rt_i = rt & (0xff << i);
            if (rs_i < rt_i) {
                rd = rd | rt_i;
            } else {
                rd = rd | rs_i;
            }
        }
        tf->regs[_rd] = rd;
    } else if (opCode == 0 && funcCode == 0x3e) { //cas
        u_long *rs = tf->regs[_rs];
        u_long tmp = *rs;
        if (*rs == tf->regs[_rt]) {
            *rs = tf->regs[_rd];
        }
        tf->regs[_rd] = tmp;
    }
    tf->cp0_epc += 4;
}
```

