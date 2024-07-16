# OS Lab6 实验报告

## 思考题

### Thinking 6.1

> 示例代码中，父进程操作管道的写端，子进程操作管道的读端。如果现在想 让父进程作为“读者”，代码应当如何修改？

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int fildes[2];
int status;
char buf[100];

int main() {

    status = pipe(fildes);

    if (status == -1) {
        printf("error\n");
    }

    switch (fork()) {
        case -1:
            printf("fork error\n");
            return EXIT_FAILURE;

        case 0: /* 子进程 - 作为管道的写者 */
            close(fildes[0]); /* 关闭不用的读端 */
            write(fildes[1], "Hello world\n", 12); /* 向管道中写数据 */
            close(fildes[1]); /* 写入结束，关闭写端 */
            exit(EXIT_SUCCESS);

        default: /* 父进程 - 作为管道的读者 */
            close(fildes[1]); /* 关闭不用的写端 */
            read(fildes[0], buf, 100); /* 从管道中读数据 */
            printf("parent-process read: %s", buf); /* 打印读到的数据 */
            close(fildes[0]); /* 读取结束，关闭读端 */
            exit(EXIT_SUCCESS);
    }
}
```

### Thinking 6.2

> 上面这种不同步修改 pp_ref 而导致的进程竞争问题在 user/lib/fd.c 中 的 dup 函数中也存在。请结合代码模仿上述情景，分析一下我们的 dup 函数中为什么会出 现预想之外的情况？ 

dup函数的功能设想中应该是把一个文件描述符fd1的内容映射到另一个文件描述符fd2。

如果映射前fd1[0] = fd[1] = 1, pipe = 2

映射的时候，先将fd[0]++，现在fd[0] = 2

之后再讲pipe+1，此时pipe=2

但是如果在pipe+1之前产生调度，导致在_pipe_is_close的时候fd[0]=pipe，被认为写端关闭，就会出错。

### Thinking 6.3

> 阅读上述材料并思考：为什么系统调用一定是原子操作呢？如果你觉得不是 所有的系统调用都是原子操作，请给出反例。希望能结合相关代码进行分析说明。 

在MOS中，系统调用会陷入内核态，并且屏蔽中断，这时候整个系统调用过程不会被时钟中断打断。因此可以认为系统调用是原子的。

```assembly
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
/* Exercise 3.9: Your code here. */
	mfc0 	t0, CP0_CAUSE
	andi 	t0, 0x7c
	lw		t0, exception_handlers(t0)
	jr 		t0
```

### Thinking 6.4

> 仔细阅读上面这段话，并思考下列问题
>
>  • 按照上述说法控制 pipe_close 中 fd 和 pipe unmap 的顺序，是否可以解决上述场 景的进程竞争问题？给出你的分析过程。 
>
> • 我们只分析了 close 时的情形，在 fd.c 中有一个 dup 函数，用于复制文件描述符。 试想，如果要复制的文件描述符指向一个管道，那么是否会出现与 close 类似的问 题？请模仿上述材料写写你的理解。

对于第一个题目，按照fd,pipe的顺序unmap可以解决上述场景的竞争问题。

由于一般情况下，`pageref(fd) < pageref(pipe)`，先unmap fd可以让这个不等式仍然成立，这样在调度的时候也不会错误的出现两者相等。

对于第二个题目，同理`pageref(pipe) > pageref(fd)`，在dup的时候，先`++pipe->pp_ref`可以让这个不等式仍然成立，因此在调度的时候不会出现两者相等的错误情况。

### Thinking 6.5

> 思考以下三个问题。 
>
> • 认真回看 Lab5 文件系统相关代码，弄清打开文件的过程。 
>
> • 回顾 Lab1 与 Lab3，思考如何读取并加载 ELF 文件。 
>
> • 在 Lab1 中我们介绍了 data text bss 段及它们的含义，data 段存放初始化过的全 局变量，bss 段存放未初始化的全局变量。关于 memsize 和 filesize ，我们在 Note 1.3.4中也解释了它们的含义与特点。关于 Note 1.3.4，注意其中关于“bss 段并不在文 件中占数据”表述的含义。回顾 Lab3 并思考：elf_load_seg() 和 load_icode_mapper() 函数是如何确保加载 ELF 文件时，bss 段数据被正确加载进虚拟内存空间。bss 段 在 ELF 中并不占空间，但 ELF 加载进内存后，bss 段的数据占据了空间，并且初始 值都是 0。请回顾 elf_load_seg() 和 load_icode_mapper() 的实现，思考这一点 是如何实现的？

打开文件流程：

调用`file.c`中的`int open(const char *path, int mode)`

之后`open()`会调用`fd_alloc(&fd)`获取文件描述符，之后与文件系统服务通信，调用`fspic_open(path, mode, fd)`,并接受返回的消息。

加载ELF文件由`env.c`中的`load_icode()`函数实现

在`elf_load_seg`中

先从 ELF 文件中读取段头信息，其中包含段的虚拟地址（`p_vaddr`）、文件中的偏移量（`p_offset`）、段在文件中的大小（`p_filesz`）以及段在内存中的大小（`p_memsz`）。

之后根据段在内存中的大小（`p_memsz`），在虚拟内存中分配相应的空间。

之后将文件中的内容（大小为 `p_filesz`）复制到内存中相应的地址。对于 BSS 段，由于 `p_filesz` 为 0，不会复制任何数据。如果 `p_memsz` 大于 `p_filesz`，则将多出的部分（即 `p_memsz - p_filesz`）初始化为 0。这部分对应的就是 BSS 段。

具体来说，对于 `bss` 段，`p_filesz` 为 0，而 `p_memsz` 为 BSS 段需要占据的空间。函数会检测这种情况，并确保在内存中为 BSS 段分配空间，并将其初始化为 0。

`load_icode_mapper()` 函数负责将 ELF 文件映射到内存中，它会调用 `elf_load_seg()` 来处理具体的段加载。通过迭代 ELF 文件中的段表，`load_icode_mapper()` 将每一个段（包括 BSS 段）加载到内存中。

先从 ELF 文件中读取文件头信息，确定段表的位置和大小。之后遍历段表中的每一个段，调用 `elf_load_seg()` 将段加载到内存中。对于每一个段，如果是 BSS 段（`p_filesz` 小于 `p_memsz`），则 `elf_load_seg()` 会确保在内存中分配相应的空间并初始化为 0。

### Thinking 6.6

> 通过阅读代码空白段的注释我们知道，将标准输入或输出定向到文件，需要 我们将其 dup 到 0 或 1 号文件描述符（fd）。那么问题来了：在哪步，0 和 1 被“安排”为 标准输入和标准输出？请分析代码执行流程，给出答案。

```c
// user/init.c中
debugf("init: running sh\n");
// stdin should be 0, because no file descriptors are open yet
if ((r = opencons()) != 0) {
    user_panic("opencons: %d", r);
}
// stdout
if ((r = dup(0, 1)) < 0) {
    user_panic("dup: %d", r);
}
```

### Thinking 6.7

> 在 shell 中执行的命令分为内置命令和外部命令。在执行内置命令时 shell 不 需要 fork 一个子 shell，如 Linux 系统中的 cd 命令。在执行外部命令时 shell 需要 fork 一个子 shell，然后子 shell 去执行这条命令。
>
>  据此判断，在 MOS 中我们用到的 shell 命令是内置命令还是外部命令？请思考为什么 Linux 的 cd 命令是内部命令而不是外部命令？ 

```c
// sh.c
for (;;) {
    if (interactive) {
        printf("\n$ ");
    }
    readline(buf, sizeof buf);

    if (buf[0] == '#') { // 注释类不执行
        continue;
    }
    if (echocmds) { // echo类command会直接执行
        printf("# %s\n", buf);
    }
    if ((r = fork()) < 0) {
        user_panic("fork: %d", r);
    }
    if (r == 0) {
        runcmd(buf); // 可以看到只有子进程会执行指令
        exit();
    } else {
        wait(r);
    }
}
```

因此在MOS中，除了echocmod和注释，其他都需要fork子shell执行。

linux中可能因为cd多级目录如果一直fork会比较浪费空间，没有什么必要，因此是内部指令。

### Thinking 6.8

> 在你的 shell 中输入命令 ls.b | cat.b > motd。 
>
> • 请问你可以在你的 shell 中观察到几次 spawn ？分别对应哪个进程？
>
> • 请问你可以在你的 shell 中观察到几次进程销毁？分别对应哪个进程？

在`spawn()`函数中加入输出，可以看到

```c
$ ls.b | cat.b > motd
[00002803] pipecreate 
called spawn(), pid:00002803
called spawn(), pid:00003004
[00004006] destroying 00004006
[00004006] free env 00004006
i am killed ... 
[00003004] destroying 00003004
[00003004] free env 00003004
i am killed ... 
[00003805] destroying 00003805
[00003805] free env 00003805
i am killed ... 
[00002803] destroying 00002803
[00002803] free env 00002803
i am killed ... 
```

`spawn`了两次

分别是由最初被`fork`出的`2803`进程`spawn`出了`3805`进程

以及`3004`被`2803`进程在`parsecmd`时`fork`得到进程`spawn`出了`4006`进程。

销毁了4次

2803：由主shell进程fork出来的子shell进程，用于解析并执行当前命令；

3004：由2803进程fork出来的子进程，用于解析并执行管道右端的命令；

3805：由2803进程spawn出来的子进程，用于执行管道左边的命令；

4006：由3004进程spawn出来的子进程，用于执行管道右边的命令；

## 实验难点

lab6作为实验的最后一章，可以说是汇总了lab0-lab5的所有知识，之后站在与用户最近的角度，提供最顶层的接口。

在实验的过程中，遇到不懂的地方可以观察sh.c中的内容，之后按照调用顺序一个个函数看下去，就知道缺少的部分是什么了。最难的部分可能就是spawn函数了，因为一下子要填好多空，比较繁琐。但是根据提示一步步做也不是特别困难。主要还是得益于之前几个lab一个个做下来，对已有的接口和系统的结构已经有了较为熟悉的认识，因此写起来还算得心应手。

## 心得体会

一学期的os至此已经算是告一段落了（除了挑战项目），这门课无论是理论还是实验，相对难度都是蛮高的。本来想着结束的时候有很多话想说，但是真到了lab6提交上去之后，又觉得没有什么。课上实验有时候很难，课下实验有时候也很难，在与oo的双重压力下，这学期还是挺充实的>_<；最后运行shell可以交互的时候，还是挺有成就感的，虽然是站在课程组的肩膀上，但是对于我这种小白来说还是非常欣慰的。