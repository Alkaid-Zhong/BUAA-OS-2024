# OS Lab4 实验报告

## 思考题

### Thinking 4.1

> 思考并回答下面的问题：
>
> 内核在保存现场的时候是如何避免破坏通用寄存器的？
> 系统陷入内核调用后可以直接从当时的 $a0-$a3 参数寄存器中得到用户调用 msyscall留下的信息吗？
> 我们是怎么做到让 sys 开头的函数“认为”我们提供了和用户调用 msyscall 时同样的参数的？
> 内核处理系统调用的过程对 Trapframe 做了哪些更改？这种修改对应的用户态的变化是什么？

*内核在保存现场的时候是如何避免破坏通用寄存器的？*

在触发中断/异常的时候，会进入`entry.S`的`exc_gen_entry`函数

```assembly
.section .text.exc_gen_entry
exc_gen_entry:
	SAVE_ALL
	/*
	* Note: When EXL is set or UM is unset, the processor is in kernel mode.
	* When EXL is set, the value of EPC is not updated when a new exception occurs.
	* To keep the processor in kernel mode and enable exception reentrancy,
	* we unset UM and EXL, and unset IE to globally disable interrupts.
	*/
	mfc0    t0, CP0_STATUS
	and     t0, t0, ~(STATUS_UM | STATUS_EXL | STATUS_IE)
	mtc0    t0, CP0_STATUS
	mfc0 	t0, CP0_CAUSE
	andi 	t0, 0x7c
	lw		t0, exception_handlers(t0)
	jr 		t0
```

其中首先执行`stackframe.h`中的`SAVE_ALL`宏

```assembly
// clang-format off
.macro SAVE_ALL
.set noat
.set noreorder
	mfc0    k0, CP0_STATUS
	andi    k0, STATUS_UM
	beqz    k0, 1f
	move    k0, sp
	/*
	* If STATUS_UM is not set, the exception was triggered in kernel mode.
	* $sp is already a kernel stack pointer, we don't need to set it again.
	*/
	li      sp, KSTACKTOP
1:
	subu    sp, sp, TF_SIZE
	sw      k0, TF_REG29(sp)
	mfc0    k0, CP0_STATUS
	sw      k0, TF_STATUS(sp)
	mfc0    k0, CP0_CAUSE
	sw      k0, TF_CAUSE(sp)
	mfc0    k0, CP0_EPC
	sw      k0, TF_EPC(sp)
	mfc0    k0, CP0_BADVADDR
	sw      k0, TF_BADVADDR(sp)
	mfhi    k0
	sw      k0, TF_HI(sp)
	mflo    k0
	sw      k0, TF_LO(sp)
	sw      $0, TF_REG0(sp)
	sw      $1, TF_REG1(sp)
	...
	sw      $28, TF_REG28(sp)
	sw      $30, TF_REG30(sp)
	sw      $31, TF_REG31(sp)
.set at
.set reorder
.endm
```

在`SAVE_ALL`中，先把sp寄存器复制到k0，之后依次复制其他的寄存器。

*系统陷入内核调用后可以直接从当时的 $a0-$a3 参数寄存器中得到用户调用 msyscall留下的信息吗？*

可以

*我们是怎么做到让 sys 开头的函数“认为”我们提供了和用户调用 msyscall 时同样的参数的？*

应该是将参数保存到了a0-a3和stackframe里面？

*内核处理系统调用的过程对 Trapframe 做了哪些更改？这种修改对应的用户态的变化是什么？*

将栈中存储的EPC寄存器值增加4，这是因为系统调用后，将会返回下一条指令，而用户程序会保证系统调用操作不在延迟槽内，所以直接加4得到下一条指令的地址，并将返回值存入v0寄存器。

### Thinking 4.2

> 思考 envid2env 函数: 为什么 envid2env 中需要判断 e->env_id != envid 的情况？如果没有这步判断会发生什么情况？

生成envid时，后十位为了方便从envs数组中直接取出Env，可能会有所重叠

```c
u_int mkenvid(struct Env *e) {
	static u_int i = 0;
	return ((++i) << (1 + LOG2NENV)) | (e - envs);
}
```

不判断的话可能取出来已经被销毁的进程。

### Thinking 4.3

> 思考下面的问题，并对这个问题谈谈你的理解：
>
> 请回顾 kern/env.c 文件 中 mkenvid() 函数的实现，该函数不会返回 0，请结合系统调用和 IPC 部分的实现与 envid2env() 函数的行为进行解释。

```c
u_int mkenvid(struct Env *e) {
	static u_int i = 0;
	return ((++i) << (1 + LOG2NENV)) | (e - envs);
}
```

由于是`(++i) << (1 + LOG2NENV)`，++i不为0，因此不会生成id为0的envid。

在指导书里面写过，在IPC调用的时候，envid为0即为curenv。

### Thinking 4.4

> 关于 fork 函数的两个返回值，下面说法正确的是：
> A、 fork 在父进程中被调用两次，产生两个返回值
> B、 fork 在两个进程中分别被调用一次，产生两个不同的返回值
> C、 fork 只在父进程中被调用了一次，在两个进程中各产生一个返回值
> D、 fork 只在子进程中被调用了一次，在两个进程中各产生一个返回值

C

### Thinking 4.5

> 我们并不应该对所有的用户空间页都使用 duppage 进行映射。那么究竟哪些用户空间页应该映射，哪些不应该呢？请结合 kern/env.c 中 env_init 函数进行的页面映射、 include/mmu.h 里的内存布局图以及本章的后续描述进行思考。
>

在 0 ~ USTACKTOP 范围的内存需要使用 duppage 进行映射;

USTACKTOP ~ UTOP 之间的 user exception stack 是用来进行页写入异常的，不会在处理COW异常时调用 fork() ,所以 user exception stack 这一页不需要共享；

USTACKTOP ~ UTOP 之间的 invalid memory 是为处理页写入异常时做缓冲区用的，所以同理也不需要共享；

UTOP以上页面的内存与页表是所有进程共享的，且用户进程无权限访问，不需要做父子进程间的duppage；

其上范围的内存要么属于内核，要么是所有用户进程共享的空间，用户模式下只可以读取。除只读、共享的页面外都需要设置 PTE_COW 进行保护。

### Thinking 4.6

> 在遍历地址空间存取页表项时你需要使用到 vpd 和 vpt 这两个指针，请参考 user/include/lib.h 中的相关定义，思考并回答这几个问题： 
>
> • vpt 和 vpd 的作用是什么？怎样使用它们？ 
>
> • 从实现的角度谈一下为什么进程能够通过这种方式来存取自身的页表？ 
>
> • 它们是如何体现自映射设计的？ 
>
> • 进程能够通过这种方式来修改自己的页表项吗？

vpd是页目录的首地址，加上偏移即可获取对应的页目录项pde，使用`vpd[PDX(va)]`访问。

vpt是页表的首地址，加上偏移即可获取对应的页表项pte，使用`vpt[VPN(va)]`访问。

从实现角度来看，操作系统在初始化页表时，会将页表和页目录的一部分映射到虚拟地址空间中，使得这些虚拟地址直接指向页表和页目录的实际物理地址。这种设计允许进程通过 `vpt` 和 `vpd` 访问自身的页表项。

自映射体现在定义上：

```c
#define vpt ((volatile Pte *)UVPT)
#define vpd ((volatile Pde *)(UVPT + (PDX(UVPT) << PGSHIFT)))
```

 进程不能通过这种方式来修改自己的页表项。该区域对用户只读不写，若想要增添页表项，需要陷入内核进行操作。

### Thinking 4.7

> 在 do_tlb_mod 函数中，你可能注意到了一个向异常处理栈复制 Trapframe 运行现场的过程，请思考并回答这几个问题： 
>
> • 这里实现了一个支持类似于“异常重入”的机制，而在什么时候会出现这种“异常重 入”？ 
>
> • 内核为什么需要将异常的现场 Trapframe 复制到用户空间？

当出现COW异常时，需要使用用户态的系统调用发生中断，即中断重入；


内核将Trapframe复制到用户空间，主要是为了让用户程序能够访问异常发生时的状态信息，这在调试和信号处理中特别有用。

### Thinking 4.8

> 在用户态处理页写入异常，相比于在内核态处理有什么优势？

隔离错误影响：如果在用户态处理异常，错误和崩溃通常被限制在单个进程内，不会影响整个操作系统的稳定性。

提升性能：减少上下文切换，解放内核，不用内核执行大量的页面拷贝工作。

简化内核设计

### Thinking 4.9

> 请思考并回答以下几个问题：
>
>  • 为什么需要将 syscall_set_tlb_mod_entry 的调用放置在 syscall_exofork 之前？
>
>  • 如果放置在写时复制保护机制完成之后会有怎样的效果？

`syscall_exofork()`返回后父子进程各自执行自己的进程，子进程需要修改`entry.S`中定义的env指针，涉及到对COW页面的修改，会触发COW写入异常，COW中断的处理机制依赖于`syscall_set_tlb_mod_entry`，所以将 `syscall_set_tlb_mod_entry `的调用放置在` syscall_exofork `之前；

父进程在调用写时复制保护机制可能会引发缺页异常，而异常处理未设置好，则不能正常处理。

## 难点分析

在本次实验中，区分那些函数是在用户态，哪些函数是在内核态，是一个非常重要并且容易被忽视的难点。比如`syscall`系列函数均是在用户态的，而他们的具体实现`sys_*`系列函数则是内核态的函数。在`lab4-2-exam`测试中，获取当前进程的进程控制块，内核态下使用的是`Struct Env *curenv`，而用户态下使用的是`Struct Env *env`，要准确区分两者的适用范围。

## 实验体会

在`lab4-2-exam`中，花费了很长时间来尝试如何在用户态下获取当前进程的进程控制块。由于这次的题目是trace，导致如果使用`syscall_getenvid`获取的话会导致陷入syscall的死循环中，找了好久才想起来再用户态下有`Struct Env *env`来表示当前进程的进程控制块（课下没好好读代码导致的）。

本来以为这次吸取了之前的教训，仔细阅读了.c文件，结果这个定义在.h里面...。

以后还是要更认真的读代码...
