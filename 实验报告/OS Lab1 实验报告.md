# OS Lab1 实验报告

## 思考题

### Thinking 1.1 

> 请阅读附录中的编译链接详解，尝试分别使用实验环境中的原生 x86 工具 链（gcc、ld、readelf、objdump 等）和 MIPS 交叉编译工具链（带有 mips-linux-gnu前缀），重复其中的编译和解析过程，观察相应的结果，并解释其中向 objdump 传入的参数的含义。
>

`objdump`传入的参数：`objdump [-d|--disassemble] [-D|--disassemble-all] [-S|--source]`

> `-D`：反汇编所有section
>
> `-d`：反汇编特定指令机器码的section
>
> `-S`：尽可能反汇编出源代码

现有如下程序：`hello.c`

```c
#include <stdio.h>
int main() {
    printf("hello\n");
    return 0;
}
```

执行指令 `gcc -E hello.c `（只预处理）+ 重定向输出

```c
...
struct _IO_FILE;
struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;
typedef void _IO_lock_t;
struct _IO_FILE
{
  int _flags;
  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;
  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;
  struct _IO_marker *_markers;
  struct _IO_FILE *_chain;
  int _fileno;
  int _flags2;
  __off_t _old_offset;
  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];
  _IO_lock_t *_lock;
  __off64_t _offset;
  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  size_t __pad5;
  int _mode;
  char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];
};
...
extern int printf (const char *__restrict __format, ...);
int main()
{
    printf("Hello World!\n");
    return 0;
}
```

执行指令 `gcc -c hello.c` （只编译不链接）+ `objdump -S hello`

```assembly
hello：     文件格式 elf64-x86-64
Disassembly of section .text:
0000000000000000 <main>:
   0:   f3 0f 1e fa             endbr64 
   4:   55                      push   %rbp
   5:   48 89 e5                mov    %rsp,%rbp
   8:   48 8d 05 00 00 00 00    lea    0x0(%rip),%rax        # f <main+0xf>
   f:   48 89 c7                mov    %rax,%rdi
  12:   e8 00 00 00 00          call   17 <main+0x17> # 这里没有填入printf的地址
  17:   b8 00 00 00 00          mov    $0x0,%eax
  1c:   5d                      pop    %rbp
  1d:   c3                      ret    
```

执行指令 `gcc -o hello.c` （正常编译）+ `objdump -S hello`

```assembly
Disassembly of section .plt.sec:
0000000000001050 <puts@plt>:
    1050:       f3 0f 1e fa             endbr64 
    1054:       f2 ff 25 75 2f 00 00    bnd jmp *0x2f75(%rip)        # 3fd0 <puts@GLIBC_2.2.5>
    105b:       0f 1f 44 00 00          nopl   0x0(%rax,%rax,1)
    
Disassembly of section .text:
0000000000001149 <main>:
    1149:       f3 0f 1e fa             endbr64 
    114d:       55                      push   %rbp
    114e:       48 89 e5                mov    %rsp,%rbp
    1151:       48 8d 05 ac 0e 00 00    lea    0xeac(%rip),%rax        # 2004 <_IO_stdin_used+0x4>
    1158:       48 89 c7                mov    %rax,%rdi
    115b:       e8 f0 fe ff ff          call   1050 <puts@plt> # 这里填入了puts@plt的地址
    1160:       b8 00 00 00 00          mov    $0x0,%eax
    1165:       5d                      pop    %rbp
    1166:       c3                      ret    
```

### Thinking 1.2

> 尝试使用我们编写的 readelf 程序，解析之前在 target 目录下生成的内核 ELF 文 件。 
>
> 也许你会发现我们编写的 readelf 程序是不能解析 readelf 文件本身的，而我们刚 才介绍的系统工具 readelf 则可以解析，这是为什么呢？（提示：尝试使用 readelf -h，并阅读 tools/readelf 目录下的 Makefile，观察 readelf 与 hello 的不同）

使用编写的`readelf`解析`mos`：

```assembly
0:0x0
1:0x80020000
2:0x80021f20
3:0x80021f38
4:0x80021f50
5:0x0
6:0x0
7:0x0
8:0x0
9:0x0
10:0x0
11:0x0
12:0x0
13:0x0
14:0x0
15:0x0
16:0x0
```

使用系统的`readelf`解析我们写的`readelf`：`readelf -h readelf`

```
ELF 头：
  Magic：   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  类别:                              ELF64
  数据:                              2 补码，小端序 (little endian)
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI 版本:                          0
  类型:                              DYN (Position-Independent Executable file)
  系统架构:                          Advanced Micro Devices X86-64
  版本:                              0x1
  入口点地址：               0x1180
  程序头起点：          64 (bytes into file)
  Start of section headers:          14488 (bytes into file)
  标志：             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         13
  Size of section headers:           64 (bytes)
  Number of section headers:         31
  Section header string table index: 30
```

使用系统的`readelf`解析`hello`：

```
ELF 头：
  Magic：   7f 45 4c 46 01 01 01 03 00 00 00 00 00 00 00 00 
  类别:                              ELF32
  数据:                              2 补码，小端序 (little endian)
  Version:                           1 (current)
  OS/ABI:                            UNIX - GNU
  ABI 版本:                          0
  类型:                              EXEC (可执行文件)
  系统架构:                          Intel 80386
  版本:                              0x1
  入口点地址：               0x8049600
  程序头起点：          52 (bytes into file)
  Start of section headers:          746252 (bytes into file)
  标志：             0x0
  Size of this header:               52 (bytes)
  Size of program headers:           32 (bytes)
  Number of program headers:         8
  Size of section headers:           40 (bytes)
  Number of section headers:         35
  Section header string table index: 34
```

使用自己编写的的`readelf`解析`readelf`没有输出：`./readelf readelf`

观察`Makefile`：

```makefile
readelf: main.o readelf.o
        $(CC) $^ -o $@
hello: hello.c
        $(CC) $^ -o $@ -m32 -static -g
```

区别是`hello`使用了`-static`参数，不允许使用动态链接库

### Thinking 1.3

> 在理论课上我们了解到，MIPS 体系结构上电时，启动入口地址为 0xBFC00000 （其实启动入口地址是根据具体型号而定的，由硬件逻辑确定，也有可能不是这个地址，但一定是一个确定的地址），但实验操作系统的内核入口并没有放在上电启动地址，而是按照 内存布局图放置。思考为什么这样放置内核还能保证内核入口被正确跳转到？ （提示：思考实验中启动过程的两阶段分别由谁执行。） 

上电的过程中，首先由`bootloader` 将内核可执行文件拷贝到内存中，之后将控制权交给操作系统，只需要启动入口地址为 `bootloader `的入口地址。

在`include/mmu.h` 中的内存布局图可以看到，`Kernal Text`和`Kernal Stack`的地址区间是`0x8002 0000 ~ 0x8040 0000`，并且`Kernal Text`从`0x8002 0000`开始，地址增加。因此使用`linker script`设置`.text`的生成地址：

```assembly
OUTPUT_ARCH(mips)
ENTRY(_start)
SECTIONS {
        . = 0x80020000;
        .text : { *(.text) }
        .data : { *(.data) }
        bss_start = .;
        .bss : { *(.bss) }
        bss_end = .;
        . = 0x80400000;
        end = . ;
}
```

并且该脚本声明了程序入口为`_start()`，在`start.S`中，可以看到`_start`函数的实现以及其跳转到了`mips_init()`

```assembly
#include <asm/asm.h>
#include <mmu.h>

.text
EXPORT(_start)
.set at
.set reorder
/* Lab 1 Key Code "enter-kernel" */
        /* clear .bss segment */
        la      v0, bss_start
        la      v1, bss_end
clear_bss_loop:
        beq     v0, v1, clear_bss_done
        sb      zero, 0(v0)
        addiu   v0, v0, 1
        j       clear_bss_loop
/* End of Key Code "enter-kernel" */

clear_bss_done:
        /* disable interrupts */
        mtc0    zero, CP0_STATUS

        /* hint: you can refer to the memory layout in include/mmu.h */
        /* set up the kernel stack */
        li      sp, 0x80400000

        /* jump to mips_init */
        j       mips_init
```

## 难点分析

由`elf.h`中可以看到，`elf`文件头的结构体定义：

```c
typedef struct {
        unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
        Elf32_Half e_type;                /* Object file type */
        Elf32_Half e_machine;             /* Architecture */
        Elf32_Word e_version;             /* Object file version */
        Elf32_Addr e_entry;               /* Entry point virtual address */
        Elf32_Off e_phoff;                /* Program header table file offset */
        Elf32_Off e_shoff;                /* Section header table file offset */
        Elf32_Word e_flags;               /* Processor-specific flags */
        Elf32_Half e_ehsize;              /* ELF header size in bytes */
        Elf32_Half e_phentsize;           /* Program header table entry size */
        Elf32_Half e_phnum;               /* Program header table entry count */
        Elf32_Half e_shentsize;           /* Section header table entry size */
        Elf32_Half e_shnum;               /* Section header table entry count */
        Elf32_Half e_shstrndx;            /* Section header string table index */
} Elf32_Ehdr;
```

以及段的定义：

```c
/* Program segment header.  */

typedef struct {
        Elf32_Word p_type;   /* Segment type */
        Elf32_Off p_offset;  /* Segment file offset */
        Elf32_Addr p_vaddr;  /* Segment virtual address */
        Elf32_Addr p_paddr;  /* Segment physical address */
        Elf32_Word p_filesz; /* Segment size in file */
        Elf32_Word p_memsz;  /* Segment size in memory */
        Elf32_Word p_flags;  /* Segment flags */
        Elf32_Word p_align;  /* Segment alignment */
} Elf32_Phdr;
```

在我们需要实现的`readelf(const void *binary, size_t size)`函数中，需要定位段头表的位置，我最终的实现是这样的：

```c
sh_table        = (void*)((unsigned long)ehdr + (unsigned long)ehdr->e_shoff);
```

但是在一开始，我并没有将`ehdr`和`ehdr->e_shoff`强制转换为`unsigned long`，而是直接计算。由结构体的定义可以看到`ehdr`的类型是`Elf32_Ehdr`，`e_shoff`的类型是`Elf32_off`，`Elf32_off`的类型实际上为`typedef uint32_t Elf32_Off`，即`uint32_t`，即32位无符号整数，指针`*p`和32位无符号整数`x`相加会默认计算向后偏移`x`个`sizeof(p)`的地址，这显然与`e_shoff`的定义不符，偏移`e_shoff`个`Elf头`的地址显然是没有意义的。因此，

```c
sh_table        = (void*)(ehdr + ehdr->e_shoff);
```

这样写是不对的，这一点我在做课下实验的时候想了好久才想到。属于是c语言基础不够扎实。

## 实验体会

在完成课下实验的时候，不能只是简简单单的填空，将空缺内容按照写c语言完形填空练习的方式完成，而是要阅读整段代码，尽量去了解实验要传达的整体思想。比如这次的`printk`函数实现，`vprintfmt`的作用，对于课上`extra`的`vscanfmt`的完成有很大的启发意义。而且在课程组给出的代码中预先定义的变量对于实验的完成有着很大的启发意义。

